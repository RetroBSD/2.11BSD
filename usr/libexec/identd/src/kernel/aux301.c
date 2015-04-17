/*
** kernel/aux301.c	Low level kernel access functions for
**                      A/UX 3.0.1 and newer
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 September 1995 by Herb Weiner <herbw@wiskit.com>
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include "kvm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/config.h>

#include <sys/socketvar.h>

#define KERNEL

#include <sys/file.h>

#include <fcntl.h>

#include <sys/user.h>
#include <sys/wait.h>

#undef KERNEL

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include "identd.h"
#include "error.h"

#include <a.out.h>

#include <sys/mmu.h>
#include <sys/var.h>

/*--------------------------------------------------------------
 * The Apple-supplied nlist(3c) function is seriously broken.
 * Here's a replacement.
 *--------------------------------------------------------------
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*--------------------------------------------------------------------------
 * Ported to Apple A/UX by Herb Weiner <herbw@wiskit.com> September 16, 1995
 *--------------------------------------------------------------------------
 */

typedef struct nlist NLIST;
#define	ISVALID(p)	(p->n_nptr && p->n_nptr[0])

int nlist(name, list)
	char	*name;
	NLIST	*list;
{
	register NLIST	*p;
	register NLIST	*s;
	struct filehdr	ebuf;
	FILE			*fstr;
	FILE			*fsym;
	NLIST			nbuf;
	off_t			strings_offset;
	off_t			symbol_offset;
	off_t			symbol_size;
	int				entries;
	int				len;
	int				maxlen;
	char			sbuf [256];
	char			*np;

	entries = -1;

	if (!(fsym = fopen (name, "r")))
		return(-1);

	if (fread((char *)&ebuf, sizeof(struct filehdr), 1, fsym) != 1
		|| BADMAG(&ebuf))
		goto done1;

	symbol_offset = ebuf.f_symptr;
	symbol_size = ebuf.f_nsyms * sizeof (NLIST);
	strings_offset = symbol_offset + symbol_size;
	if (fseek(fsym, symbol_offset, SEEK_SET))
		goto done1;

	if (!(fstr = fopen(name, "r")))
		goto done1;

	/*
	 * clean out any left-over information for all valid entries.
	 * Type and value defined to be 0 if not found; historical
	 * versions cleared other and desc as well.  Also figure out
	 * the largest string length so don't read any more of the
	 * string table than we have to.
	 */
	for (p = list, entries = maxlen = 0; ISVALID(p); ++p, ++entries)
	{
		p->n_value = 0;
		p->n_type = 0;
		p->n_scnum = 0;
		p->n_sclass = 0;
		p->n_numaux = 0;
		if ((len = strlen(p->n_nptr)) > maxlen)
			maxlen = len;
	}
	if (maxlen >= sizeof(sbuf))
	{
		(void)fprintf(stderr, "nlist: symbol too large.\n");
		entries = -1;
		goto done2;
	}

	for (s = &nbuf; symbol_size > 0; symbol_size -= sizeof(NLIST))
	{
		if (fread((char *)s, sizeof(NLIST), 1, fsym) != 1)
			goto done2;
		if (s->n_zeroes == 0)
		{
			if (!s->n_offset)
				continue;
			if (fseek(fstr, strings_offset + s->n_offset, SEEK_SET))
				goto done2;
			(void)fread(sbuf, sizeof(sbuf[0]), maxlen, fstr);
			np = sbuf;
		}
		else
			np = s->n_name;

		for (p = list; ISVALID(p); p++)
			if (!strcmp(p->n_nptr, np))
			{
				p->n_value = s->n_value;
				p->n_type = s->n_type;
				p->n_scnum = s->n_scnum;
				p->n_sclass = s->n_sclass;
				p->n_numaux = s->n_numaux;
				if (!--entries)
					goto done2;
			}
	}
done2:	(void)fclose(fstr);
done1:	(void)fclose(fsym);
	return(entries);
}

/*--------------------------------------------------------------
 * End of replacement nlist(3c)
 *--------------------------------------------------------------
 */

extern void *calloc();
extern void *malloc();

#define N_FILE 0
#define N_TCB 1
#define N_V 2
#define N_LAST 3

struct nlist nl [N_LAST+1];

static kvm_t *kd;

static struct file *xfile;

static int nfile;

static struct inpcb tcb;


int k_open()
{
  path_unix = "/unix";
  path_kmem = "/dev/kmem";

  /*
  ** Open the kernel memory device
  */
  if (!(kd = kvm_open(path_unix, path_kmem, NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");

  /* Kludge - have to do it here or A/UX 3 will barf */

  nl[N_FILE].n_nptr = "file_pageh";
  nl[N_TCB].n_nptr  = "tcb";
  nl[N_V].n_nptr  = "v";
  nl[N_LAST].n_nptr = "";

  /*
  ** Extract offsets to the needed variables in the kernel
  */

  if (nlist(path_unix,nl) != 0)
  	ERROR("main: nlist");

  return 0;
}


/*
** Get a piece of kernel memory with error handling.
** Returns 1 if call succeeded, else 0 (zero).
*/
static int getbuf(addr, buf, len, what)
  long addr;
  char *buf;
  int len;
  char *what;
{

  if (kvm_read(kd, addr, buf, len) < 0)
  {
    if (syslog_flag)
      syslog(LOG_ERR, "getbuf: kvm_read(%08x, %d) - %s : %m",
	     addr, len, what);

    return 0;
  }
  
  return 1;
}



/*
** Traverse the inpcb list until a match is found.
** Returns NULL if no match.
*/
static struct socket *
  getlist(pcbp, faddr, fport, laddr, lport)
  struct inpcb *pcbp;
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
  struct inpcb *head;

  if (!pcbp)
    return NULL;


  head = pcbp->inp_prev;
  do 
  {
    if ( pcbp->inp_faddr.s_addr == faddr->s_addr &&
	 pcbp->inp_laddr.s_addr == laddr->s_addr &&
	 pcbp->inp_fport        == fport &&
	 pcbp->inp_lport        == lport )
      return pcbp->inp_socket;
  } while (pcbp->inp_next != head &&
	   getbuf((long) pcbp->inp_next,
		  pcbp,
		  sizeof(struct inpcb),
		  "tcblist"));

  return NULL;
}



/*
** Return the user number for the connection owner
*/
int k_getuid(faddr, fport, laddr, lport, uid)
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
{
  long addr;
  struct socket *sockp;
  int i;
  struct ucred ucb;
  int nf;
  struct file *xf;
  struct var my_var;
  int FILESPPG = (PAGESIZE / sizeof (struct file)); /* Files per Page */
  int file_page_size = FILESPPG * sizeof (struct file);

  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */

  if (!getbuf(nl[N_V].n_value, &my_var, sizeof(my_var), "v"))
    return -1;

  /*-----------------------------------------------------
   * Round UP number of files to integral number of pages
   * The number of pages ACTUALLY allocated may be fewer,
   * if not in use.
   *-----------------------------------------------------
   */

  if (my_var.v_file <= 3)
    ERROR("k_getuid: System name list out of date");

  if (my_var.v_file % FILESPPG)
    nf = (my_var.v_file / FILESPPG + 1) * FILESPPG;
  else
    nf = my_var.v_file;

  if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file"))
    return -1;

  xfile = (struct file *) calloc (nf, sizeof(struct file));
  if (!xfile)
    ERROR2("k_getuid: calloc(%d,%d)", nf, sizeof(struct file));

  xf = xfile;
  nfile = 0;

  while ((addr != 0) && (nfile < nf))
  {
    /*----------------------------------------------------
     * Read in linked list of pages of file table entries.
     *----------------------------------------------------
     */

    if (!getbuf(addr, xf, file_page_size, "file[]"))
      return -1;

    nfile += FILESPPG;
	xf += FILESPPG;

    if (!getbuf(addr + PAGESIZE - sizeof (addr), &addr, sizeof(addr), "&file"))
      return -1;
  }

  /* -------------------- TCP PCB LIST -------------------- */
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;

  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
  sockp = getlist(&tcb, faddr, fport, laddr, lport);

  if (!sockp)
    return -1;

  /*
  ** Locate the file descriptor that has the socket in question
  ** open so that we can get the 'ucred' information
  */
  for (i = 0; i < nfile; i++)
  {
    if (xfile[i].f_count == 0)
      continue;

    if (xfile[i].f_type == DTYPE_SOCKET &&
	(struct socket *) xfile[i].f_data == sockp)
    {
      if (!getbuf(xfile[i].f_cred, &ucb, sizeof(ucb), "ucb"))
	return -1;

      *uid = ucb.cr_ruid;
      return 0;
    }
  }

  return -1;
}


