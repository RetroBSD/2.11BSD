/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)trap.c	1.6 (2.11BSD) 1999/9/13
 */

#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/seg.h"
#include "../machine/trap.h"
#include "../machine/iopage.h"

#include "signalvar.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "vm.h"

extern int fpp, kdj11;

#ifdef INET
extern int netoff;
#endif

#ifdef DIAGNOSTIC
extern int hasmap;
static int savhasmap;
#endif

/*
 * Offsets of the user's registers relative to
 * the saved r0.  See sys/reg.h.
 */
char regloc[] = {
	R0, R1, R2, R3, R4, R5, R6, R7, RPS
};

/*
 * Called from mch.s when a processor trap occurs.
 * The arguments are the words saved on the system stack
 * by the hardware and software during the trap processing.
 * Their order is dictated by the hardware and the details
 * of C's calling sequence. They are peculiar in that
 * this call is not 'by value' and changed user registers
 * get copied back on return.
 * Dev is the kind of trap that occurred.
 */
/*ARGSUSED*/
trap(dev, sp, r1, ov, nps, r0, pc, ps)
	dev_t dev;
	caddr_t sp, pc;
	int r1, ov, nps, r0, ps;
{
	extern int _ovno;
	static int once_thru = 0;
	register int i;
	register struct proc *p;
	time_t syst;
	mapinfo kernelmap;

#ifdef UCB_METER
	cnt.v_trap++;
#endif
	/*
	 * In order to stop the system from destroying
	 * kernel data space any further, allow only one
	 * fatal trap. After once_thru is set, any
	 * futher traps will be handled by looping in place.
	 */
	if (once_thru) {
		(void) _splhigh();
		for(;;);
	}

	if (USERMODE(ps))
		dev |= USER;
	else
#ifdef INET
	if (SUPVMODE(ps))
		dev |= SUPV;
	else
#endif
		savemap(kernelmap);	/* guarantee normal kernel mapping */
	syst = u.u_ru.ru_stime;
	p = u.u_procp;
	u.u_fpsaved = 0;
	u.u_ar0 = &r0;
	switch(minor(dev)) {

	/*
	 * Trap not expected.  Usually a kernel mode bus error.  The numbers
	 * printed are used to find the hardware PS/PC as follows.  (all
	 * numbers in octal 18 bits)
	 *	address_of_saved_ps =
	 *		(ka6*0100) + aps - 0140000;
	 *	address_of_saved_pc =
	 *		address_of_saved_ps - 2;
	 */
	default:
		once_thru = 1;
#ifdef DIAGNOSTIC
		/*
		 * Clear hasmap if we attempt to sync the fs.
		 * Else it might fail if we faulted with a mapped
		 * buffer.
		 */
		savhasmap = hasmap;
		hasmap = 0;
#endif
		i = splhigh();
		printf("ka6 %o aps %o\npc %o ps %o\nov %d\n", *ka6, &ps, 
			pc, ps, ov);
		if ((cputype == 70) || (cputype == 44))
			printf("cpuerr %o\n", *CPUERR);
		printf("trap type %o\n", dev);
		splx(i);
		panic("trap");
		/*NOTREACHED*/

	case T_BUSFLT + USER:
		i = SIGBUS;
		break;

	case T_INSTRAP + USER:
#if defined(FPSIM) || defined(GENERIC)
		/*
		 * If no floating point hardware is present, see if the
		 * offending instruction was a floating point instruction ...
		 */
		if (fpp == 0) {
			i = fptrap();
#ifdef UCB_METER
			if (i != SIGILL) {
				cnt.v_fpsim++;
			}
#endif
			if (i == 0)
				goto out;
			if (i == SIGTRAP)
				ps &= ~PSL_T;
			u.u_code = u.u_fperr.f_fec;
			break;
		}
#endif
		/*
		 * If illegal instructions are not being caught and the
		 * offending instruction is a SETD, the trap is ignored.
		 * This is because C produces a SETD at the beginning of
		 * every program which will trap on CPUs without an FP-11.
		 */
#define	SETD	0170011		/* SETD instruction */
		if(fuiword((caddr_t)(pc-2)) == SETD && u.u_signal[SIGILL] == 0)
			goto out;
		i = SIGILL;
		u.u_code = ILL_RESAD_FAULT;	/* it's simplest to lie */
		break;

	case T_BPTTRAP + USER:
		i = SIGTRAP;
		ps &= ~PSL_T;
		break;

	case T_IOTTRAP + USER:
		i = SIGIOT;
		break;

	case T_EMTTRAP + USER:
		i = SIGEMT;
		break;

	/*
	 * Since the floating exception is an imprecise trap, a user
	 * generated trap may actually come from kernel mode.  In this
	 * case, a signal is sent to the current process to be picked
	 * up later.
	 */
#ifdef INET
	case T_ARITHTRAP+SUPV:
#endif
	case T_ARITHTRAP:
	case T_ARITHTRAP + USER:
		i = SIGFPE;
		stst(&u.u_fperr);	/* save error code and address */
		u.u_code = u.u_fperr.f_fec;
		break;

	/*
	 * If the user SP is below the stack segment, grow the stack
	 * automatically.  This relies on the ability of the hardware
	 * to restart a half executed instruction.  On the 11/40 this
	 * is not the case and the routine backup/mch.s may fail.
	 * The classic example is on the instruction
	 *	cmp	-(sp),-(sp)
	 *
	 * The KDJ-11 (11/53,73,83,84,93,94) handles the trap when doing
	 * a double word store differently than the other pdp-11s.  When
	 * doing:
	 *	setl
	 *	movfi fr0,-(sp)
	 * and the stack segment becomes invalid part way thru then the
	 * trap is generated (as expected) BUT 'sp' IS NOT LEFT DECREMENTED!
	 * The 'grow' routine sees that SP is still within the (valid) stack
	 * segment and does not extend the stack, resulting in a 'segmentation
	 * violation' rather than a successfull floating to long store.
	 * The "fix" is to pretend that SP is 4 bytes lower than it really
	 * is (for KDJ-11 systems only) when calling 'grow'.
	 */
	case T_SEGFLT + USER:
		{
		caddr_t osp;

		osp = sp;
		if (kdj11)
			osp -= 4;
		if (backup(u.u_ar0) == 0)
			if (!(u.u_sigstk.ss_flags & SA_ONSTACK) && 
					grow((u_int)osp))
				goto out;
		i = SIGSEGV;
		break;
		}

	/*
	 * The code here is a half-hearted attempt to do something with
	 * all of the PDP11 parity registers.  In fact, there is little
	 * that can be done.
	 */
	case T_PARITYFLT:
	case T_PARITYFLT + USER:
#ifdef INET
	case T_PARITYFLT + SUPV:
#endif
		printf("parity\n");
		if ((cputype == 70) || (cputype == 44)) {
			for(i = 0; i < 4; i++)
				printf("%o ", MEMERRLO[i]);
			printf("\n");
			MEMERRLO[2] = MEMERRLO[2];
			if (dev & USER) {
				i = SIGBUS;
				break;
			}
		}
		panic("parity");
		/*NOTREACHED*/

	/*
	 * Allow process switch
	 */
	case T_SWITCHTRAP + USER:
		goto out;
#ifdef INET
	case T_BUSFLT+SUPV:
	case T_INSTRAP+SUPV:
	case T_BPTTRAP+SUPV:
	case T_IOTTRAP+SUPV:
	case T_EMTTRAP+SUPV:
	case T_PIRQ+SUPV:
	case T_SEGFLT+SUPV:
	case T_SYSCALL+SUPV:
	case T_SWITCHTRAP+SUPV:
	case T_ZEROTRAP+SUPV:
	case T_RANDOMTRAP+SUPV:
		i = splhigh();
		if (!netoff) {
			netoff = 1;
			savestate();
		}
		printf("Unexpected net trap (%o)\n", dev-SUPV);
		printf("ka6 %o aps %o\n", *ka6, &ps);
		printf("pc %o ps %o\n", pc, ps);
		splx(i);
		panic("net crashed");
		/*NOTREACHED*/
#endif

	/*
	 * Whenever possible, locations 0-2 specify this style trap, since
	 * DEC hardware often generates spurious traps through location 0.
	 * This is a symptom of hardware problems and may represent a real
	 * interrupt that got sent to the wrong place.  Watch out for hangs
	 * on disk completion if this message appears.  Uninitialized
	 * interrupt vectors may be set to trap here also.
	 */
	case T_ZEROTRAP:
	case T_ZEROTRAP + USER:
		printf("Trap to 0: ");
		/*FALL THROUGH*/
	case T_RANDOMTRAP:		/* generated by autoconfig */
	case T_RANDOMTRAP + USER:
		printf("Random intr ignored\n");
		if (!(dev & USER))
			restormap(kernelmap);
		return;
	}
	psignal(p, i);
out:
	while (i = CURSIG(p))
		postsig(i);
	curpri = setpri(p);
	if (runrun) {
		setrq(u.u_procp);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale)
		addupc(pc, &u.u_prof, (int) (u.u_ru.ru_stime - syst));
	if (u.u_fpsaved)
		restfp(&u.u_fps);
}

/*
 * Called from mch.s when a system call occurs.  dev, ov and nps are
 * unused, but are necessitated by the values of the R[0-7] ... constants
 * in sys/reg.h (which, in turn, are defined by trap's stack frame).
 */
/*ARGSUSED*/
syscall(dev, sp, r1, ov, nps, r0, pc, ps)
	dev_t dev;
	caddr_t sp, pc;
	int r1, ov, nps, r0, ps;
{
	extern int nsysent;
	register int code;
	register struct sysent *callp;
	time_t syst;
	register caddr_t opc;	/* original pc for restarting syscalls */
	int	i;

#ifdef UCB_METER
	cnt.v_syscall++;
#endif

	syst = u.u_ru.ru_stime;
	u.u_fpsaved = 0;
	u.u_ar0 = &r0;
	u.u_error = 0;
	opc = pc - 2;			/* opc now points at syscall */
	code = fuiword((caddr_t)opc) & 0377;	/* bottom 8 bits are index */
	if (code >= nsysent)
		callp = &sysent[0];	/* indir (illegal) */
	else
		callp = &sysent[code];
	if (callp->sy_narg)
		copyin(sp+2, (caddr_t)u.u_arg, callp->sy_narg*NBPW);
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = r1;
	if	(setjmp(&u.u_qsave) == 0)
		{
		(*callp->sy_call)();
#ifdef	DIAGNOSTIC
		if	(hasmap)
			panic("hasmap");
#endif
		}
	switch	(u.u_error)
		{
		case	0:
			ps &= ~PSL_C;
			r0 = u.u_r.r_val1;
			r1 = u.u_r.r_val2;
			break;
		case	ERESTART:
			pc = opc;
			break;
		case	EJUSTRETURN:
			break;
		default:
			ps |= PSL_C;
			r0 = u.u_error;
			break;
		}
	while	(i = CURSIG(u.u_procp))
		postsig(i);
	curpri = setpri(u.u_procp);
	if (runrun) {
		setrq(u.u_procp);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale)
		addupc(pc, &u.u_prof, (int)(u.u_ru.ru_stime - syst));
	if (u.u_fpsaved)
		restfp(&u.u_fps);
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}
