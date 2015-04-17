/*
 * 16 Apr 96 - Changes by Paul Szabo <psz@maths.usyd.edu.au>
 *
 * May 23, 1994 - Modified by Allan E. Johannesen (aej@wpi.edu) from code
 * kindly provided by Digital during the Beta test of Digital Alpha AXP OSF/1
 * 3.0 when WPI discovered that the file structures had changed.  Prior to 3.0,
 * OSF/1 ident support had only needed 64-bit modifications to the `other.c'
 * kernel routine (those mods done at WPI during the initial OSF/1 Beta tests).
 *
 * NOTE:
 *   This tool is NOT part of DEC OSF/1 and is NOT a supported product.
 *
 * BASED ON code provided by
 *   Aju John, UEG, Digital Equipment Corp. (ZK3) Nashua, NH.
 *
 * The following is an **unsupported** tool. Digital Equipment Corporation
 * makes no representations about the suitability of the software described
 * herein for any purpose. It is provided "as is" without express or implied
 * warranty.
 *
 * BASED ON:
 *  PADS program by Stephen Carpenter, UK UNIX Support, Digital Equipment Corp.
 * */

/*
 * Multiple, almost simultaneous identd requests were causing a
 * 'kernel panic' crash on our 2-CPU 2100 server running OSF 3.2C
 * (though no such problems were seen on Alphastations). We were
 * initially told to try patch 158. When that did not cure the
 * problem, Digital service came up with the following on 9 May 96:
 *
 * > The following came from an outage in the states about the same thing.
 * > 
 * > The active program was "identd" which is a freeware
 * > program that identifies the user who is opening a port on the system.
 * > 
 * > Careful analysis shows that the identd program is causing the crash by
 * > seeking to an invalid address in /dev/kmem. The program, identd reads
 * > through /dev/kmem looking for open files that are sockets, which then send
 * > pertainent information back to the other end. In this case, the socket has
 * > gone away before the read has been performed thereby causing a panic.
 * > 
 * > identd reading /dev/kmem, causing a fault on the  kernel stack guard pages
 * > which in turn cause the kernel to panic. To fix  this problem, set the
 * > following in /etc/sysconfigtab:
 * > 
 * > vm:
 * >         kernel-stack-guard-pages = 0
 * > 
 * > could you try this and see if that fixes your problem.
 *
 * This has fixed our problem, though I am worried what other
 * effects this may have.
 *
 * These crashes occured while we had k_open in the parent process (in
 * src/identd.c): the file pointers would have been all over the place.
 * Now that we have k_open in the child process (in src/parse.c) we may
 * not have a crash anyway...
 */

#include <stdlib.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#define SHOW_UTT
#include <sys/user.h>
#define KERNEL_FILE
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include <alloca.h>

#include "identd.h"
#include "error.h"
#include "kvm.h"

/* The following is in <sys/proc.h>, but only if _KERNEL is defined */
struct pid_entry {
        pid_t   pe_pid;                /* process id */
        int     pe_generation;         /* checks for struct re-use */
        struct proc *pe_proc;          /* pointer to this pid's proc */
        union {
                struct pid_entry *peu_nxt;     /* next entry in free list */
                struct {
                        int     peus_pgrp;     /* pid is pgrp leader? */
                        int     peus_sess;     /* pid is session leader? */
                } peu_s;
        } pe_un;
};

/* The following is in <sys/proc.h>, but only if _KERNEL is defined */
#define	PID_INVALID(pid) ((pid_t)(pid) < 0 || (pid_t)(pid) > (pid_t)PID_MAX)

/* The following is in <sys/ucred.h>, but only if _KERNEL is defined */
#define	INVALID_UID(uid) ((uid_t)(uid) < 0 || (uid_t)(uid) > (uid_t)UID_MAX)


#ifdef NOFILE_IN_U		/* more than 64 open files per process ? */
#  define OFILE_EXTEND
#else
#  define NOFILE_IN_U NOFILE
#endif


#define BUFLEN 1024 /* buffer length */

#ifdef DEBUG
#define sysl(a,b)	printf(a, b); printf("\n");
#else /* DEBUG */
#ifdef STRONG_LOG
#define sysl(a,b)	if (syslog_flag) { syslog(LOG_INFO, a, b); }
#else
#define sysl(a,b)
#endif
#endif /* DEBUG */


#define ZAP(x,fmt,err) \
  if (x) \
   { \
    sysl(fmt, err) \
    return (-1); \
   }

#ifdef DEBUG
#define SETZAP(x,lev,fmt,err1,err2,err3) \
  if (x) \
   { \
    printf ("zap to level %d: ", lev); printf (fmt, err1, err2, err3); printf ("\n"); \
    if (lev > zaplevel) \
     { \
      sprintf (zapmess, fmt, err1, err2, err3); \
      zaplevel = lev; \
      printf ("zap level set to %d\n", zaplevel); \
     } \
    continue; \
   }
#else /* DEBUG */
#define SETZAP(x,lev,fmt,err1,err2,err3) \
  if (x) \
   { \
    if (lev > zaplevel && syslog_flag) \
     { \
      sprintf (zapmess, fmt, err1, err2, err3); \
      zaplevel = lev; \
     } \
    continue; \
   }
#endif /* DEBUG */


struct nlist name_list[] = {
#define N_PIDTAB 0
  { "_pidtab" },
#define N_NPID 1
  { "_npid" },
  { 0 }
};

static kvm_t *kd;


int k_open()
{
  if (!(kd = kvm_open("/vmunix", "/dev/kmem" , NULL, O_RDONLY, NULL)))
  {
    if (syslog_flag) syslog(LOG_ERR, "kvm_open call failed");
    return 1;
  }

  if (kvm_nlist(kd, name_list) != 0)
  {
    if (syslog_flag) syslog(LOG_ERR, "kvm_nlist call failed");
    return 1;
  }

  return 0;
}


int k_getuid(struct in_addr *faddr, int fport,
	     struct in_addr *laddr, int lport,
	     int *uid
#ifdef ALLOW_FORMAT
	     , int *pid
	     , char **cmd
	     , char **cmd_and_args
#endif
	    )
{
  off_t pidtab_base;	/* Start address of the process table */
  int npid;		/* Number of processes in the process table */

  struct proc_plus_utask {
    struct proc The_Proc;
    struct utask Proc_Utask;
  } pu;

  struct pid_entry *the_pid_entry;
  struct file **ofile_table_extension, open_file;
  int index, index1;

#define the_proc pu.The_Proc
#define proc_utask pu.Proc_Utask
#define p_i the_pid_entry[index]
#define f_s proc_utask.uu_file_state

  static char	zapmess[BUFLEN];
  long		zaplevel;

#ifdef ALLOW_FORMAT
  static char	cmdbuf[BUFLEN];
  static char	c_abuf[BUFLEN];
  int		blen;
#endif


/* Just to save us some typing: we ALWAYS test return from kvm_read */
#define goodr(k,addr,buf,len) (kvm_read(k,addr,buf,len) == len)
#define badr(k,addr,buf,len)  (kvm_read(k,addr,buf,len) != len)


#ifdef OFILE_EXTEND
  /* Reserve space for the extended open file table of a process */
  ofile_table_extension = (struct file **) alloca((getdtablesize ())
				 * sizeof(struct file *));
#endif


  /* Do we need these extra checks? */
  ZAP(fport<1 || fport>65535 || lport<1 || lport>65535, "port numbers out of range",0)
  ZAP(faddr->s_addr==0 || faddr->s_addr==-1 || laddr->s_addr==0 || laddr->s_addr==-1, "net addresses out of range",0)

#ifdef DEBUG
  printf ("Looking up   faddr %08x  fport %d\n", faddr->s_addr, fport);
  printf ("             laddr %08x  lport %d\n", laddr->s_addr, lport);
#endif /* DEBUG */


  zaplevel = 0;
  sprintf (zapmess, "no such TCP connection");


  /* Find the start of the process table */
  ZAP(badr(kd, (off_t) name_list[N_PIDTAB].n_value, &pidtab_base, sizeof(pidtab_base)),
	"Cannot read pidtab_base",0)

  /* Find the size of the process table */
  ZAP(badr(kd, (off_t) name_list[N_NPID].n_value, &npid, sizeof(npid)), "Cannot read npid",0)

#if 0
  /* Sanity check */
  ZAP(npid < 1 || npid > 20000, "Bad npid of %d", npid)
#endif

/*
** To improve on the 20000 above:
**
** (method 1)
**
**   We observe that npid is 1024 or 4096.
**   (Are any other multiples of 1024 allowed?)
**   Accept these allowed values only.
**
** (method 2) THIS WOULD NOT WORK:
**
**   Change the declaration of name_list at the beginning to:
**
**   struct nlist name_list[] = {
**   #define N_PIDTAB 0
**     { "_pidtab" },
**   #define N_NPID 1
**     { "_npid" },
**   #define N_TASKMAX 2
**     { "task_max" },
**     { 0 }
**   };
**
**   then use
**
**   int taskmax;
**     kvm_read (kd, (off_t) name_list[N_TASKMAX].n_value, &taskmax, sizeof(taskmax));
**
**   and then replace 20000 by taskmax-1.
**
**   But then, we are still relying on values returned from kvm_read...
**   or put another way, we have no good way of checking taskmax.
**
**   This probably would not work anyway: our DEC 2100 should show around
**   1000 for taskmax (128 users), but npid returned is 4096.
*/

#ifdef DEBUG
  printf ("Number of processes: %d\n", npid);
#endif /* DEBUG */


  /* Read in the process structure */
  the_pid_entry = (struct pid_entry*)alloca(sizeof(struct pid_entry) * npid);
  ZAP(badr(kd, pidtab_base, the_pid_entry, sizeof(struct pid_entry) * npid),
	"Cannot read process structure",0)

  for (index = 0; index < npid; index++) {

    /* Sanity checks */
    /*
    This is a 'normal' thing:
    SETZAP(p_i.pe_proc == 0, 11, "Proc/utask pointer zero for proc slot %d",index,0,0)
    */
    if (p_i.pe_proc == 0) continue;
    SETZAP(PID_INVALID(p_i.pe_pid), 12, "Invalid PID %d for proc slot %d",p_i.pe_pid,index,0)

    /* Read in the proc and utask structs of the process */
    SETZAP(badr(kd, (off_t) p_i.pe_proc, &pu, sizeof(pu)), 13,
	"Cannot read proc/utask for proc slot %d (PID %d)",index,p_i.pe_pid,0)

    /* Sanity checks */
    SETZAP(p_i.pe_pid != the_proc.p_pid, 21,
	"Proc slot %d was PID %d, but has %d in proc/utask",index,p_i.pe_pid,the_proc.p_pid)
    SETZAP(INVALID_UID(the_proc.p_ruid), 22,
	"Invalid UID %d for proc slot %d (PID %d)",the_proc.p_ruid,index,p_i.pe_pid)

#ifdef DEBUG
    printf ("Looking at proc slot %d: PID %d, UID %d\n", index, the_proc.p_pid, the_proc.p_ruid);
#endif /* DEBUG */

    /* Sanity checks */
    if (f_s.uf_lastfile < 0) continue;
    SETZAP((f_s.uf_lastfile + 1) > getdtablesize(), 31,
	"Too many files used (%d) for proc slot %d (PID %d)",f_s.uf_lastfile,index,p_i.pe_pid)

#ifdef OFILE_EXTEND
    if (f_s.uf_lastfile >= NOFILE_IN_U) {
      /* Sanity checks */
      SETZAP(f_s.uf_of_count > (getdtablesize()), 32,
	"Too many (%d) OFILE_EXTEND files used for proc slot %d (PID %d)",f_s.uf_of_count,index,p_i.pe_pid)

#if 0 /* Disabled, 960811 Peter Eriksson <pen@lysator.liu.se< */
      SETZAP((f_s.uf_lastfile + 1) != (f_s.uf_of_count + NOFILE_IN_U), 33,
	"Files used %d, but %d OFILE_EXTEND ones for proc slot %d",f_s.uf_lastfile,f_s.uf_of_count,index)
#endif

      SETZAP(badr(kd, (off_t) f_s.uf_ofile_of, ofile_table_extension, f_s.uf_of_count * sizeof(struct file *)), 34,
	"Cannot read OFILE_EXTEND files for proc slot %d (PID %d)",index,p_i.pe_pid,0)
    }
#endif

#ifdef DEBUG
    printf ("proc slot %d uses %d files\n", index, f_s.uf_lastfile);
#endif /* DEBUG */

    for (index1 = 0; index1 <= f_s.uf_lastfile; index1++) {

      if (index1 < NOFILE_IN_U) {
	if (f_s.uf_ofile[index1] == (struct file *) NULL) continue;
	if (f_s.uf_ofile[index1] == (struct file *) -1)   continue;

	SETZAP(badr(kd, (off_t) f_s.uf_ofile[index1], &open_file, sizeof(open_file)), 41,
	  "Cannot read file %d for proc slot %d (PID %d)",index1,index,p_i.pe_pid)
      }
#ifdef OFILE_EXTEND
      else {
	if (ofile_table_extension[index1-NOFILE_IN_U] == (struct file *) NULL) continue;

	SETZAP(badr(kd,(off_t) ofile_table_extension[index1-NOFILE_IN_U], &open_file, sizeof(open_file)), 42,
	  "Cannot read OFILE_EXTEND file %d for proc slot %d (PID %d)",index1,index,p_i.pe_pid)
      }
#endif

#ifdef DEBUG
      printf ("Looking at proc slot %d, file %d\n", index, index1);
#endif /* DEBUG */

      if (open_file.f_type == DTYPE_SOCKET) {

	struct socket try_socket;
	struct inpcb try_pcb;

	SETZAP(badr(kd,(off_t) open_file.f_data, &try_socket,sizeof(try_socket)), 51,
	  "Cannot read socket of file %d for proc slot %d (PID %d)",index1,index,p_i.pe_pid)
	if (try_socket.so_pcb) {
	  SETZAP(badr(kd, (off_t) try_socket.so_pcb, &try_pcb,sizeof(try_pcb)), 52,
	    "Cannot read pcb of file %d for proc slot %d (PID %d)",index1,index,p_i.pe_pid)
	  if (try_pcb.inp_faddr.s_addr == faddr->s_addr &&
	      try_pcb.inp_laddr.s_addr == laddr->s_addr &&
	      try_pcb.inp_fport        == fport &&
	      try_pcb.inp_lport        == lport ) {

	    *uid = the_proc.p_ruid;

#ifdef ALLOW_FORMAT

	    *pid = the_proc.p_pid;

	    blen = strlen(proc_utask.uu_comm);
	    if (blen > BUFLEN-1) blen = BUFLEN-1;
	    sprintf(cmdbuf, "%.*s", blen, proc_utask.uu_comm);
	    *cmd = cmdbuf;

/*
How to use proc_utask.uu_arg_size and proc_utask.uu_argp ??
The value proc_utask.uu_arg_size seems to be the length of the
cmd+args string (including trailing null), but where is the string?

Neither of the following two ways work:
Way 1:
	    blen = strlen(proc_utask.uu_argp);
	    if (blen > BUFLEN-1) blen = BUFLEN-1;
	    sprintf(c_abuf, "%.*s", blen, proc_utask.uu_argp);

Way 2:
	    blen = proc_utask.uu_arg_size;
	    if (blen > BUFLEN-1) blen = BUFLEN-1;
	    SETZAP(badr(kd, (off_t) proc_utask.uu_argp,c_abuf,blen), 53, "Cannot read cmd args for proc slot %d (PID %d)",index,p_i.pe_pid,0)
	    c_abuf[blen] = 0;

Finally:
	    *cmd_and_args = c_abuf;
*/

/*
** Other stuff we could return:
**	    login_namep = proc_utask.uu_logname;
**	    envp  = proc_utask.uu_envp;
**	    n_env = proc_utask.uu_env_size;
*/

#endif

	    return (0);
	  };
	};
	continue; /* Is not this superfluous? We are at the end of the file loop anyway. */
      };
    };
  };
  ZAP(1,zapmess,0)
  return (-1);
}
