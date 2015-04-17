/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)psl.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * PDP program status longword
 */

#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_V		0x00000002	/* overflow bit */
#define	PSL_Z		0x00000004	/* zero bit */
#define	PSL_N		0x00000008	/* negative bit */
#define	PSL_ALLCC	0x0000000f	/* all cc bits - unlikely */
#define	PSL_T		0x00000010	/* trace enable bit */
#define	PSL_IPL		0x000000e0	/* interrupt priority level */
#define	PSL_PRVMOD	0x00003000	/* previous mode (all on is user) */
#define	PSL_CURMOD	0x0000c000	/* current mode (all on is user) */
#define	PSL_CURSUP	0x00004000	/* current supervisor previous kernel */
#define	PSL_BR0		0x00000000	/* bus request level 0 */
#define	PSL_BR1		0x00000020	/* bus request level 1 */
#define	PSL_BR2		0x00000040	/* bus request level 2 */
#define	PSL_BR3		0x00000060	/* bus request level 3 */
#define	PSL_BR4		0x00000080	/* bus request level 4 */
#define	PSL_BR5		0x000000a0	/* bus request level 5 */
#define	PSL_BR6		0x000000c0	/* bus request level 6 */
#define	PSL_BR7		0x000000e0	/* bus request level 7 */

#define	PSL_USERSET	(PSL_PRVMOD|PSL_CURMOD)
#define	PSL_USERCLR	PSL_IPL		/* must be clear in user mode */
