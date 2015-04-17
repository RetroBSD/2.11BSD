/*
** kernel/uw2.c                UnixWare v2.x specific kernel access functions
**
**
** Last update: 28 Nov 1995
**
** Please send mods/bug fixes/bug reports to: Paul F. Wells <paul@wellserv.com>
**
*/

#define _KMEMUSER
#include <sys/types.h>
#include <sys/ksynch.h>
#include <sys/flock.h>
#include <sys/pid.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/cred.h>
#include <sys/session.h>
#include <sys/socket.h>
#include <sys/strsubr.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp_kern.h>
#include <sys/fs/snode.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#undef _KMEMUSER

#include <sys/ksym.h>
#include <sys/sysmacros.h>

#include <arpa/inet.h>

#include <fcntl.h>

#include "paths.h"
#include "kvm.h"
#include "identd.h"
#include "error.h"

#include <stdio.h>	/* this kinda reeks */

#ifndef TRUE
enum { FALSE, TRUE };
#endif

static kvm_t *kd;

int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if (!(kd = kvm_open(path_unix, path_kmem, NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");
  
#ifndef UW2	/* don't really need this */
  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) != 0)
    ERROR("main: kvm_nlist");
#endif

  return 0;
}

/*
** Get a piece of kernel memory with error handling.
** Returns 1 if call succeeded, else 0 (zero).
*/
static int
getbuf(long addr,void *buf,int len,char *what)
{
	if (kvm_read(kd, addr, buf, len) < 0) {
		if (syslog_flag)
			syslog(LOG_ERR, "getbuf: kvm_read(%08x, %d) - %s : %m",
					addr, len, what);
		return 0;
	}
	return 1;
}

int
k_getuid(struct in_addr *faddr,int fport,struct in_addr *laddr,int lport,int *uid)
{
	int gotit;
	ulong_t info;
	struct stat tcpstat;
	dev_t tcp_clone;
	minor_t tcp_minor;
	snode_t **snode_base, *snodep, spec_node;
	int snode_hash;
	vnode_t *vnodep;
	struct inpcb pcb, *tcb_first;
	proc_t *proc_head, *procp, uproc;
	int maxfd;
	int ifdt, fdtable_len;
	fd_entry_t *fdp;
	file_t *filep, file_entry;
	cred_t creds;

	/* initialize & find pcb for specified connection */

	if (stat("/dev/tcp", &tcpstat) != 0)
		return -1;
	tcp_minor = minor(tcpstat.st_rdev);

	info = 0;
	tcb_first = NULL;
	if (getksym("tcb", (ulong_t *) &tcb_first, &info) < 0)
		return -1;

	for (gotit = FALSE, pcb.inp_prev = tcb_first; ; ) {
		/* why do we have to go backward? */
		if (!getbuf((long) pcb.inp_prev, &pcb, sizeof(pcb), "inp_prev"))
			return -1;
		if (
			pcb.inp_faddr.s_addr == faddr->s_addr &&
			pcb.inp_laddr.s_addr == laddr->s_addr &&
			pcb.inp_fport == fport &&
			pcb.inp_lport == lport
		) {
			/* I screwed up: 2.01 allowed makedev() */
			tcp_clone = makedevice(tcp_minor, pcb.inp_minor);
			gotit = TRUE;
			break;
		}
		if (pcb.inp_prev == NULL || pcb.inp_prev == tcb_first) break;
	}
	if (!gotit) return -1;

	/* now crawl through snodes to find vnode of clone device */

	info = 0;
	snode_base = NULL;
	if (getksym("spectable", (ulong_t *) &snode_base, &info) < 0)
		return -1;
	snode_hash = SPECTBHASH(tcp_clone);
	if (!getbuf((long) (snode_base + snode_hash), &snodep, sizeof(snodep), "snode_hash"))
		return -1;

	for (gotit = FALSE; snodep != NULL ; snodep = spec_node.s_next) {
		if (!getbuf((long) snodep, &spec_node, sizeof(spec_node), "snodep"))
			return -1;
		if (spec_node.s_dev == tcp_clone) {
			vnodep = (vnode_t *) ((char *) snodep + offsetof(struct snode, s_vnode));
			gotit = TRUE;
			break;
		}
	}
	if (!gotit || (spec_node.s_flag & SINVALID)) return -1;

	/* find process with this vnode */

	info = 0;
	procp = NULL;
	if (getksym("practive", (ulong_t *) &procp, &info) < 0)
		return -1;
	if (!getbuf((long) procp, &proc_head, sizeof(proc_head), "practive"))
		return -1;

	for (gotit = FALSE, procp = proc_head; procp != NULL; procp = uproc.p_next) {
		if (!getbuf((long) procp, &uproc, sizeof(uproc), "procp"))
			return -1;
		if((maxfd = uproc.p_fdtab.fdt_sizeused) == 0) continue;
		fdtable_len = maxfd * sizeof(fd_entry_t);
		if ((fdp = (fd_entry_t *) malloc(fdtable_len)) == NULL)
			return -1;
		if (!getbuf((long) uproc.p_fdtab.fdt_entrytab, fdp, fdtable_len, "fdt_entrytab")) {
			free(fdp);
			return -1;
		}
		for (ifdt = 0; ifdt < maxfd; ifdt++) {
			if (fdp[ifdt].fd_status != FD_INUSE) continue;
			filep = fdp[ifdt].fd_file;
			if (!getbuf((long) filep, &file_entry, sizeof(file_entry), "filep")) {
				free(fdp);
				return -1;
			}
			if (vnodep == file_entry.f_vnode) {
				if (file_entry.f_cred != NULL) {
					if (!getbuf((long) file_entry.f_cred, &creds, sizeof(creds), "f_cred")) {
						free(fdp);
						return -1;
					}
					free(fdp);
					gotit = TRUE;
					goto got_proc;
				}
			}
		}
		free(fdp);
	}
got_proc:	;	/* maybe */
	if (!gotit) return -1;

	/* wow! */
	*uid = creds.cr_ruid;

	return 0;
}
