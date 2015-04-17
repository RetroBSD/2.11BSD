/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmmeter.h	7.1 (Berkeley) 6/4/86
 */

/*
 * Virtual memory related instrumentation
 */
struct vmrate
{
#define	v_first	v_swtch
	u_short	v_swtch;	/* context switches */
	u_short	v_trap;		/* calls to trap */
	u_short	v_syscall;	/* calls to syscall() */
	u_short	v_intr;		/* device interrupts */
	u_short	v_soft;		/* software interrupts */
	u_short	v_pdma;		/* pseudo-dma interrupts */
	u_short	v_ovly;		/* overlay emts */
	u_short	v_fpsim;	/* floating point simulator faults */
	u_short	v_pswpin;	/* pages swapped in */
	u_short	v_pswpout;	/* pages swapped out */
	u_short	v_pgin;		/* pageins */
	u_short	v_pgout;	/* pageouts */
#define	v_last	v_pswpout
	u_short	v_swpin;	/* swapins */
	u_short	v_swpout;	/* swapouts */
};
struct vmsum
{
	long	v_swtch;	/* context switches */
	long	v_trap;		/* calls to trap */
	long	v_syscall;	/* calls to syscall() */
	long	v_intr;		/* device interrupts */
	long	v_soft;		/* software interrupts */
	long	v_pdma;		/* pseudo-dma interrupts */
	long	v_ovly;		/* overlay emts */
	long	v_fpsim;	/* floating point simulator faults */
	long	v_pswpin;	/* pages swapped in */
	long	v_pswpout;	/* pages swapped out */
	long	v_pgin;		/* pageins */
	long	v_pgout;	/* pageouts */
	long	v_swpin;	/* swapins */
	long	v_swpout;	/* swapouts */
};
#if defined(KERNEL) && defined(UCB_METER) && !defined(SUPERVISOR)
struct vmrate	cnt, rate;
struct vmsum	sum;
#endif

/* systemwide totals computed every five seconds */
struct vmtotal
{
	short	t_rq;		/* length of the run queue */
	short	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	short	t_sl;		/* jobs sleeping in core */
	short	t_sw;		/* swapped out runnable/short block jobs */
	long	t_vm;		/* total virtual memory, clicks */
	long	t_avm;		/* active virtual memory, clicks */
	size_t	t_rm;		/* total real memory, clicks */
	size_t	t_arm;		/* active real memory, clicks */
	long	t_vmtxt;	/* virtual memory used by text, clicks */
	long	t_avmtxt;	/* active virtual memory used by text, clicks */
	size_t	t_rmtxt;	/* real memory used by text, clicks */
	size_t	t_armtxt;	/* active real memory used by text, clicks */
	size_t	t_free;		/* free memory, kb */
};
#if defined(KERNEL) && !defined(SUPERVISOR)
struct	vmtotal total;
#endif
