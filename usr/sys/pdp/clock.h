/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)clock.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

#define	SECDAY		((unsigned)(24*60*60))		/* seconds per day */
#define	SECYR		((unsigned)(365*SECDAY))	/* per common year */

#define	LKS		((u_short *)0177546)	/* line clock */
#define	KW11P_CCSR	((u_short *)0172540)	/* programmable clock */
#define	KW11P_CCSB	((u_short *)0172542)	/* count set buffer */
#define	KW11P_CCNT	((u_short *)0172544)	/* counter */

#define	K_RUN		0x01			/* start counter */
#define	K_10KRATE	0x02			/* 10kHz frequency */
#define	K_LFRATE	0x04			/* line frequency */
#define	K_MODE		0x08			/* repeat interrupt */
#define	K_IENB		0x40			/* interrupt enable */
