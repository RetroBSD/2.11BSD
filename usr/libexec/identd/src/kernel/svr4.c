/*
** kernel/svr4.c                    SVR4 specific kernel access functions
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 March 1993
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#define STRNET

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#  include "kvm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/socketvar.h>

#define KERNEL

#include <sys/file.h>

#include <fcntl.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stream.h>
#include <sys/stat.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/mkdev.h>
#define ucred cred

#include <sys/wait.h>
  
#undef KERNEL

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#ifndef ATTSVR4
#include <netinet/in_pcb.h>
#endif

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
#ifndef ATTSVR4
      
#define N_FILE  0
#define N_TCB 1
      
  { "file" },
  { "tcb" },

#else                   /* AT&T's hacked version below */

#define N_FILE  0
#define N_TCPD 1
#define N_TCPD_CNT 2

  { "file" },
  { "tcpd" },
  { "tcpd_cnt" },
#endif			/* ATTSVR4 */
  
  { "" }
};

static kvm_t *kd;

#ifndef ATTSVR4
static struct inpcb tcb;
#endif

#ifdef ATTSVR4
static struct tcpd tcpd;	/* we will read this one at a time */
static int tcpd_cnt;	        /* # of tcpd's we have */
#endif


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
static short
    getlist(pcbp, faddr, fport, laddr, lport)
#ifdef ATTSVR4
  struct tcpd *pcbp;
#else
  struct inpcb *pcbp;
#endif
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
#ifdef ATTSVR4
  struct tcpcb *head;
  struct tcpcb tcpcb;
  int i;
  int address_of_tcpcb;
#else  
  struct inpcb *head;
#endif

  if (!pcbp)
    return -1;

#ifdef ATTSVR4
  i = 0;	/* because we have already read 1 we inc later of the struct */
  do
  {
    /* don't ask be why (HACK) but the forth element that td_tcpcb
    ** points to contains the address of the tcpcb
    ** the struct is struct msgb which is a bunch of pointers
    ** to arrays (in <sys/stream.h> and the fouth element of the
    ** struct seems to always have the address...there is no comments
    ** in the .h files to indict why this is, and since I don't have
    ** source I don't know why....I found this info using crash looking
    ** around the structure tcpd, using the command 'od -x address count'
    **
    ** I assume that this would change....but it seems to work on AT&T SVR4.0
    ** Rel. 2.1
    **
    ** Bradley E. Smith <brad@bradley.bradley.edu>
    */
    if (!getbuf( (long) pcbp->td_tcpcb + (3 * sizeof(int)),
		&address_of_tcpcb, sizeof(int), "&address_of_tcpcb") )
    {
      return -1;
    }
    
    if (!getbuf( address_of_tcpcb, &tcpcb, sizeof(tcpcb) , "&tcpcb"))
    {
      return -1;
    }
    
    i++;	/* for the read */
    if (!tcpcb.t_laddr || !tcpcb.t_faddr) /* skip unused entrys */
	continue;
    
    /* have to convert these babies */
    if ( tcpcb.t_faddr == htonl(faddr->s_addr) &&
	 tcpcb.t_laddr == htonl(laddr->s_addr) &&
	 tcpcb.t_fport == htons(fport) &&
	 tcpcb.t_lport == htons(lport) )

      return pcbp->td_dev;
    
    /* if we get here we need to read the tcpd followed by tcpcb, but
    ** first we check if we are at the end of the tcpd struct
    */
  } while ( (i != tcpd_cnt) &&
	   getbuf((long) (nl[N_TCPD].n_value + ( i * sizeof(struct tcpd)) ),
		  pcbp, sizeof(struct tcpd), "tcpdlist")  );
  
#else
  
  head = pcbp->inp_prev;
  do 
  {
    if ( pcbp->inp_faddr.s_addr == faddr->s_addr &&
	 pcbp->inp_laddr.s_addr == laddr->s_addr &&
	 pcbp->inp_fport        == fport &&
	 pcbp->inp_lport        == lport )
      return pcbp->inp_minor;
  } while (pcbp->inp_next != head &&
	   getbuf((long) pcbp->inp_next,
		  pcbp,
		  sizeof(struct inpcb),
		  "tcblist"));
#endif /* not ATTSVR4 */

  return -1;
}



/*
** Return the user number for the connection owner in the int pointed
** at in the uid argument.
*/
int k_getuid(faddr, fport, laddr, lport, uid)
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
{
  long addr;
  struct stat s;
  short tcpmajor;	/* Major number of the tcp device */
  short sockp;		/* Really a device minor number */
  struct file *filehead, file;
  struct ucred ucb;

  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
  if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file"))
    return -1;

  /* we just need to determine the major number of the tcp module, which
     is the minor number of the tcp clone device /dev/tcp */
  if (stat("/dev/tcp", &s))
    ERROR("stat(\"/dev/tcp\")");
  
  tcpmajor = minor(s.st_rdev);

  /* -------------------- TCP PCB LIST -------------------- */
#ifdef ATTSVR4
  /* get the number of tcpd structs together */
  if (!getbuf(nl[N_TCPD_CNT].n_value, &tcpd_cnt,
	      sizeof(tcpd_cnt), "&tcpd_cnt"))
	return -1;
  
  if (!getbuf(nl[N_TCPD].n_value, &tcpd, sizeof(tcpd), "&tcpd"))
	return -1;
  
  sockp = getlist(&tcpd, faddr, fport, laddr, lport);
#else
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;
  
  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
  sockp = getlist(&tcb, faddr, fport, laddr, lport);
#endif
  
  if (sockp < 0)
    return -1;


  /*
  * Get the pointer to the head of the file list
  */
  if (!getbuf(nl[N_FILE].n_value, &filehead, sizeof(struct file *)))
    return -1;
  
  file.f_next = filehead;
  do
  {
    struct vnode vnode;
 
    /*
    ** Grab a file record
    */
    if (!getbuf((long)file.f_next, &file, sizeof(struct file), "file"))
      return -1;

    /*
    ** Grab the vnode information to check whether this file is the
    ** correct major & minor char device
    */
    if (!getbuf((off_t)file.f_vnode, &vnode, sizeof(struct vnode), "vnode"))
      return -1;
    
    if ((vnode.v_type == VCHR) && (major(vnode.v_rdev) == tcpmajor) &&
	(minor(vnode.v_rdev) == sockp))
    {
      /*
      ** We've found it!
      */
      if (!getbuf((off_t)file.f_cred, &ucb, sizeof(struct cred), "cred"))
	return -1;
      
      *uid = ucb.cr_ruid;
      return 0;
    }
    
  } while (file.f_next != filehead);
  
  return -1;
}
