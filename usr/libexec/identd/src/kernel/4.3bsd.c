/*
** kernel/bsd43.c		Low level kernel access functions for 4.3BSD
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 March 1993
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include "kvm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/socketvar.h>

#define KERNEL

#include <sys/file.h>

#if 0

#include <sys/time.h>
#include <sys/vmmac.h>
#include <sys/proc.h>
#include <sys/dir.h>

#include <fcntl.h>

#ifdef DIRBLKSIZ
#  undef DIRBLKSIZ
#endif
#include <sys/user.h>

#include <sys/wait.h>

#else

#include <sys/dir.h>	/* tmk */
#include <sys/user.h>	/* tmk */
/*#include <sys/time.h>
*/
#include <sys/vmmac.h>
#include <sys/proc.h>
/*#include <sys/dir.h>
*/
#include <fcntl.h>
  
#ifdef DIRBLKSIZ
#  undef DIRBLKSIZ
#endif
/*#include <sys/user.h>
*/
#endif

#ifdef KERNEL
#undef KERNEL
#endif

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

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#define N_FILE 0  
#define N_NFILE 1
#define N_TCB 2

  { "_file" },
  { "_nfile" },
  { "_tcb" },
  { "" }
};

static kvm_t *kd;

static struct file *xfile;
static int nfile;

static struct inpcb tcb;


int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if (!(kd = kvm_open(path_unix, path_kmem, NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");
  
  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) != 0)
    ERROR("main: kvm_nlist");

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
  struct proc *pp;
  struct user *up;
  int offset;
  struct socket *sockp;
  int i;
  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
  if (!getbuf(nl[N_NFILE].n_value, &nfile, sizeof(nfile), "nfile"))
    return -1;
  
  if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file"))
    return -1;

  xfile = (struct file *) calloc(nfile, sizeof(struct file));
  if (!xfile)
    ERROR2("k_getuid: calloc(%d,%d)", nfile, sizeof(struct file));
  
  if (!getbuf(addr, xfile, sizeof(struct file)*nfile, "file[]"))
    return -1;
  
  /* -------------------- TCP PCB LIST -------------------- */
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;
  
  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
  sockp = getlist(&tcb, faddr, fport, laddr, lport);
  
  if (!sockp)
    return -1;

  /* -------------------- SCAN PROCESS TABLE ------------------- */
  if (kvm_setproc(kd) < 0)
    ERROR("k_getuid: kvm_setproc() failed");
  
  while (pp = kvm_nextproc(kd))
  {
    if (pp->p_stat == 0)
      continue;

    if (pp->p_stat == SZOMB)
      continue;

    /* ------------------- GET U_AREA FOR PROCESS ----------------- */
    if ((up = kvm_getu(kd, pp)) == NULL)
      ERROR1("k_getuid: kvm_getu() failed for process %d", pp->p_pid);

    /* ------------------- SCAN FILE TABLE ------------------------ */
    for (i = up->u_lastfile; i >= 0; i--)
    {
      if (up->u_ofile[i] == 0)
	continue;

      offset = up->u_ofile[i] - (struct file *) addr;
      if (xfile[offset].f_type == DTYPE_SOCKET && 
	  (struct socket *) xfile[offset].f_data == sockp)
      {
#ifdef TAHOE
	*uid = up->u_ruid;
#else
	*uid = pp->p_ruid;
#endif
	return 0;
      } 		/* if */
    } 			/* for -- ofiles */
  }   			/* for -- proc */
  
  return -1;
}

