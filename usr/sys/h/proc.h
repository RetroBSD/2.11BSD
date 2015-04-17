/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)proc.h	1.5 (2.11BSD) 1999/9/5
 */

#ifndef	_SYS_PROC_H_
#define	_SYS_PROC_H_

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
	struct	proc *p_nxt;	/* linked list of allocated proc slots */
	struct	proc **p_prev;	/* also zombies, and free proc's */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	short	p_flag;
	short	p_uid;		/* user id, used to direct tty signals */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	long	p_sig;		/* signals pending to this process */
	char	p_stat;
	char	p_dummy;	/* room for one more, here */

	/*
	 * Union to overwrite information no longer needed by ZOMBIED
	 * process with exit information for the parent process.  The
	 * two structures have been carefully set up to use the same
	 * amount of memory.  Must be very careful that any values in
	 * p_alive are not used for zombies (zombproc).
	 */
	union {
	    struct {
		char	P_pri;		/* priority, negative is high */
		char	P_cpu;		/* cpu usage for scheduling */
		char	P_time;		/* resident time for scheduling */
		char	P_nice;		/* nice for cpu usage */
		char	P_slptime;	/* secs sleeping */
		char	P_ptracesig;	/* used between parent & traced child */
		struct proc *P_hash;	/* hashed based on p_pid */
		long	P_sigmask;	/* current signal mask */
		long	P_sigignore;	/* signals being ignored */
		long	P_sigcatch;	/* signals being caught by user */
		short	P_pgrp;		/* name of process group leader */
		struct	proc *P_link;	/* linked list of running processes */
		memaddr	P_addr;		/* address of u. area */
		memaddr	P_daddr;	/* address of data area */
		memaddr	P_saddr;	/* address of stack area */
		size_t	P_dsize;	/* size of data area (clicks) */
		size_t	P_ssize;	/* size of stack segment (clicks) */
		caddr_t	P_wchan;	/* event process is awaiting */
		struct	text *P_textp;	/* pointer to text structure */
		struct	k_itimerval P_realtimer;
	    } p_alive;
	    struct {
		short	P_xstat;	/* exit status for wait */
		struct k_rusage P_ru;	/* exit information */
	    } p_dead;
	} p_un;
};
#define	p_pri		p_un.p_alive.P_pri
#define	p_cpu		p_un.p_alive.P_cpu
#define	p_time		p_un.p_alive.P_time
#define	p_nice		p_un.p_alive.P_nice
#define	p_slptime	p_un.p_alive.P_slptime
#define	p_hash		p_un.p_alive.P_hash
#define	p_ptracesig	p_un.p_alive.P_ptracesig
#define	p_sigmask	p_un.p_alive.P_sigmask
#define	p_sigignore	p_un.p_alive.P_sigignore
#define	p_sigcatch	p_un.p_alive.P_sigcatch
#define	p_pgrp		p_un.p_alive.P_pgrp
#define	p_link		p_un.p_alive.P_link
#define	p_addr		p_un.p_alive.P_addr
#define	p_daddr		p_un.p_alive.P_daddr
#define	p_saddr		p_un.p_alive.P_saddr
#define	p_dsize		p_un.p_alive.P_dsize
#define	p_ssize		p_un.p_alive.P_ssize
#define	p_wchan		p_un.p_alive.P_wchan
#define	p_textp		p_un.p_alive.P_textp
#define	p_realtimer	p_un.p_alive.P_realtimer
#define	p_clktim	p_realtimer.it_value

#define	p_xstat		p_un.p_dead.P_xstat
#define	p_ru		p_un.p_dead.P_ru

#define	PIDHSZ		16
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

#if defined(KERNEL) && !defined(SUPERVISOR)
struct	proc *pidhash[PIDHSZ];
struct	proc *pfind();
struct	proc proc[], *procNPROC;	/* the proc table itself */
struct	proc *freeproc, *zombproc, *allproc, *qs;
			/* lists of procs in various states */
int	nproc;
#endif

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	SLOAD		0x0001	/* in core */
#define	SSYS		0x0002	/* swapper or pager process */
#define	SLOCK		0x0004	/* process being swapped out */
#define	SSWAP		0x0008	/* save area flag */
#define	P_TRACED	0x0010	/* process is being traced */
#define	P_WAITED	0x0020	/* another tracing flag */
#define	SULOCK		0x0040	/* user settable lock in core */
#define	P_SINTR		0x0080	/* sleeping interruptibly */
#define	SVFORK		0x0100	/* process resulted from vfork() */
#define	SVFPRNT		0x0200	/* parent in vfork, waiting for child */
#define	SVFDONE		0x0400	/* parent has released child in vfork */
	/*		0x0800	/* unused */
#define	P_TIMEOUT	0x1000	/* tsleep timeout expired */
#define	P_NOCLDSTOP	0x2000	/* no SIGCHLD signal to parent */
#define	P_SELECT	0x4000	/* selecting; wakeup/waiting danger */
	/*		0x8000	/* unused */

#define	S_DATA	0		/* specified segment */
#define	S_STACK	1

#endif	/* !_SYS_PROC_H_ */
