/*
 * kernel/sco3242_325.c - kernel access functions for SCO 3.2v4.2
 *			  and 3.2v5.0.0
 */


/*
 * Copyright 1995 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell <abe@cc.purdue.edu>
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * MODIFICATION HISTORY
 *
 * 95/06/05 created by Victor A. Abell <abe@cc.purdue.edu>
 *
 *
 * 95/07/30 modified by Bela Lubkin <belal@sco.com>
 *     -- Added code to find the real socket head, which may be hidden
 *        behind an in-kernel rlogind/telnetd stream head.
 *
 * 95/07/31 modified by Victor A. Abell <abe@cc.purdue.edu>
 *     -- Moved reading of TCP control block into k_getuid() so it will
 *	  be initialized at each call of k_getuid(), thus enabling the
 *	  -b abd -w modes to function properly.
 */

#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/file.h>
#include <sys/immu.h>
#include <sys/inode.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/socket.h>

#if	sco<500
#include <sys/net/protosw.h>
#include <sys/net/socketvar.h>
#else	/* sco>=500 */
#include <sys/protosw.h>
#include <sys/socketvar.h>
#endif	/* sco<500 */

#include <sys/sysi86.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/var.h>

#include <sys/netinet/in.h>
#include <sys/net/route.h>
#include <sys/netinet/in_pcb.h>

#if   sco>=500
#define       _INKERNEL
#include <sys/net/iknt.h>
#undef        _INKERNEL
#endif	/* sco>=500 */

/*
 * This confusing sequence of redefinitions of xdevmap allows the sizing
 * of the copy of the kernel's xdevmap[] table to be dynamic, based on the
 * kernel's nxdevmaps value.
 *
 * The net result is that there is a dummy struct XDEVMAP[1] that is never
 * used.  The copy of the kernel's xdevmap[] table is stored in the space
 * malloc()'d in k_open() and addressed by Xdevmap.  The last redefinition
 * of xdevmap to Xdevmap causes the macroes of <sys/sysmacros.h> to use
 * Xdevmap.
 *
 * All this is done: 1) to avoid having to allocate a large amount of fixed
 * space in advance to a copy of the kernel's xdevmap; and 2) to keep CC from
 * complaining about the absence of a "struct xdevmap xdevmap[]," matching
 * the "extern struct xdevmap xdevmap[]" declaration in <sys/sysmacros.h>,
 * while still allowing the use of the equivalent of a "struct xdevmap *"
 * construct instead, particularly with the kernel forms of the major() and
 * minor() macroes.
 */

#define	xdevmap XDEVMAP
#define	_INKERNEL
#include <sys/sysmacros.h>
#undef	_INKERNEL
static struct XDEVMAP XDEVMAP[1], *Xdevmap;
#undef	xdevmap
#define	xdevmap Xdevmap

#include "identd.h"
#include "error.h"


/*
 * Local definitions
 */

#define	KMEM		"/dev/kmem"	/* kernel memory device */
#define	N_UNIX		"/unix"		/* kernel image */
#define	PROCBFRD	32		/* proc structure buffer size */


/*
 * Local variables
 */

static int Kmem = -1;			/* kernel memory file descriptor */
static off_t Kp = (off_t)NULL;		/* proc table address */
static int nxdevmaps = -1;		/* maximum kernel xdevmap[] index */

struct nlist Nl[] = {			/* kernel image name list */

	{ "nxdevmaps",	0, 0, 0, 0, 0 },
#define	X_NXD					0

	{ "proc",	0, 0, 0, 0, 0 },
#define	X_PROC					1

	{ "sockdev",	0, 0, 0, 0, 0 },
#define	X_SDEV					2

	{ "socktab",	0, 0, 0, 0, 0 },
#define	X_STAB					3

	{ "tcb",	0, 0, 0, 0, 0 },
#define	X_TCB					4

	{ "v",		0, 0, 0, 0, 0 },
#define	X_VAR					5

	{ "xdevmap",	0, 0, 0, 0, 0 },
#define	X_XDEV					6

#if   sco>=500
	{ "ikntrinit",  0, 0, 0, 0, 0 },
#define X_IKNTR					7
#endif /* sco>=500 */

	{ NULL,         0, 0, 0, 0, 0 }
};

static char *Psb;			/* proc structure buffer */
static int Sdev;			/* kernel's socket device */
static off_t Stab;			/* kernel's socket table address */
static char *Uab;			/* user area buffer */
static struct var Var;			/* kernel var struct */


/*
 * k_open() - open access to kernel memory
 */

int
k_open()
{
	int err, i;
	size_t len;

	if ((Kmem = open(KMEM, O_RDONLY, 0)) < 0)
		ERROR1("main: open(%s)", KMEM);
/*
 * Get the kernel name list and check it.
 */
	if (nlist(N_UNIX, Nl) < 0)
		ERROR1("main: nlist(%s)", N_UNIX);
	for (err = i = 0; Nl[i].n_name; i++) {
		if (Nl[i].n_value == (long)NULL) {

#if   sco>=500
			if (i == X_IKNTR)
			
			/*
			 * The ikntrinit kernel entry point is optional, so
			 * it's OK if nlist() fails to find its address.
			 */
				continue;
#endif	/* sco>=500 */

			if (syslog_flag)
				syslog(LOG_ERR, "no kernel address for %s",
					Nl[i].n_name);
			(void) fprintf(stderr, "no kernel address for %s\n",
				Nl[i].n_value);
			err++;
		}
	}
	if (err)

k_open_err_exit:

		ERROR("main: k_open");
/*
 * Read or set kernel values:
 *
 *	Kp = process table address;
 *	Sdev = socket device major number;
 *	Stab = socket table address;
 *	Var = kernel environment structure.
 */
	Kp = (off_t)Nl[X_PROC].n_value;
	if (kread((off_t)Nl[X_SDEV].n_value, (char *)&Sdev, sizeof(Sdev))) {
		if (syslog_flag)
			syslog(LOG_ERR, "sockdev read (%#x)",
				Nl[X_SDEV].n_name);
		(void) fprintf(stderr, "sockdev read (%#x)\n",
			Nl[X_SDEV].n_name);
		err++;
	}
	Stab = (off_t)Nl[X_STAB].n_value;
	if (kread((off_t)Nl[X_VAR].n_value, (char *)&Var, sizeof(Var))) {
		if (syslog_flag)
			syslog(LOG_ERR, "var struct read (%#x)",
				Nl[X_VAR].n_name);
		(void) fprintf(stderr, "var struct read (%#x)\n",
			Nl[X_VAR].n_name);
		err++;
	}
	if (err)
		goto k_open_err_exit;
/*
 * Get extended device table parameters.  These are needed by the kernel
 * versions of the major() and minor() device numbers; they identify socket
 * devices and assist in the conversion of socket device numbers to socket
 * table addresses.
 */
	if (kread((off_t)Nl[X_NXD].n_value, (char *)&nxdevmaps,
		sizeof(nxdevmaps))
	)
		ERROR1("main: k_open nxdevmaps read (%#x)", Nl[X_NXD].n_name);
	if (nxdevmaps < 0)
		ERROR1("main: k_open nxdevmaps size (%d)", nxdevmaps);
	len = (size_t)((nxdevmaps + 1) * sizeof(struct XDEVMAP));
	if ((Xdevmap = (struct XDEVMAP *)malloc(len)) == (struct XDEVMAP *)NULL)
		ERROR1("main: xdevmap malloc (%d)", len);
	if (kread((off_t)Nl[X_XDEV].n_value, (char *)Xdevmap, len))
		ERROR1("main: xdevmap read (%#x)", Nl[X_XDEV].n_value);
/*
 * Allocate user area and proc structure buffers.
 */
	if ((Uab = (char *)malloc(MAXUSIZE * NBPC)) == (char *)NULL)
		ERROR1("main: k_open user struct size (%d)", MAXUSIZE * NBPC);
	if ((Psb = (char *)malloc(sizeof(struct proc) * PROCBFRD)) == NULL)
		ERROR1("main: k_open proc stuct size (%d)",
			sizeof(struct proc) * PROCBFRD);
	return(0);
}


/*
 * k_getuid() - get User ID for local and foreign TCP address combination
 *
 * returns: -1 if address combination not found
 *	     0 if address combination found and:
 *			*uid = User ID of process with open socket file
 *			       having the address combination
 */

int
k_getuid(faddr, fport, laddr, lport, uid)
	struct in_addr *faddr;		/* foreign network address */
	int fport;			/* foreign port */
	struct in_addr *laddr;		/* local network address */
	int lport;			/* local port */
	int *uid;			/* returned UID pointer */
{
	struct file f;
	int i, j, nf, pbc, px;
	struct inode in;
	struct proc *p;
	struct inpcb pcb, tcb;
	struct queue *q, qb;
	off_t sa, spa;
	struct user *u;

#if   sco>=500
	iknt_t ikb;
#endif /* sco>=500 */

/*
 * Search the kernel's TCP control block chain for one whose local and foreign
 * addresses match.
 */
	if (kread((off_t)Nl[X_TCB].n_value, (char *)&tcb, sizeof(tcb))) {
		if (syslog_flag)
			syslog(LOG_ERR, "TCP struct read (%#x)",
				Nl[X_TCB].n_name);
		(void) fprintf(stderr, "TCB struct read (%#x)\n",
			Nl[X_TCB].n_name);
		ERROR("k_getuid: no TCP control block head");
	}
	i = 0;
	pcb = tcb;
	do {
		if (pcb.inp_faddr.s_addr == faddr->s_addr
		&&  pcb.inp_laddr.s_addr == laddr->s_addr
		&&  pcb.inp_fport == fport && pcb.inp_lport == lport) {
			i = 1;
			break;
		}
	} while (pcb.inp_next != (struct inpcb *)NULL
	     &&  pcb.inp_next != tcb.inp_prev
	     &&  kread((off_t)pcb.inp_next, (char *)&pcb, sizeof(pcb)) == 0);
	if ( ! i)
		return(-1);
/*
 * Follow the stream input queue for the TCP control block to its end
 * to get the socket address.
 */
	for (q = pcb.inp_q, qb.q_ptr = (caddr_t)NULL; q; q = qb.q_next) {
		if (kread((off_t)q, (char *)&qb, sizeof(qb)))
			return(-1);
	}

#if   sco>=500
/*
 * If this is an in-kernel rlogind/telnetd stream head, follow its private
 * pointer back to the original stream head where the socket address may
 * be found.
 */
	if (Nl[X_IKNTR].n_value && qb.q_qinfo
	&&  qb.q_qinfo == (struct qinit *)Nl[X_IKNTR].n_value) {
		if (kread((off_t)qb.q_ptr, (char *)&ikb, sizeof(ikb)))
			return(-1);
		qb.q_ptr = ikb.ik_oqptr;
	}
#endif /* sco>=500 */

	if (qb.q_ptr == (caddr_t)NULL)
		return(-1);
/*
 * Search the process table, the user structures associated with its proc
 * structures, and the open file structures associated with the user structs
 * to find an inode whose major device is Sdev and whose socket structure's
 * address matches the address identified from the TCB control block scan.
 */
	for (pbc = px = 0, u = (struct user *)Uab; px < Var.v_proc; px++) {
		if (px >= pbc) {

		/*
		 * Refill the proc struct buffer.
		 */
			i = Var.v_proc - px;
			if (i > PROCBFRD)
				i = PROCBFRD;
			j = kread((off_t)(Kp + (px * sizeof(struct proc))),
				Psb, sizeof(struct proc) * i);
			pbc = px + i;
			if (j) {
				px += i;
				continue;
			}
			p = (struct proc *)Psb;
		} else
			p++;
		if (p->p_stat == 0 || p->p_stat == SZOMB)
			continue;
	/*
	 * Get the user area for the process and follow its file structure
	 * pointers to their inodes.  Compare the addresses of socket
	 * structures to the address located in the TCP control block scan
	 * and return the User ID of the first process that has a match.
	 */
		if (sysi86(RDUBLK, (int)p->p_pid, Uab, MAXUSIZE * NBPC) == -1)
			continue;
		nf = u->u_nofiles ? u->u_nofiles : Var.v_nofiles;
		for (i = 0; i < nf; i++) {
			if (u->u_ofile[i] == (struct file *)NULL
			||  kread((off_t)u->u_ofile[i], (char *)&f, sizeof(f))
			||  f.f_count == 0
			||  f.f_inode == (struct inode *)NULL
			||  kread((off_t)f.f_inode, (char *)&in, sizeof(in)))
				continue;
			if ((in.i_ftype & IFMT) != IFCHR
			||  major(in.i_rdev) != Sdev)
				continue;
			spa = Stab + (minor(in.i_rdev)*sizeof(struct socket *));
			if (kread(spa, (char *)&sa, sizeof(sa)))
				continue;
			if ((caddr_t)sa == qb.q_ptr) {
				*uid = (int)p->p_uid;
				return(0);
			}
		}
	}
	return(-1);
}


/*
 * kread() - read from kernel memory
 */

int
kread(addr, buf, len)
	off_t addr;			/* kernel memory address */
	char *buf;			/* buffer to receive data */
	unsigned len;			/* length to read */
{
	int br;

	if (lseek(Kmem, (off_t)addr, SEEK_SET) == (off_t)-1L)
		return(1);
	if ((br = read(Kmem, buf, len)) < 0)
		return(1);
	return(((unsigned)br == len) ? 0 : 1);
}

