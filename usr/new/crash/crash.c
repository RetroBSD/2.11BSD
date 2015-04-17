/*
 *	U N I X   2 . 9 B S D   C R A S H   A N A L Y Z E R
 *
 *
 * Name -
 *	crash -- analyze post-mortem or active system image
 *
 * Author -
 *	Mike Muuss, JHU EE
 *
 * Synopsis of options -
 *
 *	-b	Brief mode; skip general display of processes
 *	-d	Crash dump contains swap image.  (default?)
 *	-v	Verbose; dump much information about each proc [future]
 *	-t	TTY structs to be dumpped
 *	-i	Incore inode table to be printed
 *	-c FILE	Provide non-standard file name for system image input
 *	-s FILE Provide non-standard symboltable input
 *	-u ADDR	Trace a process other than currently selected one
 *	-z	Interrupt Trace displayed
 *	APS	Print PS & PC at time of interupt
 *
 *
 *
 *	synopsis:	crash [aps] [-s sfile] [-c cfile]
 *
 *	It examines a dump of unix which it looks for in the file
 *	sysdump.  It prints out the contents of the general
 *	registers, the kernel stack and a traceback through the
 *	kernel stack.  If an aps is specified, the ps and pc at
 *	time of interrupt are also printed out.  The dump of the
 *	stack commences from a "reasonable" address and all addresses
 *	are relocated to virtual addresses by using the value of
 *	kdsa6 found in the dump.
 *	  If the -s argument is found the following argument is taken
 *	to be the name of a file, containing a symbol table which
 *	should be used in interpreting text addresses.  The default
 *	is "/unix".  If the -c argument is found, the following argument
 *	is taken to be the name of a file which should be used instead
 *	of the default.
 *
 *		R E V I S I O N   H I S T O R Y
 *
 *	??/??/??  MJM	More than I can tell
 *
 *	01/09/80  MJM	Added C-list printing for JHU/UNIX clists.
 *
 *	12/11/80 MJM+RSM Modified to use new MOUNT structure.
 *
 *	12/16/80  RSM	Ability to ask for text or data symbol on printout.
 *
 *	12/21/80  RSM	Added ability to name-list an overlay file.
 *
 *	10/07/81  RSM	Can now print longs in decimal format.
 *		I don't think this works--JCS
 *
 *	12/08/81 JCP+MJM The SYS Group
 *			Modified to work for Version 7 Vanilla UNIX
 *
 *	03/03/83  JCS	Modified for 2.81 kernel
 *		Another new mount structure
 *		Made stack traces work with SRI overlaid kernel
 *		New p_flag values, uses USIZE
 *		Dump of some network stats (broke tty dump--Ritchie
 *		compiler is a crock)
 *		Print longs in octal format.
 *
 *	2/10/84 GLS Modified for 2.9 kernel
 *		Made it work with MENLO_KOV
 *		Output 2.9 clists.
 *
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/file.h>
#include <sys/fs.h>
#include <sys/ioctl.h>

#define	USIZEB	(64*USIZE)

/*
 *-----The following three lines *must* remain together
 */
#include <sys/user.h>
struct	user	u;
char u_wasteXX[USIZEB-sizeof(struct user)];	/* STORAGE FOR KERNEL STACKS
						 * TO LOAD--must follow user.h
						 */
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/tty.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/msgbuf.h>
#include "crash.h"

struct	buf	bfreelist;
struct	proc	*proc;		/* NPROC */
struct	callout	*callout;	/* NCALL */

/*
 * Interrupt Tracing Strutures - The SYS Group
 */
#ifdef	INTRPT
/*
 * to preserve the integrity of the trace information, the pointer and data
 * area are fetched in the same read.  This means the following two lines
 * *MUST* appear in this order
 */
char	*itrptr;			/* pointer to next structure to use */
struct	itrace	itrace[100];		/* max size of trace area */
/* related kernel variables */
char	*itrstr;			/* start of tracing area */
char	*itrend;			/* end of tracing area */
#endif	INTRPT

/* Actual core allocation for tty structures */
int ndh11 = 0;
struct tty *dh11;	/* NDH11 */
int ndz11 = 0;
struct tty *dz11;	/* NDZ11 */
int nkl11 = 0;
struct tty *kl11;	/* NKL11 */


/* Global Variables */

struct msgbuf msgbuf;
char *panicstr;
unsigned saveps;	/* ps after trap */
long	dumplo;		/* Offset of swapspace on dump */
unsigned nbuf;		/* size of buffer cache */
int	ncallout;	/* max simultaneous callouts */
int	nfile;		/* Stored value of NFILE */
int	ninode;		/* Stored value of NINODE */
int	nproc;		/* Stored value of NPROC */
int	ntext;		/* Stored value of NTEXT */
int	nswap;		/* Number of swap blocks */
int	nmount;		/* number of mountable file systems */
int	nclist;		/* number of clists */
long	stime;		/* crash time */
long	bootime;	/* when system was booted */
unsigned bpaddr;	/* Location of actual buffers (clicks) */
dev_t	rootdev;	/* device of the root */
dev_t	swapdev;	/* swapping device */
dev_t	pipedev;	/* pipe device */
unsigned bsize;		/* size of buffers */
int	cputype;	/* type of cpu = 40, 44, 45, 60, or 70 */
struct	inode	*rootdir;	/* pointer to inode of root directory */
struct	proc	*runq;		/* head of linked list of running processes */
int	lbolt;			/* clock ticks since time was last updated */
int	mpid;			/* generic for unique process id's */
bool_t	runin;			/* set when sched wants to swap someone in */
bool_t	runout;			/* set when sched is out of work */
bool_t	runrun;			/* reschedule at next opportunity */
char	curpri;			/* p_pri of current process */
size_t	maxmem;			/* actual max memory per process */
size_t	physmem;		/* physical memory */
u_short	*lks;			/* pointer to clock device */
int	updlock;		/* lock for sync */
daddr_t	rablock;		/* block to be read ahead */
memaddr clststrt;		/* Location of actual clists (clicks) */
memaddr cfreebase;		/* base of clists in kernel */
unsigned clsize;		/* Size of clists */
short	ucbclist;		/* mapped out clist flag */

/*
 * V7 (Stone knives & bear skins memorial) Clist structures.  "cfree" conflicts
 * with calloc's cfree ...
 */
struct	cblock	*cfree_;	/* NCLIST */
struct	cblock	*cfreelist;
int cbad;

char	*subhead;			/* pntr to sub-heading */
int	line = 0;				/* current line number */
int	kmem;				/* Global FD of dump file */

int	overlay;			/* overlays found flag */
int	ovno, ovend;			/* current overlay number */
u_int		aova, aovd;		/* addresses of ova and ovd in core */
unsigned	ova[8], ovd[8];		/* overlay addresses and sizes */

/* Display table for Mounted Files */
struct display mnt_tab[] = {
	"\n\nDevice",	 (char *) &((struct mount *)0)->m_dev,	DEV, 0,
	"\tOn Inode",	 (char *) &((struct mount *)0)->m_inodp, OCT, 0,
	END
};

#ifdef	INTRPT
/* Display for Interrupt Tracing Feature (the SYS Group) */
struct	display	itr_buf[] {
	"",		&itrace[0].intpc,	OCT, 0,
	"",		&itrace[0].intpc,	TADDR, 0,
	"",		&itrace[0].intps,	OCT, 0,
	"",		&itrace[0].r0,		OCT, 0,
	"",		&itrace[0].r0,		TADDR, 0,
	"",		&itrace[0].savps,	OCT, 0,
	END
};
#endif	INTRPT

/* Display table for Callouts */
struct display cal_buf[] = {
	"\nfunc",	(char *) &((struct callout *)0)->c_func, OCT, 0,
	" ",		(char *) &((struct callout *)0)->c_func, TADDR, 0,
	"\targ",	(char *) &((struct callout *)0)->c_arg,	DEC, 0,
	"\ttime",	(char *) &((struct callout *)0)->c_time,DEC, 0,
	END
};


/* Display for C-list display */
struct display cl_buf[] = {
	": link",	(char *) &((struct cblock *)0)->c_next,	OCT, 0,
	"\tCh",		(char *) ((struct cblock *)0)->c_info,	CHARS, 0,
	END
};

int ttyflg(), ttystat();

/* Display for tty structures */
struct display tty_buf[] = {
	"\n\nMaj/Min",	(char *) &((struct tty *)0)->t_dev,	DEV, 0,
	"\nFlags",	(char *) &((struct tty *)0)->t_flags,	OCT, ttyflg,
	"\nAddr",	(char *) &((struct tty *)0)->t_addr,	OCT, 0,
	"",		(char *) &((struct tty *)0)->t_addr,	DADDR, 0,
	"\nDelct",	(char *) &((struct tty *)0)->t_delct,	HALFDEC, 0,
	"\nLine",	(char *) &((struct tty *)0)->t_line,	HALFDEC, 0,
	"\nCol",	(char *) &((struct tty *)0)->t_col,	HALFDEC, 0,
	"\nState",	(char *) &((struct tty *)0)->t_state,	OCT, ttystat,
	"\nIspeed",	(char *) &((struct tty *)0)->t_ispeed,	HALFDEC, 0,
	"\tOspeed",	(char *) &((struct tty *)0)->t_ospeed,	HALFDEC, 0,
	"\nRAW",	(char *) &((struct tty *)0)->t_rawq.c_cc,DEC, 0,
	"\tc_cf",	(char *) &((struct tty *)0)->t_rawq.c_cf,OCT, 0,
	"\tc_cl",	(char *) &((struct tty *)0)->t_rawq.c_cl,OCT, 0,
	"\nOUT",	(char *) &((struct tty *)0)->t_outq.c_cc,DEC, 0,
	"\tc_cf",	(char *) &((struct tty *)0)->t_outq.c_cf,OCT, 0,
	"\tc_cl",	(char *) &((struct tty *)0)->t_outq.c_cl,OCT, 0,
	"\nCAN",	(char *) &((struct tty *)0)->t_canq.c_cc,DEC, 0,
	"\tc_cf",	(char *) &((struct tty *)0)->t_canq.c_cf,OCT, 0,
	"\tc_cl",	(char *) &((struct tty *)0)->t_canq.c_cl,OCT, 0,
	END
};

/* Display table for Random Variables */
struct display rv_tab[] = {
	"\nRoot directory",	(char *) &rootdir,	OCT, 0,
	"\nLightningbolt",	(char *) &lbolt,		DEC, 0,
	"\nNext PID",		(char *) &mpid,		DEC, 0,
	"\n   runrun",		&runrun,	OCT, 0,
	"\n   runout",		&runout,	OCT, 0,
	"\n   runin",		&runin,		OCT, 0,
	"\n   curpri",		&curpri,	OCT, 0,
	"\n   maxmem",		(char *) &maxmem,	OCT, 0,
	"\n   physmem",		(char *) &physmem,	OCT, 0,
	"\nClock addr",		(char *) &lks,		OCT, 0,
	"\nRoot dev",		(char *) &rootdev,	DEV, 0,
	"\nSwap Dev",		(char *) &swapdev,	DEV, 0,
	"\nPipe Dev",		(char *) &pipedev,	DEV, 0,
	"\nSwap Size",		(char *) &nswap,		DEC, 0,
	"\nupdate() lock",		(char *) &updlock,	DEC, 0,
	"\nReadahead blk",	(char *) &rablock,	OCT, 0,
	"\n    Max procs",		(char *) &nproc,	DEC, 0,
	"\n    Max inodes",		(char *) &ninode,	DEC, 0,
	"\n    Max files",		(char *) &nfile,	DEC, 0,
	"\n    Max texts",		(char *) &ntext,	DEC, 0,
	"\n*cfreelist",		(char *) &cfreelist,	OCT, 0,
	"\nSystem Buffer start click",	(char *) &bpaddr,	OCT, 0,
	"\tNo. Buffers",	(char *) &nbuf,	DEC, 0,
	"\tSize",	(char *) &bsize,	OCT, 0,
	"\nClist start click",	(char *) &clststrt,		OCT, 0,
	"\nClist start address",(char *) &cfreebase,		OCT, 0,
	"\tSize of area",	(char *) &clsize,		DEC, 0,
	END
};

int pdrprint(), printuid(), printgid();

struct	display ov_tab[] = {
"\n\nThe Kernel Overlay Information and Variables:\n\t__ovno",
			(char *) &ovno,		DEC, 0,
	"\n\tOverlay #0 PAR",		(char *) &ova[0],	OCT, 0,
	"\tPDR",			(char *) &ovd[0],	OCT, pdrprint,
	"\n\tOverlay #1 PAR",		(char *) &ova[1],	OCT, 0,
	"\tPDR",			(char *) &ovd[1],	OCT, pdrprint,
	"\n\tOverlay #2 PAR",		(char *) &ova[2],	OCT, 0,
	"\tPDR",			(char *) &ovd[2],	OCT, pdrprint,
	"\n\tOverlay #3 PAR",		(char *) &ova[3],	OCT, 0,
	"\tPDR",			(char *) &ovd[3],	OCT, pdrprint,
	"\n\tOverlay #4 PAR",		(char *) &ova[4],	OCT, 0,
	"\tPDR",			(char *) &ovd[4],	OCT, pdrprint,
	"\n\tOverlay #5 PAR",		(char *) &ova[5],	OCT, 0,
	"\tPDR",			(char *) &ovd[5],	OCT, pdrprint,
	"\n\tOverlay #6 PAR",		(char *) &ova[6],	OCT, 0,
	"\tPDR",			(char *) &ovd[6],	OCT, pdrprint,
	"\n\tOverlay #7 PAR",		(char *) &ova[7],	OCT, 0,
	"\tPDR",			(char *) &ovd[7],	OCT, pdrprint,

	"\n\tOverlay #8 PAR",		(char *) &ova[8],	OCT, 0,
	"\tPDR",			(char *) &ovd[8],	OCT, pdrprint,
	"\n\tOverlay #9 PAR",		(char *) &ova[9],	OCT, 0,
	"\tPDR",			(char *) &ovd[9],	OCT, pdrprint,
	"\n\tOverlay #a PAR",		(char *) &ova[10],	OCT, 0,
	"\tPDR",			(char *) &ovd[10],	OCT, pdrprint,
	"\n\tOverlay #b PAR",		(char *) &ova[11],	OCT, 0,
	"\tPDR",			(char *) &ovd[11],	OCT, pdrprint,
	"\n\tOverlay #c PAR",		(char *) &ova[12],	OCT, 0,
	"\tPDR",			(char *) &ovd[12],	OCT, pdrprint,
	"\n\tOverlay #d PAR",		(char *) &ova[13],	OCT, 0,
	"\tPDR",			(char *) &ovd[13],	OCT, pdrprint,
	"\n\tOverlay #e PAR",		(char *) &ova[14],	OCT, 0,
	"\tPDR",			(char *) &ovd[14],	OCT, pdrprint,
	"\n\tOverlay #f PAR",		(char *) &ova[15],	OCT, 0,
	"\tPDR",			(char *) &ovd[15],	OCT, pdrprint,
	END
};

int bufflg();

struct display buf_tab[] = {
	"\nFlags",	(char *) &((struct buf *)0)->b_flags,	OCT, bufflg,
	"\tdev",	(char *) &((struct buf *)0)->b_dev,	DEV, 0,
	"\tbcount",	(char *) &((struct buf *)0)->b_bcount,	DEC, 0,
	"\nxmem",	&((struct buf *)0)->b_xmem,	ONEBYTE, 0,
	"\taddr",	(char *) &((struct buf *)0)->b_un.b_addr, OCT, 0,
	"\tblkno",	(char *) &((struct buf *)0)->b_blkno,	LONGDEC, 0,
	"\nerror",	&((struct buf *)0)->b_error,	ONEBYTE, 0,
	"\tresid",	(char *) &((struct buf *)0)->b_resid,	OCT, 0,
	END
};

int inoflg(), inomod();

struct display inode_tab[] = {
	"\ndev",	(char *) &((struct inode *)0)->i_dev,DEV, 0,
	"\tnumber",	(char *) &((struct inode *)0)->i_number,DEC,0,
	"\tcount",	(char *) &((struct inode *)0)->i_count,	DEC,0,
	"\tflag",	(char *) &((struct inode *)0)->i_flag,	OCT,inoflg,
	"\nnlink",	(char *) &((struct inode *)0)->i_nlink,	DEC,0,
	"\tuid",	(char *) &((struct inode *)0)->i_uid,	UDEC,printuid,
	"\tgid",	(char *) &((struct inode *)0)->i_gid,	UDEC,printgid,
	"\nmode",	(char *) NULL,	IGNORE, inomod,
	END
};

/* Display the USER Structure */
/*
 * note that on V7, the u.u_ar0 and u.u_qsav areas contain more than
 * they do on V6 UNIX
 *
 *	On V6	u.u_ar0[0]  = saved R5
 *		u.u_ar0[1]  = saved R6
 *
 *	On V7	u.u_ar0[0]  = saved R2
 *		u.u_ar0[1]  = saved R3
 *		u.u_ar0[2]  = saved R4
 *		u.u_ar0[3]  = saved R5
 *		u.u_ar0[4]  = saved R6 (stack pointer)
 *
 * for this reason, we declare the following labels, which must be used
 * anywhere the saved registers are referenced
 */
#define	R2	0	/* index of saved R2 */
#define	R3	1
#define	R4	2
#define	R5	3
#define	R6	4
#define	R1	5

struct display u_tab[] = {
	"\nsaved R5",	(char *) &(u.u_rsave.val[R5]),	OCT, 0,
	"\tsaved R6",	(char *) &(u.u_rsave.val[R6]),	OCT, 0,
	"\tsegflg",	&(u.u_segflg),			ONEBYTE, 0,
	"\terror",	&(u.u_error),			HALFDEC, 0,
	"\nuid",	(char *) &(u.u_uid),		UDEC, printuid,
	"\nsvuid",	(char *) &(u.u_svuid),		UDEC, printuid,
	"\nruid",	(char *) &(u.u_ruid),		UDEC, printuid,
	"\tsvgid",	(char *) &(u.u_svgid),		UDEC, printgid,
	"\trgid",	(char *) &(u.u_rgid),		UDEC, printgid,
	"\tgroups",	(char *) u.u_groups,		UDEC, printgid,
	"\nprocp",	(char *) &(u.u_procp),		OCT, 0,
	"\tbase",	(char *) &(u.u_base),		OCT, 0,
	"\tcount",	(char *) &(u.u_count),		DEC, 0,
	"\toff",	(char *) &(u.u_offset),		HEXL, 0,
	"\ncdir",	(char *) &(u.u_cdir),		OCT, 0,
	"\tdirp",	(char *) &(u.u_dirp),		OCT, 0,
	"\tpdir",	(char *) &(u.u_pdir),		OCT, 0,
	"\targ0",	(char *) &(u.u_arg[0]),		OCT, 0,
	"\narg1",	(char *) &(u.u_arg[1]),		OCT, 0,
	"\targ2",	(char *) &(u.u_arg[2]),		OCT, 0,
	"\targ3",	(char *) &(u.u_arg[3]),		OCT, 0,
	"\targ4",	(char *) &(u.u_arg[4]),		OCT, 0,
	"\narg5",	(char *) &(u.u_arg[5]),		OCT, 0,
	"\ttsize",	(char *) &(u.u_tsize),		DEC, 0,
	"\tdsize",	(char *) &(u.u_dsize),		DEC, 0,
	"\tssize",	(char *) &(u.u_ssize),		DEC, 0,
	"\nctty",	(char *) &(u.u_ttyp),		OCT, 0,
	"\tttydev",	(char *) &(u.u_ttyd),		DEV, 0,
	"\nsep",	&(u.u_sep),		ONEBYTE, 0,
	"\tar0",	(char *) &(u.u_ar0),	OCT, 0,
	END
};

int procflg(), prtsig();

/* Display table for Proc Structure */
struct display proc_tab[] = {
	"\nStat",	(char *) &((struct proc *)0)->p_stat,	ONEBYTE, 0,
	"\tFlags",	(char *) &((struct proc *)0)->p_flag,	HEX, procflg,
	"\nPri",	(char *) &((struct proc *)0)->p_pri,	HALFDEC, 0,
	"\tSig",	(char *) &((struct proc *)0)->p_sig,	HEXL, 0,
	"\tCursig",	(char *) &((struct proc *)0)->p_cursig,	HALFDEC, 0,
	"\tUid",	(char *) &((struct proc *)0)->p_uid,	DEC, printuid,
	"\nsigmask",	(char *) &((struct proc *)0)->p_sigmask,HEXL, prtsig,
	"\nsigignore",	(char *) &((struct proc *)0)->p_sigignore,HEXL, prtsig,
	"\nsigcatch",	(char *) &((struct proc *)0)->p_sigcatch,HEXL, prtsig,
	"\nTime",	(char *) &((struct proc *)0)->p_time,	HALFDEC, 0,
	"\tcpu",	(char *) &((struct proc *)0)->p_cpu,	HALFDEC, 0,
	"\tnice",	(char *) &((struct proc *)0)->p_nice,	HALFDEC, 0,
	"\tpgrp",	(char *) &((struct proc *)0)->p_pgrp,	OCT, 0,
	"\nPid",	(char *) &((struct proc *)0)->p_pid,	DEC, 0,
	"\tPpid",	(char *) &((struct proc *)0)->p_ppid,	DEC, 0,
	"\taddr",	(char *) &((struct proc *)0)->p_addr,	OCT, 0,
	"\tdaddr",	(char *) &((struct proc *)0)->p_daddr,	OCT, 0,
	"\tsaddr",	(char *) &((struct proc *)0)->p_saddr,	OCT, 0,
	"\tdsize",	(char *) &((struct proc *)0)->p_dsize,	OCT, 0,
	"\tssize",	(char *) &((struct proc *)0)->p_ssize,	OCT, 0,
	"\ntextp",	(char *) &((struct proc *)0)->p_textp,	OCT, 0,
	"\tWchan",	(char *) &((struct proc *)0)->p_wchan,	OCT, 0,
	"",		(char *) &((struct proc *)0)->p_wchan,	DADDR, 0,
	"\nRlink",	(char *) &((struct proc *)0)->p_link,	OCT, 0,
	"\tClktim",	(char *) &((struct proc *)0)->p_clktim,	DEC, 0,
	"\tSlptim",	(char *) &((struct proc *)0)->p_slptime, HALFDEC, 0,
	END
};

/* The Process Status word interpretation */
char *pmsg1[] = {
	"<null>",
	"Sleeping (all cases)",
	"Abandoned Wait State",
	"*** Running ***",
	"Being created",
	"Being Terminated (ZOMB)",
	"Ptrace Stopped",
	"<eh?>"
};

/* The Process Flag Word bit table */
char *p_bits[] = {
	"in core, ",
	"scheduler proc, ",
	"locked in core, ",
	"being swapped, ",
	"being traced, ",
	"WTED, ",
	"ULOCK, ",
	"OMASK,",
	"VFORK, ",
	"VFPRINT, ",
	"VFDONE, ",
	"timing out, ",
	"detached, ",
	"OUSIG, ",
	"selecting, ",
};
#define	FLAG_MASK	037777


/*
 *	This table lists all of the dump variables to be fetched.
 */

struct fetch fetchtab[] = {

#ifdef	INTRPT
	/* interrupt tracing feature - The SYS Group */
	"_savptr",	&itrptr,	sizeof itrptr + sizeof itrace,
#endif

	"_bfreelist",	(char *) &bfreelist,	sizeof bfreelist,
	"_nfile",	(char *) &nfile,		sizeof nfile,
	"_nmount",	(char *) &nmount,		sizeof nmount,
	"_nproc",	(char *) &nproc,		sizeof nproc,
	"_ntext",	(char *) &ntext,		sizeof ntext,
	"_nclist",	(char *) &nclist,		sizeof nclist,
	"_nbuf",	(char *) &nbuf,		sizeof nbuf,
	"_ncallout",	(char *) &ncallout,	sizeof ncallout,
	"_u",		(char *) &u,		sizeof u,	
	"_bpaddr",	(char *) &bpaddr,		sizeof bpaddr,
	"_bsize",	(char *) &bsize,		sizeof bsize,
	"_ucb_cli",	(char *) &ucbclist,		sizeof ucbclist,
	"_clststrt",	(char *) &clststrt,		sizeof clststrt,
	"_cfree",	(char *) &cfreebase,		sizeof cfreebase,
	"_ninode",	(char *) &ninode,		sizeof ninode,

	/* Random Variables */
#ifdef	notdef
	"_dumplo",	(char *) &dumplo,	sizeof dumplo,
#endif	notdef
	"_rootdir",	(char *) &rootdir,	sizeof rootdir,
	"_cputype",	(char *) &cputype,	sizeof cputype,
	"_lbolt",	(char *) &lbolt,		sizeof lbolt,
	"_time",	(char *) &stime,		sizeof stime,
	"_boottim",	(char *) &bootime,	sizeof bootime,
	"_mpid",	(char *) &mpid,		sizeof mpid,
	"_runrun",	(char *) &runrun,	sizeof runrun,
	"_runout",	(char *) &runout,	sizeof runout,
	"_runin",	(char *) &runin,	sizeof runin,
	"_curpri",	(char *) &curpri,	sizeof curpri,
	"_maxmem",	(char *) &maxmem,	sizeof maxmem,
	"_physmem",	(char *) &physmem,	sizeof physmem,
	"_rootdev",	(char *) &rootdev,	sizeof rootdev,
	"_swapdev",	(char *) &swapdev,	sizeof swapdev,
	"_pipedev",	(char *) &pipedev,	sizeof pipedev,
	"_nswap",	(char *) &nswap,		sizeof nswap,
	"_updlock",	(char *) &updlock,	sizeof updlock,
	"_rablock",	(char *) &rablock,	sizeof rablock,
	"_msgbuf",	(char *) &msgbuf,	sizeof msgbuf,
	"_panicst",	(char *) &panicstr,	sizeof panicstr,
	END
};

struct	fetch fetch1tab[] = {
	"__ovno",	(char *) &ovno,		sizeof ovno,
	END
};

/*
 *	This table lists all addresses to be fetched.
 */

struct fetch fetchatab[] = {
	"ova",		(char *) &aova,			sizeof *ova,
	"ovd",		(char *) &aovd,			sizeof *ovd,
	END
};

char	*corref = "/core";		/* Default dump file */
char *symref = "/unix";
char *MIS_ARG = "Missing arg";

unsigned find(); int stroct();
char	*calloc();

/* Flag definitions */
int bflg = 0;				/* brief flag -- u only */
int dflg = 0;				/* Swapdev flag */
int vflg = 0;				/* verbose flag -- dump each proc */
int tflg = 0;				/* dump tty structs flag */
int iflg = 0;				/* dump inode table flag */
#ifdef	INTRPT
int trflg = 0;				/* interrupt trace display flag */
#endif	INTRPT


/*
 *			M A I N
 *
 *	Consume options and supervise the flow of control
 *
 */

main(argc,argv)
int argc;
char **argv;
{
	static unsigned *up;	/*!!! never initialized */
	register int i;

	int trapstack;
	int u_addr;				/* u block number */
	int u_death;			/* death u */
	unsigned sp;
	unsigned r5;
	unsigned xbuf[8];

	trapstack = 0;
	u_addr = 0;

	/* parse off the arguments and do what we can with them */
	for(i=1; i<argc; i++)
	    if( *argv[i] == '-' ) {
		switch( argv[i][1] ) {
	
		case 'c':
			/* Coredump input file name */
			if ((i+1) >= argc) barf( MIS_ARG );
			corref = argv[++i];
			continue;
	
		case 's':
			/* Symboltable input file name */
			if ((i+1) >= argc) barf( MIS_ARG );
			symref = argv[++i];
			continue;
	
		case 'u':
			/* trace a process other than current (takes U addr) */
			if ((i+1) >= argc) barf( MIS_ARG );
			u_addr = stroct( argv[++i] );
			continue;
	
		case 'b':
			/* Brief */
			bflg++;
			continue;

		case 'd':
			/* Dump includes swap area */
			dflg++;
			continue;

		case 'v':	/* do everything */
			/* Verbose */
			vflg++;
			continue;

		case 't':
			/* TTY structures */
			tflg++;
			continue;

		case 'i':
			/* Inode table */
			iflg++;
			continue;

#ifdef	INTRPT
		case 'z':
			/* Interrupt Trace */
			trflg++;
			continue;
#endif	INTRPT

		default:
			printf("Bad argument:  %s\n", argv[i]);

		case 'h':
			/* Help message */
			printf("Usage:  crash [-i] [-d] -[t] [-c dumpfile] [-s symtab file] [-u u-addr]\n\n");
			exit(0);
		}
	
	    } else {
		trapstack = stroct( argv[i] );
		if( trapstack == 0 )
			printf("Bad argument:  %s\n", argv[i]);
	    }

	kmem = open(corref,0);
	if (kmem < 0) {
		printf("Unable to open %s\n",corref);
		exit(-1);
	}

	/*
	 * Load the symbol table
	 */
	getsym(symref);

	/*
	 * Load all of the shadow variables from the dump file,
	 * and print the header page.
	 */
	fetch(fetchtab);

	if (tflg) {
		clsize = nclist * sizeof(struct cblock);
		cfree_ = (struct cblock *)calloc(nclist,sizeof(struct cblock));
		if (cfree_ == (struct cblock *) NULL) {
			printf("crash: out of memory getting cfree\n");
			exit(1);
		}
		/* CLIST loaded first, improve accuracy on dump running sys */
		if (!vf("_cfreeli", (char *) &cfreelist, sizeof cfreelist)) {
			printf("could not find _cfreeli\n");
			exit(1);
		}
	if	(ucbclist)
		lseek(kmem, (off_t)ctob((long)clststrt), 0); /* clists click */
	else
		lseek(kmem, (off_t) cfreebase, 0);  /* clists in kernel  */

		read(kmem, (caddr_t) cfree_, clsize);

		vf("_nkl11", (char *) &nkl11, sizeof nkl11);
		kl11 = (struct tty *) calloc(nkl11, sizeof(struct tty));
		if (kl11 == (struct tty *) NULL) {
			printf("crash: out of memory in kl11\n");
			exit(1);
		}
		vf("_kl11", (char *) kl11, sizeof(struct tty)*nkl11);
		if (vf("_ndh11", (char *) &ndh11, sizeof ndh11)) {
			dh11 = (struct tty *) calloc(ndh11, sizeof(struct tty));
			if (dh11 == (struct tty *) NULL) {
				printf("crash: out of memory in dh11\n");
				exit(1);
			}
			vf("_dh11", (char *) dh11, sizeof(struct tty)*ndh11);
		}
		if (vf("_ndz11", (char *) &ndz11, sizeof dz11)) {
			dz11 = (struct tty *) calloc(ndz11, sizeof(struct tty));
			if (dz11 == (struct tty *) NULL) {
				printf("crash: out of memory in dz11\n");
				exit(1);
			}
			vf("_dz11", (char *) dz11, sizeof(struct tty)*ndz11);
		}
	}

	/*
	 *	Setup for MENLO_KOV
	 */
	if (overlay) {
		fetch(fetch1tab);
		fetcha(fetchatab);
		vf("ova", (char *) ova, sizeof ova);
		vf("ovd", (char *) ovd, sizeof ovd);
		getpars();
	}

	/* Display general information */
	general();

	/*	dump the registers	*/
	subhead = "Registers & Stack Dump";
	saveps = find("saveps", 0);
	lseek(kmem, (off_t) saveps, 0);
	read(kmem, (caddr_t)&saveps, sizeof saveps);
	newpage();
	printf("\tThe registers\n");
	lseek(kmem, (off_t)0300,0);
	read(kmem,xbuf,sizeof xbuf);
	printf("\nr0: "); octout(xbuf[0]); /* 2 */
	putchar(' ');	  if(!symbol(xbuf[0], DSYM, 0)) putchar('\t');
	printf("\tr1: "); octout(xbuf[1]); /* 3 */
	putchar(' ');	  if(!symbol(xbuf[1], DSYM, 0)) putchar('\t');
	printf("\tr2: "); octout(xbuf[2]); /* 4 */
	putchar(' ');	  if(!symbol(xbuf[2], DSYM, 0)) putchar('\t');
	printf("\nr3: "); octout(xbuf[3]); /* 5 */
	putchar(' ');	  if(!symbol(xbuf[3], DSYM, 0)) putchar('\t');
	printf("\tr4: "); octout(xbuf[4]); /* 6 */	
	putchar(' ');	  if(!symbol(xbuf[4], DSYM, 0)) putchar('\t');
	printf("\tr5: "); octout(xbuf[5]); /* 7 */
	putchar(' ');	  if(!symbol(xbuf[5], DSYM, 0)) putchar('\t');
	printf("\nsp: "); octout(xbuf[6]); /* 8 */	
	putchar(' ');	  if(!symbol(xbuf[6], DSYM, 0)) putchar('\t');
	printf("\tksda: "); octout(xbuf[7]); /* 9 */
	printf("\t\tps: "); octout(saveps); printps(saveps);
	/* was: sp = xbuf[6] & 0177760; /*!!! why this mask?? */
	sp = xbuf[6] & 0177776;
	r5 = xbuf[5];

	/*
	 *	Take a stack dump
	 */
	u_death = xbuf[7];
	if( u_addr == 0 )
		u_addr = u_death;

	/* lseek in u_addr core clicks (64 bytes) */
	lseek(kmem, (off_t)(unsigned)u_addr<<6,0);
	read (kmem, &u, USIZEB);

	if (u_addr != u_death) {
		sp = u.u_rsave.val[R6];
		r5 = u.u_rsave.val[R5];
	}

	printf ("\n\n u at 0%o, r5 = 0%o, sp = 0%o\n\n", u_addr, r5, sp);

	/*	If an aps was specified, give ps, pc, sp at trap time */
	if (trapstack == 0) {
		trapstack = sp;	/* temporary */
	}
	else {
		printf("\n\n\timmediately prior to the trap:\n");
		printf("\nsp: "); octout((unsigned)sp+4);
		printf("\tps: "); octout(up[1]);
		printf("\tpc: "); octout(up[0]);
		putchar(' ');
		symbol(up[0], ISYM, 0);
	}

	stackdump( sp-2, sp-4 );	/* current r5 and some number less */

	/* Display Procsss information */
	if (!bflg) dprocs();

	putchar('\n');		/* Flush buffer:  The End */
	exit(0);
}


/*
 *			S T A C K D U M P
 *
 * This routine interprets the kernel stack in the
 * user structure (_u).
 */
stackdump( r5, sp )
unsigned r5, sp;
{
	register unsigned *up;
	register unsigned i;

	if ((sp < 0140000) || (sp > 0140000+USIZEB-18)) {
		printf("\n\tsp(%0o) is unreasonable, 0140000 assumed\n", sp);
		sp = 0140000;
		/* since sp is bad, give him whole user area */
	}

	/* now give a dump of the stack, relocating as we go */
	printf("\n\n");
	printf("STACK:  loc+00 loc+02 loc+04 loc+06   loc+10 loc+12 loc+14 loc+16");

	for( i = sp & ~017; i < 0140000+USIZEB; i += 16) {
	
		up = (unsigned *) &u;
		up += ((int)i - 0140000)>>1;	/* damn pointer conversions */
		printf("\n"); octout(i);
		printf(": "); octout(up[0]);
		printf(" "); octout(up[1]);
		printf(" "); octout(up[2]);
		printf(" "); octout(up[3]);
		printf("   "); octout(up[4]);
		printf(" "); octout(up[5]);
		printf(" "); octout(up[6]);
		printf(" "); octout(up[7]);
	}

	/* go back to beginning of stack space and give trace */
	printf("\n\n");
	printf("TRACE:   old r2  old r3  old r4  ");
	if (overlay)
		printf("ovno    ");
	printf("old r5  old pc");
	i = sp;
	while ((r5 > i) && (r5 < 0140000+USIZEB)) {
		if (r5 == 0140000+USIZEB-18) break;
		up = (unsigned *) &u;
		up += ((int)r5 - 0140000)>>1;
		printf("\n"); octout(r5);
		printf(":  ");
		if (overlay) {
			      octout(up[-4]);
		printf("  ");
		}
			      octout(up[-3]);
		printf("  "); octout(up[-2]);
		printf("  "); octout(up[-1]);
		printf("  "); octout(up[0]);
		printf("  "); octout(up[1]);
		putchar(' ');
		symbol(up[1], ISYM, (overlay ? up[-1] : 0));
		i = r5 + 4;
		r5 = up[0];
	}
	putchar('\n');
	if (r5 != 0140000+USIZEB-18)
		printf("\tThe dynamic chain is broken\n");
}


/*
 *			G E N E R A L
 *
 *
 * Display information of a general nature
 */
general() {
	register int	i;	/* multi-function counter */
	register int	j;

	/* Set up page heading */
	subhead = "General Information";
	newpage();

	/* Tell general stuff */
	printf("UNIX Version 2.10bsd was running on a PDP-11/%d CPU\n",
		cputype);
	printf("when a crash occured at %s", ctime(&stime));
	printf("system booted at %s", ctime(&bootime));
	/* print panic message */
	panicpr();
	printf("\n\nThe `Random Variables' at time of crash:\n\n");
	display(rv_tab, 0);
	if (overlay)
		display(ov_tab, 0);
	/* print contents of msgbf */
	msgbufpr();
	/* Display Mount Table */
	{
		struct mount *mount;	/* NMOUNT */
		register struct mount *mp;

		mount = (struct mount *) calloc(nmount, sizeof(struct mount));
		if (mount != (struct mount *) NULL) {
			subhead = "The Mount Structure";
			newpage();
			if (vf("_mount", (char *)mount,
			    sizeof(struct mount)*nmount))
				for(mp = mount; mp < &mount[nmount]; mp++)
					if (mp->m_dev) {
						display(mnt_tab, mp);
						printdev(mp);
					}
			free(mount);
		}
	}

	/* Display mbuf status info */
	dispnet();

	/* Display the Callouts */
	if( tflg )  {
		struct callout *cp;

		subhead = "The Callout Structure";
		newpage();
		callout = (struct callout *) calloc(ncallout, sizeof(struct callout));
		if (callout == (struct callout *) NULL) {
			printf("crash: out of memory getting callout\n");
			exit(1);
		}
		vf("_callout", (char *) callout, sizeof(struct callout)*ncallout);
		for(cp=callout; cp<&callout[ncallout]; cp++)
			if (cp->c_func)
				display( cal_buf, cp );
		free(callout);
	}

#ifdef	INTRPT
	/* Display Interrupt Trace */
	if( trflg )  {
		itrstr = find( "_savstr", 1 );
		itrend = find( "_savend", 1 );
		subhead = "The Interrupt Trace (The SYS Group)";
		newpage();
		printf("\nEntry %d was the last trap processed\n",
			(itrptr - itrstr)/(sizeof (struct itrace)) - 1
		);

		printf("\nEntry\tInt PC\t<address>\tInt PS\tTrap r0\t<address>\tTrap PS\n");
		for(i = 0; i < (itrend - itrstr)/(sizeof(struct itrace)); i++) 
		{
			printf("\n%3d",i);
			display( itr_buf, i*(sizeof (struct itrace)) );
		}
	}
#endif

	/* If requested, dump tty structures and C-list */
	if( tflg )  {
		struct tty *tp;
		struct cblock *bp;

		subhead = "The KL-11 TTY Structures";
		newpage();
		line = 8;
		for(tp=kl11; tp<&kl11[nkl11]; tp++)  {
			if( line > LINESPERPAGE ) {
				newpage();
				line = 8;
			}
			display(tty_buf, tp );
			line += 13;
			if( tp->t_rawq.c_cc > 0 ) {
				bp = (struct cblock *)(tp->t_rawq.c_cf-1);
				bp = (struct cblock *)((int)bp & ~CROUND);
				chase( "RAW", bp);
			}
			if( tp->t_outq.c_cc > 0 ) {
				bp = (struct cblock *)(tp->t_outq.c_cf-1);
				bp = (struct cblock *)((int)bp & ~CROUND);
				chase( "OUT", bp );
			}
			if( tp->t_canq.c_cc > 0 ) {
				bp = (struct cblock *)(tp->t_canq.c_cf-1);
				bp = (struct cblock *)((int)bp & ~CROUND);
				chase( "CAN", bp);
			}
		}

		if (ndh11 != 0) {
			subhead = "The DH-11 TTY structures";
			newpage();
			line = 8;
			for(tp=dh11; tp<&dh11[ndh11]; tp++)  {
				if( line > LINESPERPAGE ) {
					newpage();
					line = 8;
				}
				display(tty_buf, tp);
				line += 13;
				if( tp->t_rawq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_rawq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase("RAW", bp);
				}
				if( tp->t_outq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_outq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase( "OUT", bp);
				}
				if( tp->t_canq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_canq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase( "CAN", bp);
				}
			}
		}

		if (ndz11 != 0) {
			subhead = "The DZ-11 TTY Structures";
			newpage();
			line = 8;
			for(tp=dz11; tp<&dz11[ndz11]; tp++)  {
				if( line > LINESPERPAGE ) {
					newpage();
					line = 8;
				}
				display(tty_buf, tp );
				line += 13;
				if( tp->t_rawq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_rawq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase( "RAW", bp );
				}
				if( tp->t_outq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_outq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase( "OUT", bp );
				}
				if( tp->t_canq.c_cc > 0 ) {
				    bp = (struct cblock *)(tp->t_canq.c_cf-1);
				    bp = (struct cblock *)((int)bp & ~CROUND);
				    chase( "CAN", &tp->t_canq );
				}
			}
		}

		subhead = "The CBUF Freelist";
		newpage();
		line = 8;
		if( cfreelist != (struct cblock *) NULL )
			chase( "Freelist", cfreelist);
		else
			printf("The freelist was empty\n");
	}

	/* display the C-list */
	if( tflg )  {
		struct cblock *cp;

		subhead = "The C-list";
		newpage();
		line = 8;
		if (ucbclist)
			j = (unsigned)0120000;
		else
			j = (unsigned)cfreebase;

		for(cp=cfree_; cp<&cfree_[nclist]; cp++)  {
			putchar('\n');
			octout( j );
			display( cl_buf, cp);
			j += sizeof(struct cblock);
			line++;
			if (line > LINESPERPAGE) {
				newpage();
				line = 8;
			}
		}
	}

	/* Display the in-core inode table */
	if( iflg )  {
		struct buf *buf, *bp;	/* NBUF */
		struct inode *inode, *ip;	/* NINODE */

		buf = (struct buf *) calloc(nbuf, sizeof(struct buf));
		if (buf == (struct buf *) NULL) {
			printf("crash: out of memory in buf\n");
			exit(1);
		}
		subhead = "The BUF struct";
		newpage();
		line = 8;
		j = find("_buf", 1 );
		if (vf("_buf", (char *) buf, sizeof(struct buf)*nbuf)) {
			for(bp = buf; bp < &buf[nbuf]; bp++)  {
				if( line > LINESPERPAGE ) {
					newpage();
					line = 8;
				}
				printf("\n\nBuf stored at 0%o\n", j);
				display(buf_tab, bp);
				line += 6;
				j += sizeof(struct buf);
			}
		}
		free(buf);

		inode = (struct inode *) calloc(ninode, sizeof(struct inode));
		if (inode == (struct inode *) NULL) {
			printf("crash: out of memory in inode\n");
			exit(1);
		}
		subhead = "The In-Core Inode Table";
		newpage();
		j = find( "_inode", 1 );
		if (vf("_inode", (char *) inode, sizeof(struct inode)*ninode)) {
			for(ip = inode; ip<&inode[ninode]; ip++ )  {
				if (ip->i_count == 0)
					continue;
				if( line > LINESPERPAGE )
					newpage();
				printf("\n\nInode stored at %o\n", j);
				display(inode_tab, ip);
				line += 6; 
				j += sizeof inode[0];
			}
		}
		free(inode);
	}
}

/*
 *			C H A S E
 *
 * This routine is used to chase down a c-list chain,
 * and display the data found there.
 * Note the external variable, clststrt or cfreebase.
 */
chase( msg, headp )
register struct cblock *headp;
{
	register unsigned int cp;
	memaddr clstp, clstbase, clstend;

	if	(ucbclist)
		clstbase = (memaddr)0120000;
	else
		clstbase = (memaddr)cfreebase;

	clstend = clstbase + clsize;
	printf("\n----The %s Queue-----\n", msg);
	line += 2;

	clstp = (unsigned) headp;
	cp = (unsigned) cfree_ + clstp - clstbase;
	while( clstp != 0 )  {
		putchar('\n');
		line++;
		if (line > LINESPERPAGE) {
			newpage();
			printf("\n----The %s Queue-----\n", msg);
			line = 8;
		}
		octout( clstp );

		if( clstp > clstend || clstp < clstbase)
			barf(" cp exceeds limit");
		display( cl_buf, cp );

		clstp = (memaddr) ((struct cblock *)cp)->c_next;
		cp = (unsigned) cfree_ + clstp - clstbase;
	}
}



/*
 *			D U S E R
 *
 *
 *	Display the currently loaded User Structure
 */
duser() {

	printf("Name: %-15s\n", u.u_comm);
	display( u_tab, 0 );
	printf("\n\nSignal Disposition:");
	col( 0, (unsigned *)u.u_signal, NSIG );
	printf("\n\nOpen file pointers");
	col( 0, (unsigned *)u.u_ofile, NOFILE );
/****
	printf("\n\nuisa:");
	col( 0, (unsigned *)u.u_uisa, 16 );
	printf("\n\nuisd:");
	col( 0, (unsigned *)u.u_uisd, 16);
****/
}


/*
 *			D P R O C S
 *
 *	Display information on each process
 *
 *    For every line entry in the process table,
 * the ptable entry is displayed.  Then the
 * per-process user structure is loaded into
 * this program's incore copy of the user structure.
 * Selected tidbits are displayed by duser().
 *	Mike Muuss, 7/8/77.
 */

dprocs() {
	register struct proc *p;
	register unsigned i;
	register unsigned loc;
	long baddr;			/* dump byte addr */


	proc = (struct proc *) calloc(nproc, sizeof(struct proc));
	if (proc == (struct proc *) NULL) {
		printf("crash: out of memory getting proc\n");
		exit(1);
	}
	vf("_proc", (char *) proc, sizeof(struct proc)*nproc);
	loc = find( "_proc", 1 );	/* base dump addr */
	subhead = "Information on Pid XXXXX";		/* page header */

	/* Consider each active entry */
	for( i=0, p=proc; i<nproc; i++, p++, loc += sizeof(struct proc) ) {
		if( p->p_stat == 0 ) continue;

		prints( &subhead[19], p->p_pid);
		newpage();

		printf("Entry for PID %d at ", p->p_pid);
		octout((unsigned) loc);
		printf("\nStatus was: %s on ",
			(p->p_stat > sizeof pmsg1 ?
				(char *)sprintf("%d", p->p_stat) :
				 pmsg1[p->p_stat]));
		symbol(p->p_wchan, DSYM, 0);
		printf(" (0%o) wchan.\nFlags set were: ", p->p_wchan);
		putbits( p_bits, p->p_flag&FLAG_MASK );	/* interpret flags */
		putchar('\n');
		display( proc_tab, p );
		putchar('\n');

		/* Analyze the swapspace image */
		if( p->p_flag & SLOAD ) {
			baddr = p->p_addr * 64L;
		} else {
			if( !dflg ) {
				printf("\nImage not avail.\n");
				continue;
			}
			printf("\n Image retrieved from swapdev.\n");
			baddr = ( p->p_addr ) * 512L;
		}

		lseek( kmem, (off_t) baddr, 0 );
		read( kmem, &u, USIZEB );

		/* Verify integrity of swap image, and if OK, print */
		if( (unsigned)u.u_procp != loc )  {
			printf("\n----> Image Defective <----\n");
			printf("\nu.u_procp="); octout((unsigned)u.u_procp);
			printf("\tbaddr="); loctout(baddr);
			continue;
		}
		duser();

		/* eventually, I plan to print a core dump */
		stackdump( (unsigned)u.u_rsave.val[R5], (unsigned)u.u_rsave.val[R6] );
	}
	free(proc);
}

/*
 * print panicstr
 */
panicpr()
{
	char pbuf[20];
	register char ch;
	register int i;

	if (panicstr != (char *)0) {
		printf("\n\tpanicstr: \"");
		lseek(kmem,(off_t)(unsigned)panicstr,0);
		read(kmem,pbuf,sizeof pbuf);
		for (i=0; i<sizeof pbuf; i++) {
			if ((ch=pbuf[i]) == 0)
				break;
			if (ch < 040 || ch > 0177) {
				switch(ch) {
				case '\n':
					printf("\\n");
					break;
				case '\r':
					printf("\\r");
					break;
				case '\t':
					printf("\\t");
					break;
				default:
					printf("\\%3o",ch);
				}
			} else {
				putchar(ch);
			}
		}
		putchar('"');
	} else {
		printf("\n\tpanicstr is null");
	}
}

/*
 * Print contents of msgbuf
 */
msgbufpr()
{
	register int i;
	register int j;
	register char ch;

	printf("\n\nThe contents of msgbuf:");
	for (i=0, j=0; i<MSG_BSIZE; i++) {
		if ((j++)%20 == 0)
			putchar('\n');
		if ((ch=msgbuf.msg_bufc[i]) < 040 || ch > 0177) {
			switch(ch) {
			case '\n':
				printf("  \\n");
				break;
			case '\r':
				printf("  \\r");
				break;
			case '\t':
				printf("  \\t");
				break;
			default:
				printf("\\%3o",ch);
			}
		} else {
			printf("   %c",ch);
		}
	}
}
