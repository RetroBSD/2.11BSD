/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cons.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *  KL11/DL11 registers and bits
 */
struct dldevice
{
	short	dlrcsr;
	short	dlrbuf;
	short	dlxcsr;
	short	dlxbuf;
};

/* bits in dlrcsr */
#define	DL_DSC		0100000		/* data status change (RO) */
#define	DL_RNG		0040000		/* ring indicator (RO) */
#define	DL_CTS		0020000		/* clear to send (RO) */
#define	DL_CD		0010000		/* carrier detector (RO) */
#define	DL_RA		0004000		/* receiver active (RO) */
#define	DL_SRD		0002000		/* secondary received data (RO) */
/* bits 9-8 are unused */
#define	DL_RDONE	0000200		/* receiver done (RO) */
#define	DL_RIE		0000100		/* receiver interrupt enable */
#define	DL_DIE		0000040		/* dataset interrupt enable */
/* bit 4 is unused */
#define	DL_STD		0000010		/* secondary transmitted data */
#define	DL_RTS		0000004		/* request to send */
#define	DL_DTR		0000002		/* data terminal ready */
#define	DL_RE		0000001		/* reader enable (write only) */
#define	DL_BITS		\
"\10\20DSC\17RNG\16CTS\15CD\14RA\13SRD\10RDONE\7RIE\6DIE\4STD\3RTS\2DTR\1RE"

/* bits in dlrbuf */
#define	DLRBUF_ERR	0100000		/* error (RO) */
#define	DLRBUF_OVR	0040000		/* overrun (RO) */
#define	DLRBUF_FRE	0020000		/* framing error (RO) */
#define	DLRBUF_RDPE	0010000		/* receive data parity error (RO) */
#define	DLRBUF_BITS	\
"\10\20ERR\17OVR\16FRE\15RDPE"

/* bits in dlxcsr */
/* bits 15-8 are unused */
#define	DLXCSR_TRDY	0000200		/* transmitter ready (RO) */
#define	DLXCSR_TIE	0000100		/* transmitter interrupt enable */
/* bits 5-3 are unused */
#define	DLXCSR_MM	0000004		/* maintenance */
/* bit 1 is unused */
#define	DLXCSR_BRK	0000001		/* break */
#define	DLXCSR_BITS	\
"\10\10TRDY\7TIE\3MM\1BRK"
