/*
** kernel/other.c	Low level kernel access functions for a number of
**			similar Unixes (all more or less based of 4.2BSD)
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

#ifdef HAVE_KVM
#  include <kvm.h>
#else
#  include "kvm.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#ifdef __convex__
#  define _KERN_UTIL
#endif

#include <sys/socketvar.h>

#ifdef __convex__
#  undef _KERN_UTIL
#endif

#define KERNEL

#if defined(SUNOS35) || defined(__alpha)
#  define nfile SOME_OTHER_VARIABLE_NAME
#  define _KERNEL
#endif

#include <sys/file.h>

#if defined(SUNOS35) || defined(__alpha)
#  undef nfile
#  undef _KERNEL
#endif

#include <fcntl.h>

#if defined(ultrix)
#  include <sys/dir.h>
#  undef KERNEL
#endif

#if defined(sequent)
#  undef KERNEL
#endif

#include <sys/user.h>
#include <sys/wait.h>

#ifdef KERNEL  
#  undef KERNEL
#endif

#ifdef __convex__
#  include <sys/mbuf.h>
#endif

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#ifdef __alpha
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#endif
 
#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/ip_var.h>
 
#ifdef __alpha
#include <netinet/tcpip.h>
#endif
 
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#ifdef MIPS
#  include <sysv/sys/var.h>
extern int errno;
#endif

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#if (defined(ultrix) && defined(mips))
   /* Ultrix on DEC's MIPS machines */

#define N_FILE  0  
#define N_NFILE 1
#define N_TCB   2

  { "file" },
  { "nfile" },
  { "tcb" },
  
#else
#ifdef MIPS		/* MIPS RISC/OS */

#define N_FILE 0  
#define N_V    1
#define N_TCB  2

  { "_file" },
  { "_v" },
  { "_tcb" },

#else

#define N_FILE 0  
#define N_NFILE 1
#define N_TCB 2

  { "_file" },
  { "_nfile" },
  { "_tcb" },

#endif
#endif
  { "" }
};

static kvm_t *kd;

static struct file *xfile;

#ifndef ultrix
static int nfile;
#endif

#if defined(MIPS)
static struct var v;
#endif

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
#ifdef __alpha
  caddr_t addr;
#else
  long addr;
#endif
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
#ifdef __alpha
	   getbuf((caddr_t) pcbp->inp_next,
#else
	   getbuf((long) pcbp->inp_next,
#endif
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
#ifdef __alpha
  caddr_t addr;
#else
  long addr;
#endif
  struct socket *sockp;
  int i;
  struct ucred ucb;
  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
#if defined(MIPS)
  if (!getbuf(nl[N_V].n_value, &v, sizeof(v), "v"))
    return -1;
  
  nfile = v.v_file;
  addr = nl[N_FILE].n_value;
#else /* not MIPS */
  if (!getbuf(nl[N_NFILE].n_value, &nfile, sizeof(nfile), "nfile"))
    return -1;

  if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file"))
    return -1;
#endif
  
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
#ifdef __convex__
      *uid = xfile[i].f_cred.cr_uid;
#else
      if (!getbuf(xfile[i].f_cred, &ucb, sizeof(ucb), "ucb"))
	return -1;
      
      *uid = ucb.cr_ruid;
#endif

      return 0;
    }
  }
  
  return -1;
}

