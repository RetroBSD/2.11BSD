/*
** kernel/irix4.c             Kernel access functions to retrieve user number
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 March 1993
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
**
** Last update: 28 June 1993
**
** Modified by Christopher Kranz, Princeton University to work with suid
** root programs.  The method for which one descends through the kernel
** process structures was borrowed from lsof 2.10 written by Victor A. Abell,
** Purdue Research Foundation.
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

#include <sys/proc.h>
#include <sys/syssgi.h>

#define _KERNEL

#include <sys/file.h>

#include <sys/inode.h>

#include <fcntl.h>

#include <sys/user.h>
#include <sys/wait.h>
  
#undef _KERNEL

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

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
#define N_V    1
#define N_TCB  2
#define N_PROC 3
 
  { "file" },
  { "v" },
  { "tcb" },
  { "proc" },
  { "" }
};

static kvm_t *kd;

static struct file *xfile;
static int nfile;

static struct var v;

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
  struct socket *sockp;
  int i;
  struct inode inode;
  long paddr;
  struct proc *pp;
  struct proc ps;
  long pa;
  int px;
  int nofiles;
  struct user *up;
  char *uu;
  size_t uul;
  struct file **fp;
  struct file f;

  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
  if (!getbuf(nl[N_V].n_value, &v, sizeof(v), "v"))
    return -1;
  
  nfile = v.v_file;
  addr = nl[N_FILE].n_value;
  
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
  if ( (paddr = nl[N_PROC].n_value) == NULL ) {
    if (debug_flag && syslog_flag)
      syslog(LOG_DEBUG, "k_getuid:  paddr == NULL");
    return -1;
  }

  paddr &= 0x7fffffff;

  uul = (size_t) (sizeof(struct user)
      + (v.v_nofiles * sizeof(struct file *)));

  if ( (uu=(char *)malloc(uul)) == (char *)NULL ) 
    return -1;

  fp = (struct file **)(uu + sizeof(struct user));
  up = (struct user *)uu;

  for (pp=&ps, px=0 ; px < v.v_proc ; px++) {
    pa = paddr + (long)(px * sizeof(struct proc));

    if ( !getbuf(pa, (char *)&ps, sizeof(ps), "proc") )
      continue;

    if ( pp->p_stat == 0 || pp->p_stat == SZOMB )
      continue;
  
    /* ------------------- GET U_AREA FOR PROCESS ----------------- */
    if ((i=syssgi(SGI_RDUBLK, pp->p_pid, uu, uul)) < sizeof(struct user))
      continue;

    /* ------------------- SCAN FILE TABLE ------------------------ */
    if (i <= sizeof(struct user)
    ||  ((long)up->u_ofile - UADDR) != sizeof(struct user))
      nofiles = 0;
    else
      nofiles = (i - sizeof(struct user)) / sizeof(struct file *);

    for (i = 0 ; i < nofiles ; i++) {
      if (fp[i] == NULL)
        break;

      if (!getbuf(fp[i], &f, sizeof(f), "file"))
        return -1;

      if ( f.f_count == 0 )
        continue;

      if (!getbuf(f.f_inode, &inode, sizeof(inode), "inode"))
        return -1;

      if ((inode.i_ftype & IFMT) == IFCHR && soc_fsptr(&inode) == sockp) {
        *uid = up->u_ruid;
        return 0;
      }
    } /* scan file table */
  }  /* scan process table */
  return -1;
}
