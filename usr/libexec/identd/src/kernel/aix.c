/*
** 941026: Harlan Stenn <harlan@pfcs.com> Hacked mercilessly from:
**
**  aixident -- AIX identification daemon
**  version 1.0
**
**  Copyright 1992 by Charles M. Hannum.
**
**  Permission is granted to copy, modify, and use this program in any way,
**  so long as the above copyright notice, this permission notice, and the
**  warranty disclaimer below remain on all copies, and are unaltered.
**
**  aixident is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <sys/types.h>

#ifdef _AIX42
#if defined(__GNUC__)

typedef	long long	aligned_off64_t __attribute__ ((aligned (8)));
typedef	long long	aligned_offset_t  __attribute__ ((aligned (8)));
#define	off64_t		aligned_off64_t
#define	offset_t	aligned_offset_t

#endif
#endif

#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <syslog.h>

#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/vfs.h>
#include <net/route.h>
#include <netinet/in.h>
 
#if	defined(_AIX4)
#include <netinet/ip.h>
#endif	/* defined(_AIX4) */
 
#include <netinet/in_pcb.h>
#include <arpa/inet.h>

#define _KERNEL 1
#include <sys/file.h>
#undef  _KERNEL
#include <procinfo.h>
 
#if	defined(_AIX4)
#define	u_maxofile	U_maxofile
#define	u_ufd	U_ufd
#endif	/* defined(_AIX4) */

#include "identd.h"
#include "error.h"
#include "paths.h"

int kmem;

int kread ();

int k_open()
{
    if ((kmem = open (_PATH_KMEM, O_RDONLY)) == -1)
	ERROR("main: k_open");

    return 0;
}

/*
** Return the user number for the connection owner
*/
int k_getuid(faddr, fport, laddr, lport, uid
#ifdef ALLOW_FORMAT
	     , pid, cmd, cmd_and_args
#endif
 	     )
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
#ifdef ALLOW_FORMAT
  int *pid;
  char **cmd,**cmd_and_args;
#endif
{
  struct sockaddr_in foreign, local;
  int max_procs = 64,
      num_procs, fd;
  struct procinfo *procinfo;
  struct user user;
  struct file *filep, file;
  struct socket *socketp, socket;
  struct protosw *protoswp, protosw;
  struct domain *domainp, domain;
  struct inpcb *inpcbp, inpcb;
  struct passwd *passwd;
  static char argline[4096],progname[256];
  char *cp;
 
  while ((procinfo = (struct procinfo *)
		     malloc ((size_t) (max_procs * sizeof (*procinfo)))) &&
         (num_procs = getproc (procinfo, max_procs,
			       sizeof (*procinfo))) == -1 &&
	 errno == ENOSPC) {
    max_procs <<= 1;
    free (procinfo);
  }

  if (! procinfo) {
    if (syslog_flag)
      (void) syslog (LOG_ERR, "out of memory allocating %ld procinfo structs\n",
		     max_procs);
    return -1;
  }

  for (; num_procs; num_procs--, procinfo++) {

    if (procinfo->pi_stat == 0 || procinfo->pi_stat == SZOMB)
      continue;

    if (getuser (procinfo, sizeof (*procinfo), &user, sizeof (user)))
      continue;

    for (fd = 0; fd < user.u_maxofile; fd++) {

      if (! (filep = user.u_ufd[fd].fp))
	continue;

      if (kread ((off_t) filep, (char *) &file, sizeof (file))) {
	if (syslog_flag)
          (void) syslog (LOG_ERR, "can't read file struct from %#x",
		         (unsigned) filep);
	return -1;
      }

      if (file.f_type != DTYPE_SOCKET)
	continue;

      if (! (socketp = (struct socket *) file.f_data))
	continue;

      if (kread ((off_t) socketp, (char *) &socket, sizeof (socket))) {
	if (syslog_flag)
          (void) syslog (LOG_ERR, "can't read socket struct from %#x",
			 (unsigned) socketp);
	return -1;
      }

      if (! (protoswp = socket.so_proto))
	continue;

      if (kread ((off_t) protoswp, (char *) &protosw, sizeof (protosw))) {
	if (syslog_flag)
	  (void) syslog (LOG_ERR, "can't read protosw struct from %#x",
			 (unsigned) protoswp);
	return -1;
      }

      if (protosw.pr_protocol != IPPROTO_TCP)
	continue;

      if (! (domainp = protosw.pr_domain))
	continue;

      if (kread ((off_t) domainp, (char *) &domain, sizeof (domain))) {
	if (syslog_flag)
	  (void) syslog (LOG_ERR, "can't read domain struct from %#x",
			 (unsigned) domainp);
	return -1;
      }

      if (domain.dom_family != AF_INET)
	continue;

      if (! (inpcbp = (struct inpcb *) socket.so_pcb))
	continue;

      if (kread ((off_t) inpcbp, (char *) &inpcb, sizeof (inpcb))) {
	if (syslog_flag)
	  (void) syslog (LOG_ERR, "can't read inpcb struct from %#x",
			 (unsigned) inpcbp);
	return -1;
      }

      if (socketp != inpcb.inp_socket)
	continue;

      if (inpcb.inp_faddr.s_addr != faddr->s_addr ||
	  inpcb.inp_fport != fport ||
	  inpcb.inp_laddr.s_addr != laddr->s_addr ||
	  inpcb.inp_lport != lport)
	continue;

      *uid = procinfo->pi_uid;
#ifdef ALLOW_FORMAT
      *pid = procinfo->pi_pid;
      bzero(argline,sizeof(argline));
      getargs(procinfo,sizeof(*procinfo),argline,sizeof(argline));
      strcpy(progname,argline);
      *cmd = progname;
      for (cp = argline; *cp != '\0';) {
	cp += strlen(cp);
	*cp++ = ' ';
      }
      *cmd_and_args = argline;
#endif

      return 0;
    }
  }

  return -1;
}

int
kread (addr, buf, len)
  off_t addr;
  char *buf;
  int len;
{
  int br;

  if (lseek (kmem, addr, L_SET) == (off_t) -1)
    return (-1);

  br = read(kmem, buf, len);

  return ((br == len) ? 0 : 1);
}


