/*
 * 12 Apr 96 - Written by Paul Szabo <psz@maths.usyd.edu.au>
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <signal.h>
#include <setjmp.h>
#include <apollo/base.h>
#include <apollo/name.h>	/* Not really needed */
#include <apollo/pgm.h>

#include "identd.h"
#include "error.h"


#define USE_SAFE_ADDR


#define BUFLEN 256 /* buffer length, 80 would probably be enough */


/* These are from #include <limits.h>, but is only with #ifdef _INCLUDE_SYS5_SOURCE */
#define	PID_MAX		30000		/* max value for a process ID */
#define	UID_MAX		65535		/* max value for a user or group ID */

typedef unsigned int tcp_$netaddr_t;
/* An address of 129.78.69.2 is 0x814e4502 as int,
   or [0] is 0x81, [1] is 0x4e, [2] is 0x45, [3] is 0x02 as char array */

typedef struct {	/* network status structure */
/* Note the mismatches between /sys/ins/tcp.ins.{pas,c}, both dated 3 May 89 */
    unsigned short	nsnd;		/* # send bufs allocated */	/* Always 0x239c */
    unsigned short	nrcv;		/* # receive bufs allocated */	/* Either 0x239c, or maybe 0x2000, 0x8000 or 0xc000 */
    unsigned short	ssize;		/* # bufs on send buffer */	/* Stuff in queue, sndQ */
    unsigned short	rsize;		/* # bufs on receive buffer */	/* Stuff in queue, rcvQ */
    unsigned short	xstat;		/* network status word */
    unsigned short	state;		/* state of this connection */
    unsigned short	iostate;
    unsigned short	flags;		/* misc. flags (see below) */	/* See where??? 0x0002 tcp, 0x0220 udp, 0x0804 ip */
    unsigned short	lport;		/* local port */
    unsigned short	rport;		/* foreign port */
    tcp_$netaddr_t	raddr;		/* foreign address */
    tcp_$netaddr_t	laddr;		/* local address */
    unsigned short	options;	/* corresponds to ucb uc_options */
    unsigned short	pad1;		/* Padding for alignment */
} tcp_$netstate_t;

typedef char enum {
    tcp_$CLOSED,
    tcp_$LISTEN,
    tcp_$SYN_SENT,
    tcp_$state_0x03,
    tcp_$state_0x04,
    tcp_$ESTABLISHED,
    tcp_$state_0x06,
    tcp_$state_0x07,
    tcp_$TIME_WAIT,
    tcp_$CLOSE_WAIT,
    tcp_$state_0x0a,
    tcp_$CLOSING
} tcp_$tcpstate_t;

typedef struct {	/* Complete state of a connection */
/*
 * Both /sys/ins/tcp.ins.{pas,c} have 8-character ifname (so mis-alignment at ifunit);
 * /sys/ins/tcp.ins.pas has process UID instead of pgrp.
 */
    tcp_$netstate_t	cstate;
    tcp_$netaddr_t	route;		/* Gateway address, if any */
    char		*tcb;		/* The tcb, if any, for this connection */
    tcp_$tcpstate_t	tstate;		/* State of tcp connection, if any */
    char		ifname[9];	/* Name of network interface */
    pinteger		ifunit;		/* Unit number of device */
    pinteger		pr_num;		/* Raw protocol number, if any */
    uid_$t		pgrp #attribute[aligned(1)]; /* Process group (??) */
} tcp_$constate_t;

typedef tcp_$constate_t *tcp_$constate_p;

typedef short enum {
    tcp_$memstats,
    tcp_$netstats,
    tcp_$ifcbs,
    tcp_$tcbs,
    tcp_$hosts,
    tcp_$gateway,
    tcp_$all_conns
} tcp_$info_key_t;

#define MAXCON 0x0080	/* Why 128??? This is what netstat uses.
			   What happens if there are more connections? */

void extern tcp_$get_server_info (
    tcp_$info_key_t	&key,
    tcp_$constate_p	&state_p,
    short		&statesize,
    short		&max_con,
    short		*num_con,
    status_$t		*status);


typedef struct {
  uid_$t pers;
  uid_$t proj;
  uid_$t org ;
  uid_$t subs;
  long   node;
  } acl_$sid;


#define proc2_$aname_len 32
typedef char proc2_$aname_t[proc2_$aname_len];	/* accounting name */

typedef short proc2_$state_t;
typedef struct {
	uid_$t		stack_uid;	/* uid of user stack			*/
	linteger	stack_base;	/* base address of user stack		*/
	proc2_$state_t	state;		/* ready, waiting, etc.			*/
	pinteger	usr;		/* user sr				*/
	linteger	upc;		/* user pc				*/
	linteger	usp;		/* user stack pointer			*/
	linteger	usb;		/* user sb ptr (A6)			*/
	time_$clock_t	cpu_total;	/* cumulative cpu used by process	*/
	pinteger	priority;	/* process priority			*/
	acl_$sid	sid;		/* sid of process			*/
	uid_$t		pgroup;		/* process group uid			*/
	boolean		is_server;	/* process is a server			*/
	boolean		pad;
	pinteger	min_pri;	/* min priority of process (1..16)	*/
	pinteger	max_pri;	/* max priority of process (1..16)	*/
	linteger	exec_faults;	/* execute faults			*/
	linteger	data_faults;	/* data faults				*/
	linteger	disk_pageIO;	/* disk paging				*/
	linteger	net_pageIO;	/* net paging				*/
	uid_$t		orig_pgroup;	/* original process group		*/
	pinteger	orig_upgid;	/* original unix process group		*/
	pinteger	unix_PID;	/* Unix process ID			*/
	pinteger	unix_PPID;	/* Unix Process Parent ID		*/
	pinteger	unix_PGID;	/* Unix Process Group ID		*/
	pinteger	udpid;		/* unix debugger id			*/
	pinteger	asid;		/* address space id			*/
	uid_$t		acct_uid;	/* uid of command being run		*/
	pinteger	acct_len;	/* length of command name		*/
	proc2_$aname_t	acct_name;	/* name of the invoked command		*/
	pinteger	proc_len;	/* length of process name		*/
	proc2_$aname_t	proc_name;	/* process name				*/
	uid_$t		tty_uid;	/* uid of controlling terminal		*/
	pinteger	pad2;
	time_$clock_t	cpu_time;	/* total CPU time used			*/
	pinteger	pad3;
	linteger	user_ticks;	/* ticks executing in user space	*/
	linteger	system_ticks;	/* ticks executing in system space	*/
	linteger	tick_size;	/* size of a tick in microseconds	*/
} proc2_$info_t;

extern void proc2_$upid_to_uid (
	short		&upid,	/* convert unix pid to puid */
	uid_$t		*p2_uid,
	status_$t	*status
	);

extern void proc2_$get_info(
	uid_$t		&p2_uid,
	proc2_$info_t	*info,
	pinteger	&info_buf_len,	/* size of info buffer (bytes) */
	status_$t	*status
	);


/*
 * Converts SID to string: <person.group.org.nodeID> or maybe
 * <person.group.org.nodeID.subsys>. Writes the string
 * without adding a null byte: zero the buffer first.
 */
extern void rgyc_$sid2text (
	acl_$sid	&sid,
	char		*buffer,
	status_$t	*status
	);

/* Surely we could use rgyc_$pgo_uid_to_name instead */

typedef int acl_$unix_id;

/*
 * Converts UID to UNIX number, for pers, proj or org.
 */
extern void rgyc_$pgo_uid_to_num (
	int	&domain,	/* convert: 0: pers, 1: proj, 2: org */
	uid_$t	&uid,		/* UID to convert */
        short	*unknown,	/* set to 0 for pers or proj, 0 for org (why??) */
	acl_$unix_id *num,	/* UNIX number */
	status_$t *status);


typedef short enum {	/* Returned by VALIDATE */
    name_$junk_flag,
    name_$wdir_flag,
    name_$ndir_flag,
    name_$node_flag,
    name_$root_flag,
    name_$node_data_flag,
    name_$cs_flag
} name_$namtyp_t;

/*
 * Do a gpath on an object UID
 */
extern void name_$gpath_lc (
	uid_$t		&inuid,
	name_$namtyp_t	&namtyp,
	pinteger	&maxpnamlen,
	name_$long_pname_t	pname,
	pinteger	*pnamlen,
	status_$t	*status);


extern void context_$set (
	uid_$t		&p2_uid,
	status_$t	*status);

extern void pgm_$context_get_args (
	short		*argument_count,
	pgm_$argv_ptr	*arg_vector_ptr,
	status_$t	*status);

/* Look up values in other process's address space */
extern void xpd_$locate_proc (
	uid_$t		&p2_uid,
	void		&address,
	int		zero[2],	/* Is this some UID? Why need to set to zero? */
	void		*value,
	int		*stat,		/* Why does always get set to 0x070000 ? Is it really a short? */
	status_$t	*status);


#ifdef USE_SAFE_ADDR
static jmp_buf jmpenv;

void my_fault_handler (int sig)
{
  _longjmp (jmpenv, 1);
  exit (1);	/* This should never be reached */
}

/*
 * Tests if object at addr, of length len, is readable.
 * Any way to do this easier??? Must I generate and trap an error???
 * Does not test each address, only endpoints of range.
 */
int testaddr (char addr[], long len)
{
  char	c1, c2;

  if (_setjmp (jmpenv)) {
    signal (11, SIG_DFL);
    return (0);
  }

  c1 = 0; c2 = 0;
  signal (11, my_fault_handler);
  c1 = addr[0];
  if (len > 0 && len < 100000) c2 = addr[len];
  signal (11, SIG_DFL);

  /*
   * While all we want is to look up c1,c2, we must actually use them,
   * otherwise the optimiser may eliminate the assignments.
   */
  return (((int)(c1|c2)&0x00ff) + 1024);
}
#endif /* USE_SAFE_ADDR */


#ifdef DEBUG
#define sysl(a,b)	printf(a, b); printf("\n");
#else /* DEBUG */
#ifdef STRONG_LOG
 /* This isn't worth the time for the procedure call, but if you want... */
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


struct UCB;
struct PCB;

struct	PCB
{
	struct PCB *self1;
	struct PCB *self2;
	struct PCB *next;
	struct PCB *prev;
	struct UCB *ucb;
} *sock_PCB;

struct	UCB
{
	struct UCB *next;
	struct UCB *prev;
	struct in_addr ucb_faddr;
	struct in_addr ucb_laddr;
	u_short ucb_fport;
	u_short ucb_lport;
	struct	PCB *pcb;
	void	*Unknown;	/* What is this for??? */
	char	Dummy[PIDOFF];	/* What goodies lurk within??? */
	short	PID;
} *sock_UCB;

/*
Known good values of PIDOFF:
 OS Version    PIDOFF
 10.2   m68k   0x46   (probably should be higher!)
 10.3   m68k   0xc9
 10.4   m68k   0xc9
 10.4.1 m68k   0xc9
 10.4.1 a88k   0x52   (probably should be higher!)
Some values should be higher to correctly allow for incoming
telnet connections (and find the running shell, not a PID of
zero or the PID of telnetd).
*/



int k_open()
{
  /* Apollos do not use this rubbish */
  return 0;
}



int k_getuid(struct in_addr *faddr, int fport,
	     struct in_addr *laddr, int lport,
	     int *uid
#ifdef ALLOW_FORMAT
	     , int *retpid
	     , char **retcmd
	     , char **retcmd_and_args
#endif
	    )
{
 int		pcb, ucb;
 short		pid;

 status_$t	status;

 tcp_$netaddr_t	tcp_ra, tcp_la;
 unsigned short	tcp_rp, tcp_lp;
 tcp_$constate_t tcp_state[MAXCON];
 short		tcp_ncon;
 long		tcp_i;

 uid_$t		p2_uid;
 proc2_$info_t	p2_info;

 int		domain;
 acl_$unix_id	unix_num;
 short		unknown;

#ifdef ALLOW_FORMAT
 static name_$long_pname_t	pname;
 pinteger	pnamlen;

 short		arg_count;
 pgm_$arg       *arg;
 pgm_$arg_ptr	*arg_ptr, *arg_ptr2;
 pgm_$argv_ptr	arg_vector_ptr;

 static name_$long_pname_t	cmdarg;
#endif

 static char	zapmess[BUFLEN];
 long		zaplevel;


 /* Do we need this extra check? */
 ZAP(fport<1 || fport>65535 || lport<1 || lport>65535, "port numbers out of range",0)
 tcp_rp = fport & 0xffff;
 tcp_lp = lport & 0xffff;

 /* Do we need this extra check? */
 ZAP(faddr->s_addr==0 || faddr->s_addr==-1 || laddr->s_addr==0 || laddr->s_addr==-1, "net addresses out of range",0)
 tcp_ra = faddr->s_addr;
 tcp_la = laddr->s_addr;

 zaplevel = 0;
 sprintf (zapmess, "no such TCP connection");


 tcp_$get_server_info (
   tcp_$all_conns,
   tcp_state,
   (short) sizeof(tcp_$constate_t),
   MAXCON,
   &tcp_ncon,
   &status);
 ZAP (status.all,"tcp_$get_server_info failed with status %08x",status.all)

 for (tcp_i=0; tcp_i<tcp_ncon; tcp_i++) {
#ifdef DEBUG
   printf ("\n");
   printf ("Connection %d (of %d):\n", tcp_i, tcp_ncon);
   printf ("cstate.nsnd       %6d (%04x)\n", tcp_state[tcp_i].cstate.nsnd&0xffff, tcp_state[tcp_i].cstate.nsnd&0xffff);
   printf ("cstate.nrcv       %6d (%04x)\n", tcp_state[tcp_i].cstate.nrcv&0xffff, tcp_state[tcp_i].cstate.nrcv&0xffff);
   printf ("cstate.ssize      %6d (%04x)\n", tcp_state[tcp_i].cstate.ssize&0xffff, tcp_state[tcp_i].cstate.ssize&0xffff);
   printf ("cstate.rsize      %6d (%04x)\n", tcp_state[tcp_i].cstate.rsize&0xffff, tcp_state[tcp_i].cstate.rsize&0xffff);
   printf ("cstate.xstat      %6d (%04x)\n", tcp_state[tcp_i].cstate.xstat&0xffff, tcp_state[tcp_i].cstate.xstat&0xffff);
   printf ("cstate.state      %6d (%04x)\n", tcp_state[tcp_i].cstate.state&0xffff, tcp_state[tcp_i].cstate.state&0xffff);
   printf ("cstate.iostate    %6d (%04x)\n", tcp_state[tcp_i].cstate.iostate&0xffff, tcp_state[tcp_i].cstate.iostate&0xffff);
   printf ("cstate.flags      %6d (%04x)\n", tcp_state[tcp_i].cstate.flags&0xffff, tcp_state[tcp_i].cstate.flags&0xffff);
   printf ("cstate.lport      %6d (%04x)\n", tcp_state[tcp_i].cstate.lport&0xffff, tcp_state[tcp_i].cstate.lport&0xffff);
   printf ("cstate.rport      %6d (%04x)\n", tcp_state[tcp_i].cstate.rport&0xffff, tcp_state[tcp_i].cstate.rport&0xffff);
   printf ("cstate.raddr      %08x\n", tcp_state[tcp_i].cstate.raddr);
   printf ("cstate.laddr      %08x\n", tcp_state[tcp_i].cstate.laddr);
   printf ("cstate.options    %6d (%04x)\n", tcp_state[tcp_i].cstate.options&0xffff, tcp_state[tcp_i].cstate.options&0xffff);
   printf ("cstate.pad1       %6d (%04x)\n", tcp_state[tcp_i].cstate.pad1&0xffff, tcp_state[tcp_i].cstate.pad1&0xffff);
   printf ("route             %08x\n", tcp_state[tcp_i].route);
   printf ("tcb               %08x\n", (int)tcp_state[tcp_i].tcb);
   printf ("tstate            %02x\n", tcp_state[tcp_i].tstate&0xff);
   printf ("ifname            %.9s\n", tcp_state[tcp_i].ifname);
   printf ("ifunit            %6d (%04x)\n", tcp_state[tcp_i].ifunit&0xffff, tcp_state[tcp_i].ifunit&0xffff);
   printf ("pr_num            %6d (%04x)\n", tcp_state[tcp_i].pr_num&0xffff, tcp_state[tcp_i].pr_num&0xffff);
   printf ("pgrp              %08x.%08x\n", tcp_state[tcp_i].pgrp.high, tcp_state[tcp_i].pgrp.low);
   printf ("\n");
#endif /* DEBUG */


   if (
      (tcp_state[tcp_i].cstate.raddr == tcp_ra &&
       tcp_state[tcp_i].cstate.laddr == tcp_la &&
       tcp_state[tcp_i].cstate.rport == tcp_rp &&
       tcp_state[tcp_i].cstate.lport == tcp_lp)) {
     /* What??? No sanity check of tcp_state[tcp_i].tstate ? */

     pcb = (int)tcp_state[tcp_i].tcb;

     /* We must not just ZAP on error, but keep message and loop through all lines/connections */

     SETZAP(!pcb, 11, "pcb is zero (closed?)",0,0,0)
#ifdef USE_SAFE_ADDR
     SETZAP(!testaddr((char*)pcb,sizeof(*sock_PCB)), 12, "pcb at %08x is unreadable",pcb,0,0)
#endif /* USE_SAFE_ADDR */
     sock_PCB = (struct PCB *) pcb;

#ifdef DEBUG
     printf ("PCB.self1         %08x\n", (int)sock_PCB->self1);
     printf ("PCB.self2         %08x\n", (int)sock_PCB->self2);
     printf ("PCB.next          %08x\n", (int)sock_PCB->next);
     printf ("PCB.prev          %08x\n", (int)sock_PCB->prev);
     printf ("PCB.ucb           %08x\n", (int)sock_PCB->ucb);
     printf ("\n");
#endif /* DEBUG */

     SETZAP(pcb!=(int)sock_PCB->self1, 13, "pcb at %d, but PCB.self1 at %d",pcb,(int)sock_PCB->self1,0)
     SETZAP(pcb!=(int)sock_PCB->self2, 14, "pcb at %d, but PCB.self2 at %d",pcb,(int)sock_PCB->self2,0)

     ucb = (int)sock_PCB->ucb;
     /* Cannot just test ucb==0: usually we get bogus, but non-null, ucb addresses. */
#ifdef USE_SAFE_ADDR
     SETZAP(!(ucb&0xffff0000), 21, "ucb is zero (closed?)",0,0,0)
     SETZAP(!testaddr((char*)ucb,sizeof(*sock_UCB)), 22, "ucb at %08x is unreadable",ucb,0,0)
#else /* USE_SAFE_ADDR */
     SETZAP(!(ucb&0xf0000000), 21, "ucb is zero (closed?)",0,0,0)
#endif /* USE_SAFE_ADDR */
     sock_UCB = (struct UCB *) ucb;

#ifdef DEBUG
     printf ("UCB.next          %08x\n", (int)sock_UCB->next);
     printf ("UCB.prev          %08x\n", (int)sock_UCB->prev);
     printf ("UCB.ucb_faddr     %08x\n", (int)sock_UCB->ucb_faddr.s_addr);
     printf ("UCB.ucb_laddr     %08x\n", (int)sock_UCB->ucb_laddr.s_addr);
     printf ("UCB.ucb_fport     %6d (%04x)\n", sock_UCB->ucb_fport&0xffff, sock_UCB->ucb_fport&0xffff);
     printf ("UCB.ucb_lport     %6d (%04x)\n", sock_UCB->ucb_lport&0xffff, sock_UCB->ucb_lport&0xffff);
     printf ("UCB.pcb           %08x\n", (int)sock_UCB->pcb);
     printf ("UCB.Unknown       %08x\n", (int)sock_UCB->Unknown);
     printf ("UCB.PID           %6d (%04x)\n", sock_UCB->PID&0xffff, sock_UCB->PID&0xffff);
     printf ("\n");
     {
       long	i;
       char	*cp;
       short	*sp;
       long	*lp, *qp;

       printf ("The Dummy array:\n");
       printf ("      index      byte         short                   long                quad\n");
       for (cp=sock_UCB->Dummy, i=0; i<256; i++, cp++) {
         sp = (short *) cp;
         lp = (long *)  cp;
         qp = (long *)  (cp + 4);
         printf ("%6d %04x %6d %02x %8d %04x %13d %08x   %08x.%08x\n", i,i,*cp,*cp&0xff,*sp,*sp&0xffff,*lp,*lp,*lp,*qp);
       }
     }
     printf ("\n");
#endif /* DEBUG */

     SETZAP(tcp_ra!=(int)sock_UCB->ucb_faddr.s_addr, 23, "Wanted faddr %08x, but found %08x",tcp_ra,(int)sock_UCB->ucb_faddr.s_addr,0)
     SETZAP(tcp_la!=(int)sock_UCB->ucb_laddr.s_addr, 24, "Wanted laddr %08x, but found %08x",tcp_la,(int)sock_UCB->ucb_laddr.s_addr,0)
     SETZAP(tcp_rp!=sock_UCB->ucb_fport, 25, "Wanted fport %d, but found %d",tcp_rp,sock_UCB->ucb_fport,0)
     SETZAP(tcp_lp!=sock_UCB->ucb_lport, 26, "Wanted lport %d, but found %d",tcp_lp,sock_UCB->ucb_lport,0)
     SETZAP(pcb!=(int)sock_UCB->pcb, 27, "pcb at %d, but UCB.pcb at %d",pcb,(int)sock_UCB->pcb,0)

     pid = sock_UCB->PID;
     SETZAP(pid<=0 || pid>PID_MAX, 28, "Bad PID %d in UCB",pid,0,0)

     proc2_$upid_to_uid (pid, &p2_uid, &status);
     SETZAP(status.all, 31, "proc2_$upid_to_uid failed for PID %d with status %08x", pid&0xffff, status.all, 0)

#ifdef DEBUG
     printf ("process uid       %08x.%08x\n", p2_uid.high, p2_uid.low);
     printf ("\n");
#endif /* DEBUG */

     proc2_$get_info (p2_uid, &p2_info, sizeof(p2_info), &status);
     SETZAP(status.all, 41, "proc2_$get_info failed on PID %d with status %08x", pid&0xffff, status.all, 0)

#ifdef DEBUG
     printf ("stack_uid         %08x.%08x\n", p2_info.stack_uid.high, p2_info.stack_uid.low);
     printf ("stack_base        %d\n", p2_info.stack_base);
     printf ("state             %08x\n", p2_info.state);
     printf ("usr               %d\n", p2_info.usr);
     printf ("upc               %d\n", p2_info.upc);
     printf ("usp               %d\n", p2_info.usp);
     printf ("usb               %d\n", p2_info.usb);
     printf ("cpu_total         %08x %04x\n", p2_info.cpu_total.high, p2_info.cpu_total.low);
     printf ("priority          %d\n", p2_info.priority);
     printf ("sid.pers          %08x.%08x\n", p2_info.sid.pers.high, p2_info.sid.pers.low);
     printf ("sid.proj          %08x.%08x\n", p2_info.sid.proj.high, p2_info.sid.proj.low);
     printf ("sid.org           %08x.%08x\n", p2_info.sid.org.high, p2_info.sid.org.low);
     printf ("sid.subs          %08x.%08x\n", p2_info.sid.subs.high, p2_info.sid.subs.low);
     printf ("sid.node          %x\n", p2_info.sid.node);
     printf ("pgroup            %08x.%08x\n", p2_info.pgroup.high, p2_info.pgroup.low);
     printf ("is_server         %d\n", p2_info.is_server);
     printf ("min_pri           %d\n", p2_info.min_pri);
     printf ("max_pri           %d\n", p2_info.max_pri);
     printf ("exec_faults       %d\n", p2_info.exec_faults);
     printf ("data_faults       %d\n", p2_info.data_faults);
     printf ("disk_pageIO       %d\n", p2_info.disk_pageIO);
     printf ("net_pageIO        %d\n", p2_info.net_pageIO);
     printf ("orig_pgroup       %08x.%08x\n", p2_info.orig_pgroup.high, p2_info.orig_pgroup.low);
     printf ("orig_upgid        %d\n", p2_info.orig_upgid);
     printf ("unix_PID          %d\n", p2_info.unix_PID);
     printf ("unix_PPID         %d\n", p2_info.unix_PPID);
     printf ("unix_PGID         %d\n", p2_info.unix_PGID);
     printf ("udpid             %d\n", p2_info.udpid);
     printf ("asid              %d\n", p2_info.asid);
     printf ("acct_uid          %08x.%08x\n", p2_info.acct_uid.high, p2_info.acct_uid.low);
     printf ("acct_len          %d\n", p2_info.acct_len);
     printf ("acct_name         %.*s\n", p2_info.acct_len, p2_info.acct_name);
     printf ("proc_len          %d\n", p2_info.proc_len);
     printf ("proc_name         %.*s\n", p2_info.proc_len, p2_info.proc_name);
     printf ("tty_uid           %08x.%08x\n", p2_info.tty_uid.high, p2_info.tty_uid.low);
     printf ("cpu_time          %08x %04x\n", p2_info.cpu_time.high, p2_info.cpu_time.low);
     printf ("user_ticks        %d\n", p2_info.user_ticks);
     printf ("system_ticks      %d\n", p2_info.system_ticks);
     printf ("tick_size         %d\n", p2_info.tick_size);
     printf ("\n");
#endif /* DEBUG */

     SETZAP(p2_info.unix_PID!=pid, 42, "Looked up PID %d, but proc2_$get_info found %d", pid&0xffff, p2_info.unix_PID&0xffff, 0)

     /*
     for (p2_i = 0; p2_i < BUFLEN; p2_i++) retbuf[p2_i] = 0x00;
     rgyc_$sid2text (p2_info.sid, retbuf, &status);
     SETZAP(status.all, 51, "rgyc_$sid2text failed on person UID %08x.%08x with status %08x", p2_info.sid.pers.high, p2_info.sid.pers.low, status.all)
     */

     domain = 0;
     rgyc_$pgo_uid_to_num (domain, p2_info.sid.pers, &unknown, &unix_num, &status);
     SETZAP(status.all, 51, "rgyc_$pgo_uid_to_num failed on person UID %08x.%08x with status %08x", p2_info.sid.pers.high, p2_info.sid.pers.low, status.all)
     SETZAP(unix_num<0 || unix_num>UID_MAX, 52, "Bad unix UID %d from rgyc_$pgo_uid_to_num on person UID %08x.%08x",unix_num,p2_info.sid.pers.high,p2_info.sid.pers.low)

#ifdef DEBUG
     printf ("Unix UID:         %d\n", unix_num);
     printf ("Unknown:          %d\n", unknown);
     printf ("\n");
#endif /* DEBUG */

     *uid = unix_num;

#ifdef ALLOW_FORMAT
     *retpid = pid;

     name_$gpath_lc (p2_info.acct_uid, name_$junk_flag, name_$long_pnamlen_max, pname, &pnamlen, &status);
     /*
      * We can fail this with binaries removed,
      * or hard-linked and removed from original location.
      * So we fail gracefully...
     SETZAP(status.all, 61, "name_$gpath_lc failed on object UID %08x.%08x with status %08x", p2_info.acct_uid.high, p2_info.acct_uid.low, status.all)
     */
     if (status.all) {
       pnamlen = p2_info.acct_len;
       if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
       sprintf (pname, "%.*s (UID=%08x.%08x --> status=%08x)", pnamlen, p2_info.acct_name, p2_info.acct_uid.high, p2_info.acct_uid.low, status.all);
       pnamlen = strlen(pname);
     }
     else if (pnamlen < 1 || pnamlen > name_$long_pnamlen_max) {
       pnamlen = p2_info.acct_len;
       if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
       sprintf (pname, "%.*s (UID=%08x.%08x --> bad return length)", pnamlen, p2_info.acct_name, p2_info.acct_uid.high, p2_info.acct_uid.low);
       pnamlen = strlen(pname);
     }
     pname[pnamlen] = 0;

#ifdef DEBUG
     printf ("Command length:   %d\n", pnamlen);
     printf ("Command name:     %s\n", pname);
     printf ("\n");
#endif /* DEBUG */

     *retcmd = pname;

     context_$set (p2_uid, &status);
     SETZAP(status.all, 71, "context_$set failed on process UID %08x.%08x with status %08x", p2_uid.high, p2_uid.low, status.all)

     pgm_$context_get_args (&arg_count, &arg_vector_ptr, &status);
     /*
      * We can fail this for processes we do not 'own'.
      * So we fail gracefully...
     SETZAP(status.all, 81, "pgm_$context_get_args failed on process UID %08x.%08x with status %08x", p2_uid.high, p2_uid.low, status.all)
     */
     if (status.all) {
       pnamlen = p2_info.acct_len;
       if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
       sprintf (cmdarg, "%.*s (pgm_$context_get_args --> status=%08x)", pnamlen, p2_info.acct_name, status.all);
       pnamlen = strlen(cmdarg);
     }
     else {
       short i, j;

       int k1[2];
       void *k2;
       int k3;

       if (arg_count > 0) {
#ifdef DEBUG
         printf ("Argument count:   %d\n", arg_count);
#endif /* DEBUG */
         pnamlen = 0;
         arg_ptr = arg_vector_ptr;
         for (i = 0; i< arg_count; i++, arg_ptr++) {
           /*
           ** For 'own' argument would simply have
           **   arg = *arg_ptr;
           ** But need to de-reference arg_ptr in context of other process,
           ** and then need to de-reference the arg pointer itself.
           */
           k1[0] = 0; k1[1] = 0; k3 = 0;
           xpd_$locate_proc (p2_uid, (void)arg_ptr, k1, &k2, &k3, &status);
           if (status.all) {
             pnamlen = p2_info.acct_len;
             if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
             sprintf (cmdarg, "%.*s (xpd_$locate_proc --> status=%08x)", pnamlen, p2_info.acct_name, status.all);
             pnamlen = strlen(cmdarg);
             break;
           }
           if (k3 != 0x070000) {
             pnamlen = p2_info.acct_len;
             if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
             sprintf (cmdarg, "%.*s (xpd_$locate_proc --> k3=%08x)", pnamlen, p2_info.acct_name, k3);
             pnamlen = strlen(cmdarg);
             break;
           }
           arg_ptr2 = k2;
           arg = *arg_ptr2;
           k1[0] = 0; k1[1] = 0; k3 = 0;
           xpd_$locate_proc (p2_uid, (void)arg, k1, &k2, &k3, &status);
           if (status.all) {
             pnamlen = p2_info.acct_len;
             if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
             sprintf (cmdarg, "%.*s (xpd_$locate_proc --> status=%08x)", pnamlen, p2_info.acct_name, status.all);
             pnamlen = strlen(cmdarg);
             break;
           }
           if (k3 != 0x070000) {
             pnamlen = p2_info.acct_len;
             if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
             sprintf (cmdarg, "%.*s (xpd_$locate_proc --> k3=%08x)", pnamlen, p2_info.acct_name, k3);
             pnamlen = strlen(cmdarg);
             break;
           }
           arg = k2;
           if (pnamlen > 0) { sprintf (&cmdarg[pnamlen], " "); pnamlen++; }
           j = arg->len;
           if (j + pnamlen > name_$long_pnamlen_max) j = name_$long_pnamlen_max - pnamlen;
           sprintf (&cmdarg[pnamlen], "%.*s", j, arg->chars);
           pnamlen += strlen (&cmdarg[pnamlen]);
           if (pnamlen+10 > name_$long_pnamlen_max) break;
         }
       }
       else {
         pnamlen = p2_info.acct_len;
         if (pnamlen > proc2_$aname_len) pnamlen = proc2_$aname_len;
         sprintf (cmdarg, "%.*s (pgm_$context_get_args --> no args)", pnamlen, p2_info.acct_name);
         pnamlen = strlen(cmdarg);
       }
     }

#ifdef DEBUG
     printf ("Cmd+args length:  %d\n", pnamlen);
     printf ("Command+args:     %s\n", cmdarg);
     printf ("\n");
#endif /* DEBUG */

     *retcmd_and_args = cmdarg;


/*
 * Do we need to set context back to our own with
 *  context_$current (what arguments?)   or
 *  context_$set (our_own_Proc-UID)      ?
*/

/*
 * Could we figure out environment with ev_$context_read_var_entry ?
 * Current (working) and home (naming) directories with name_$read_dirs_p ?
*/


#endif

     return (0);
   }
 }

 ZAP(1,zapmess,0)
 /* return (-1); */
}

