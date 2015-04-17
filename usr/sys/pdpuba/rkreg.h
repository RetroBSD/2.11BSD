/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rkreg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

struct	rkdevice
{
	short	rkds;
	short	rker;
	short	rkcs;
	short	rkwc;
	caddr_t	rkba;
	short	rkda;
};

/* bits in rkds; all are read only */
/* bits 15-13 are drive identification */
#define	RK_DPL		0010000		/* drive power low */
#define	RK_RK05		0004000		/* drive is an rk05; always 1 */
#define	RK_DRU		0002000		/* drive unsafe */
#define	RK_SIN		0001000		/* seek incomplete */
#define	RK_SOK		0000400		/* sector counter ok */
#define	RK_DRY		0000200		/* drive ready */
#define	RK_RWSRDY	0000100		/* read/write/seek ready */
#define	RK_WPS		0000040		/* write protect status */
#define	RK_SKC		0000020		/* seek complete */
/* bits 3-0 are sector counter */
#define	RK_BITS		\
"\10\15DPL\13DRU\12SIN\11SOK\10DRY\7RWSRDY\6WPS\5SKC"

/* bits in rker; all are read only */
#define	RKER_DRE	0100000		/* drive error */
#define	RKER_OVR	0040000		/* overrun */
#define	RKER_WLO	0020000		/* write lock out violation */
#define	RKER_SKE	0010000		/* seek error */
#define	RKER_PGE	0004000		/* programming error */
#define	RKER_NXM	0002000		/* nonexistent memory */
#define	RKER_DLT	0001000		/* data late */
#define	RKER_TE		0000400		/* timing error */
#define	RKER_NXD	0000200		/* nonexistent disk */
#define	RKER_NXC	0000100		/* nonexistent cylinder */
#define	RKER_NXS	0000040		/* nonexistent sector */
/* bits 4-2 are unused */
#define	RKER_CSE	0000002		/* checksum error */
#define	RKER_WCE	0000001		/* write check error */
#define	RKER_BITS	\
"\10\20DRE\17OVR\16WLO\15SKE\14PGE\13NXM\12DLT\11TE\10NXD\
\7NXC\6NXS\2CSE\1WCE"

/* bits in rkcs */
#define	RKCS_ERR	0100000		/* error */
#define	RKCS_HE		0040000		/* hard error */
#define	RKCS_SCP	0020000		/* search complete */
/* bit 12 is unused */
#define	RKCS_INHBA	0004000		/* inhibit bus address increment */
#define	RKCS_FMT	0002000		/* format */
/* bit 9 is unused */
#define	RKCS_SSE	0000400		/* stop on soft error */
#define	RKCS_RDY	0000200		/* control ready */
#define	RKCS_IDE	0000100		/* interrupt on done enable */
/* bits 5-4 are the UNIBUS extension bits */
/* bits 3-1 is the command */
#define	RKCS_GO		0000001		/* go */
#define	RKCS_BITS	\
"\10\20ERR\17HE\16SCP\15INHBA\14FMT\13SSE\12RDY\11IDE\1GO"

/* commands */
#define	RKCS_RESET	0000000		/* control reset */
#define	RKCS_WCOM	0000002		/* write */
#define	RKCS_RCOM	0000004		/* read */
#define	RKCS_WCHK	0000006		/* write check */
#define	RKCS_SEEK	0000010		/* seek */
#define	RKCS_RCHK	0000012		/* read check */
#define	RKCS_DRESET	0000014		/* drive reset */
#define	RKCS_WLCK	0000016		/* write lock */

/* bits in rkda */
/* bits 15-13 are drive select */
/* bits 12-5 are cylinder address */
#define	RKDA_SUR	0000020		/* surface */
/* bits 3-0 are sector address */
