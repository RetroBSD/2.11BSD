/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sireg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *	Definitions for Systems Industries 9500 controller.
 */

/*
 *	Controller registers and bits
 */
struct sidevice
{
	short	sicnr;		/* control register */
	short	siwcr;		/* word count register */
	short	sipcr;		/* port/cylinder address register */
	short	sihsr;		/* head/sector address register */
	caddr_t	simar;		/* UNIBUS memory address register */
	short	sierr;		/* error register */
	short	sissr;		/* seek status register */
	short	sisar;		/* seek address register */
	short	sidbr;		/* data buffer register (unused) */
	short	sicom;		/* communications register */
	short	siscr;		/* shared computer option register */
};

/* Other bits of sicnr */
#define	SI_OFP		0040000		/* offset+ */
#define	SI_OFM		0020000		/* offset- */
#define	SI_DONE		0000200		/* done */
#define	SI_IE		0000100		/* interrupt enable */
#define	SI_READ		0000004		/* read command */
#define	SI_WRITE	0000002		/* write command */
#define	SI_GO		0000001
#define	SI_RESET	0000000		/* logic master clear */
#define	SI_BITS		\
"\10\10DONE\7IE\3READ\2WRITE\1GO"

/* sierr */
#define	SIERR_ERR	0100000		/* error */
#define SIERR_CNT	0140000		/* contention error */
#define	SIERR_TIMO	0020000		/* timeout error */

#define	SIERR_BITS \
"\10\20ERR\17CNT\16TMO\15SEL\14FLT\13WLK\12BSE\11FMT\10SKI\
\7AVE\6CRC\5VFY\4TMG\2OSE\1OSC"

/* sissr */
#define	SISSR_DONE	010		/* Seek complete */
#define	SISSR_ERR	004		/* Hardware seek error */
#define	SISSR_BUSY	002		/* Busy error */
#define	SISSR_NRDY	001		/* Not ready to seek */

#define	SISSR_BITS \
"\10\20DONE4\17ERR4\16BUSY4\15NRDY4\
\14DONE3\13ERR3\12BUSY3\11NRDY3\
\10DONE2\7ERR2\6BUSY2\5NRDY2\
\4DONE1\3ERR1\2BUSY1\1NRDY1"
