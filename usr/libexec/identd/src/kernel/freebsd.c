/*
** kernel/freebsd.c	   Low level kernel access functions for FreeBSD 2.x
**
** This program is in the public domain and may be used freely by anyone
** who wants to.
**
** Last update: 11 Aug 1996
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <paths.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include <kvm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/socket.h>

#include <sys/queue.h>
#include <sys/uio.h>

#include <sys/socketvar.h>

#define KERNEL

#include <sys/file.h>

#undef KERNEL

#include <fcntl.h>

#include <sys/time.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <sys/filedesc.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include <pwd.h>

#include "identd.h"
#include "error.h"

#ifdef INPLOOKUP_SETLOCAL
#define	_HAVE_OLD_INPCB
#endif

extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#define N_TCB 0
  { "_tcb" },
#define N_BTEXT 1
  { "_btext" },
  { "" }
};

static kvm_t *kd;

static int nfile;

#ifdef _HAVE_OLD_INPCB
static struct inpcb tcb;
#else
static struct inpcbhead tcb;
#endif

int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if ((kd = (kvm_t *)kvm_openfiles(path_unix, path_kmem, NULL, O_RDONLY, NULL)) == NULL)
    ERROR("main: kvm_open");

  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) < 0)
    ERROR("main: kvm_nlist");

  return 0;
}


/*
** Get a piece of kernel memory with error handling.
** Returns 1 if call succeeded, else 0 (zero).
*/
static int getbuf(addr, buf, len, what)
  unsigned long addr;
  char *buf;
  unsigned int len;
  char *what;
{
    if (addr < nl[N_BTEXT].n_value ||		/* Overkill.. */
	addr >= (unsigned long)0xFFC00000 ||
	(addr + len) < nl[N_BTEXT].n_value ||
	(addr + len) >= (unsigned long)0xFFC00000)
    {
	if (syslog_flag)
	    syslog(LOG_ERR, "getbuf: bad address (%08x not in %08x-0xFFC00000) - %s",
		   addr, nl[N_BTEXT].n_value, what);
	return 0;
    }
    
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
#ifdef _HAVE_OLD_INPCB
    getlist(pcbp, faddr, fport, laddr, lport)
  struct inpcb *pcbp;
#else
    getlist(pcbhead, faddr, fport, laddr, lport)
  struct inpcbhead *pcbhead;
#endif
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
#ifdef _HAVE_OLD_INPCB
  struct inpcb *head;
#else
  struct inpcb *head, pcbp;
#endif

#ifdef _HAVE_OLD_INPCB
  if (!pcbp)
    return NULL;
#else
  head = pcbhead->lh_first;
  if (!head)
    return NULL;
#endif


#ifdef _HAVE_OLD_INPCB
  head = pcbp->inp_prev;
#endif
#ifdef _HAVE_OLD_INPCB
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
#else
  do
  {
    if (!getbuf((long) head, &pcbp, sizeof(struct inpcb), "tcblist"))
       break;
    if (pcbp.inp_faddr.s_addr == faddr->s_addr &&
	pcbp.inp_fport        == fport &&
	pcbp.inp_lport        == lport )
	return(pcbp.inp_socket);
    head = pcbp.inp_list.le_next;
  } while (head != NULL);
#endif

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
  struct kinfo_proc	*kp;
  int			nentries;

  if ((kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nentries)) == NULL)
  {
    ERROR("k_getuid: kvm_getprocs");
    return -1;
  }

  /* -------------------- TCP PCB LIST -------------------- */
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;

#ifdef _HAVE_OLD_INPCB
  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
#endif
  sockp = getlist(&tcb, faddr, fport, laddr, lport);

  if (!sockp)
    return -1;

  /*
  ** Locate the file descriptor that has the socket in question
  ** open so that we can get the 'ucred' information
  */
  for (i = 0; i < nentries; i++)
  {
    if(kp[i].kp_proc.p_fd != NULL)
    {
      int		j;
      struct filedesc	pfd;
      struct file	**ofiles;
      struct file	ofile;

      if(!getbuf(kp[i].kp_proc.p_fd, &pfd, sizeof(pfd), "pfd"))
        return(-1);

      ofiles = (struct file **) malloc(pfd.fd_nfiles * sizeof(struct file *));
      if (!ofiles) {
	  ERROR("k_getuid: ofiles malloc failed");
	  return(-1);
      }
      
      if(!getbuf(pfd.fd_ofiles, ofiles,
                 pfd.fd_nfiles * sizeof(struct file *), "ofiles"))
      {
        free(ofiles);
        return(-1);
      }

      for(j = 0; j < pfd.fd_nfiles; j ++)
      {
	  if (!ofiles) {
	      ERROR("k_getuid: ofiles malloc failed");
	      return(-1);
	  }
	  
	  if(!getbuf(ofiles[j], &ofile, sizeof(struct file), "ofile"))
	  {
	      free(ofiles);
	      return(-1);
	  }

        if(ofile.f_count == 0)
	  continue;

        if(ofile.f_type == DTYPE_SOCKET &&
           (struct socket *) ofile.f_data == sockp)
	{
          struct pcred	pc;

          if(!getbuf(kp[i].kp_proc.p_cred, &pc, sizeof(pc), "pcred"))
            return(-1);

          *uid = pc.p_ruid;
          free(ofiles);
	  return 0;
	}
      }

      free(ofiles);
    }
  }

  return -1;
}
