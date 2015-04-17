/*
** kernel/ptx4.c                    PTX4 specific kernel access functions
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

#  include "kvm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/socketvar.h>

#define _INKERNEL
#include <sys/file.h>
#include <sys/var.h>
#undef _INKERNEL

#include <fcntl.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stream.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/sysmacros.h>

#include <sys/wait.h>
  
#undef _KMEMUSER

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/route.h>

#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/ip_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#define N_TCP_NHDRS 0
#define N_TCP_HASH 1
#define N_FILE  2
#define N_V  3
      
  { "tcp_nhdrs" },
  { "tcp_hash" },
  { "file" },
  { "v" },
  { "" }
};

static kvm_t *kd;

static struct inpcb_hdr *k_tcp_hash;
static struct inpcb_hdr *tcp_hash;
static int tcp_nhdrs;
static struct var v;

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
    getlist(faddr, fport, laddr, lport)
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
  int i;
  struct inpcb cur_pcb;
  struct inpcb *inp;

  for (i = 0; i < tcp_nhdrs; i++) {
    inp = tcp_hash[i].inph_next;
    /* Linked list on this hash chain ends when points to hash header */
    while (inp != (struct inpcb *) &k_tcp_hash[i]) {
      /* Can we get this PCB ? */
      if (!getbuf((long) inp, &cur_pcb, sizeof(struct inpcb), "tcblist"))
	break;
      
      if ( cur_pcb.inp_faddr == faddr->s_addr &&
	   cur_pcb.inp_laddr == laddr->s_addr &&
	   cur_pcb.inp_fport        == fport &&
	   cur_pcb.inp_lport        == lport )
	return cur_pcb.inp_minordev;
      inp = cur_pcb.inp_next;
    }
  }

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
  int nfile;
  struct ucred ucb;
  int i;

  
  /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
  if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file"))
    return -1;

  /* we just need to determine the major number of the tcp module, which
     is the minor number of the tcp clone device /dev/tcp */
  if (stat("/dev/tcp", &s))
    ERROR("stat(\"/dev/tcp\")");
  
  tcpmajor = minor(s.st_rdev);

  /* -------------------- TCP PCB LIST -------------------- */
  /* Get count of TCP hash headers */
  if (!getbuf(nl[N_TCP_NHDRS].n_value,
      &tcp_nhdrs, sizeof(tcp_nhdrs), "tcp_nhdrs"))
    return(-1);
  if ((tcp_hash = calloc(tcp_nhdrs, sizeof(struct inpcb_hdr))) == NULL)
    return(-1);
  k_tcp_hash = (struct inpcb_hdr *) nl[N_TCP_HASH].n_value;
  if (!getbuf(nl[N_TCP_HASH].n_value,
      tcp_hash, tcp_nhdrs * sizeof(struct inpcb_hdr), "tcp_hash"))
    return -1;
  
  sockp = getlist(faddr, fport, laddr, lport);
  
  if (sockp == -1)
    return -1;


  /*
  * Get the pointer to the head of the file list
  */
  if (!getbuf(nl[N_FILE].n_value, &filehead, sizeof(struct file *)))
    return -1;

  /* Get size of file array */
  if (!getbuf(nl[N_V].n_value, &v, sizeof(struct var )))
    return -1;

  nfile = v.v_file;
  
  for (i = 0; i < nfile ; i++) {
    struct vnode vnode;
 
    /*
    ** Grab a file record
    */
    if (!getbuf((long) &filehead[i], &file, sizeof(struct file), "file"))
      return -1;

    /* In use ? */
    if (file.f_count == 0)
      continue;

    /*
    ** Grab the vnode information to check whether this file is the
    ** correct major & minor char device
    */
    /* Extra check */
    if (!file.f_vnode)
      continue;
    if (!getbuf((off_t)file.f_vnode, &vnode, sizeof(struct vnode), "vnode"))
      return -1;
    
    if ((vnode.v_type == VCHR) && (major(vnode.v_rdev) == tcpmajor) &&
	(minor(vnode.v_rdev) == sockp))
    {
      /*
      ** We've found it!
      */
      if (!getbuf((off_t)file.f_cred, &ucb, sizeof(struct ucred), "cred"))
	return -1;
      
      *uid = ucb.cr_ruid;
      return 0;
    }
    
  }
  
  return -1;
}
