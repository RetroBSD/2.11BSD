/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fperr.h	1.2 (2.11BSD) 1997/8/28
 */

/*
 * Structure of the floating point error register save/return
 */
struct fperr {
	short	f_fec;
	caddr_t	f_fea;
};

/*
 * These are not used any where and are present here merely for reference.
 * All the of the VAX oriented FP codes were removed from signal.h and the
 * kernel.  The kernel now passes the unaltered contents of the FEC register to
 * the application.  The values below were taken from the PDP-11/70 processor
 * handbook.
*/

#ifdef	notnow
#define	FPE_OPCODE_TRAP	0x2	/* Bad FP opcode */
#define	FPE_FLTDIV_TRAP 0x4	/* FP divide by zero */
#define	FPE_INTOVF_TRAP	0x6	/* FP to INT overflow */
#define	FPE_FLTOVF_TRAP	0x8	/* FP overflow */
#define	FPE_FLTUND_TRAP	0xa	/* FP underflow */
#define	FPE_UNDEF_TRAP	0xc	/* FP Undefined variable */
#define	FPE_MAINT_TRAP	0xe	/* FP Maint trap */
#endif /* notnow */
