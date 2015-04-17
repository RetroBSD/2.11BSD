/*
** kernel/next_mach.c		Low level kernel access functions for 4.3BSD
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
#include <sys/types.h>
#include <sys/param.h>
#include <netdb.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>

#include "kvm.h"
#include "paths.h"

#include <sys/socket.h>
#include <sys/socketvar.h>

#include <sys/ioctl.h>

#ifndef NeXT31
#  define KERNEL
#  define KERNEL_FEATURES 
#else
#  define KERNEL_FILE
#endif

#include <sys/file.h>

#ifndef NeXT31
#  undef KERNEL
#  undef KERNEL_FEATURES 
#else
#  undef KERNEL_FILE
#endif

#include <sys/user.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#define N_FILE 0  
#define N_NFILE 1
#define N_TCB 2

   { {"_file_list"} },
   { {"_max_file"} },
   { {"_tcb"} },
   { {""} }
};

static kvm_t *kd;


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



/**
***
*** Traverse the inpcb list until a match is found 
*** Returns NULL if no match.
*** 
**/

static
struct socket *getlist(struct inpcb *pcbp,
		       struct in_addr *faddr,
		       int fport,
		       struct in_addr *laddr,
		       int lport)
{
   struct inpcb *head;
  
   if (!pcbp)
      return NULL;		/* Someone gave us a duff one here */
  
   head = pcbp->inp_prev;
   do {
      if (pcbp->inp_faddr.s_addr == faddr->s_addr &&
	  pcbp->inp_laddr.s_addr == laddr->s_addr &&
	  pcbp->inp_fport        == fport &&
	  pcbp->inp_lport        == lport )
	 return pcbp->inp_socket;

   } while (pcbp->inp_next != head &&
	    getbuf((long) pcbp->inp_next, 
                   pcbp, 
                   sizeof(struct inpcb), 
		   "tcblist"));

   return NULL;			/* Not found */
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
   static struct ucred ucb;
   struct socket *sockp;
   int nfile;
   struct inpcb tcb;		/*  */
   struct file file_entry;
   void * addr;

   /* -------------------- TCP PCB LIST -------------------- */
   if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
      return -1;
      
   tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
   sockp = getlist(&tcb, faddr, fport, laddr, lport);

   if (!sockp)
      return -1;

   /* -------------------- FILE DESCRIPTOR TABLE -------------------- */
   /* So now we hit the fun Mach kernel structures */
   if (!getbuf(nl[N_FILE].n_value, &addr, sizeof(addr), "&file_table"))
      return -1;
      
   /* We only use nfile as a failsafe in case something goes wrong! */
   if (!getbuf(nl[N_NFILE].n_value, &nfile, sizeof(nfile), "nfile"))
      return -1;
      
   file_entry.links.next = addr;
   /* ------------------- SCAN FILE TABLE ------------------------ */
   do {
      if (!getbuf((unsigned long) file_entry.links.next, &file_entry, 
	          sizeof(file_entry), "struct file"))
         return -1;
	 
      if (file_entry.f_count &&
	  file_entry.f_type == DTYPE_SOCKET) {
	 if ((void *) file_entry.f_data == (void *) sockp) {
	    if (!getbuf((unsigned long) file_entry.f_cred, 
	                &ucb, sizeof(ucb), "ucb"))
	       return -1;
	       
	    *uid = ucb.cr_ruid;
	    return 0;
	 }
      }
   } while ((file_entry.links.next != addr) && (--nfile));
   return -1;
}
