/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rxreg.h	1.2 (2.11BSD GTE) 1995/11/21
 */

struct	rxdevice	{
	short	rxcs;			/* command and status register */
	short	rxdb;			/* multipurpose register: */
#define	rxba	rxdb			/* 	bus address register */
#define rxqa	rxdb			/*	Q22 high-bits */
#define	rxta	rxdb			/*	track address register */
#define	rxsa	rxdb			/*	sector address register */
#define	rxwc	rxdb			/*	word count register */
#define	rxes	rxdb			/*	error and status register */
};

/* bits in rxcs */
#define	RX_ERR		0100000		/* error */
#define	RX_INIT		0040000		/* initialize */

/* bits 13-12 are the extension bits */
#define	RX_RX02		0004000		/* rx02 (read only) */

#define RX_Q22		0002000		/* enable 22-bit mode (DSD MXV-22) */
#define	RX_HD		0001000		/* DSD 480 head select */
#define	RX_DD		0000400		/* double density */
#define	RX_XREQ		0000200		/* transfer request */
#define	RX_IE		0000100		/* interrupt enable */
#define	RX_DONE		0000040		/* done */
#define	RX_UNIT		0000020		/* unit select */

/* bits 3-1 are the function code */
#define	RX_GO		0000001		/* go */

/* function codes */
#define	RX_FILL		0000000		/* fill buffer */
#define	RX_EMPTY	0000002		/* empty */
#define	RX_WSECT	0000004		/* write sector */
#define	RX_RSECT	0000006		/* read sector */
#define	RX_SMD		0000010		/* set media density */
#define	RX_RDSTAT	0000012		/* read status */
#define	RX_WDDSECT	0000014		/* write deleted data sector */
#define	RX_RDEC		0000016		/* read error code */

/* ioctls */
#define	RXIOC_FORMAT	_IO(r, 1)	/* format media */

#define	RX_BITS	\
"\10\20ERR\17INIT\14RX02\13QBUS\12HD\11DD\10XREQ\7IE\6DONE\5UNIT1\1GO"

/* bits in rxes */
/* bits 15-12 are unused in the standard rx11 */
#define	RXES_IBMDD	0010000		/* DSD 480 IBM double density select */
#define	RXES_NXM	0004000		/* nonexistent memory */
#define	RXES_WCOVFL	0002000		/* word count overflow */

/* bit 9 is unused in the standard rx11 */
#define	RXES_HD		0001000		/* DSD 480 head select */
#define	RXES_UNIT	0000400		/* unit select */
#define	RXES_RDY	0000200		/* ready */
#define	RXES_DDATA	0000100		/* deleted data */
#define	RXES_DD		0000040		/* double density */
#define	RXES_DENSERR	0000020		/* density error */
#define	RXES_ACLO	0000010		/* ac low */
#define	RXES_INITDONE	0000004		/* initialization done */

/* bit 1 is unused in the standard rx11 */
#define	RXES_S1RDY	0000002		/* DSD 480 side 1 ready */
#define	RXES_CRC	0000001		/* crc error */
#define	RXES_BITS	\
"\10\15IBMDD\14NXM\13WCOVFL\12HD\11UNIT1\10RDY\7DDATA\
\6DDENS\5DENSERR\4ACLO\3INIT\2S1RDY\1CRC"

/* bits in rxes1 */
/* bits 15-8 contain the word count */
/* bits 7-0 contain the error code */
#define	RXES1_D0NOHOME	0000010		/* drive 0 failed to see home on init */
#define	RXES1_D1NOHOME	0000020		/* drive 1 failed to see home on init */
#define	RXES1_XTRK	0000040		/* track number > 076 */
#define	RXES1_WRAP	0000050		/* found home before desired track */
#define	RXES1_HNF	0000070		/* header not found after 2 revs */
#define	RXES1_NSEP	0000110		/* up controller found not SEP clock */
#define	RXES1_NOPREAMB	0000120		/* preamble not found */
#define	RXES1_NOID	0000130		/* preamble found;ID burst timeout */
#define	RXES1_HNEQTRK	0000150		/* track reached doesn't match header */
#define	RXES1_XIDAM	0000160		/* up made to many attempts for IDAM */
#define	RXES1_NOAM	0000170		/* data AM timeout */
#define	RXES1_CRC	0000200		/* crc error reading disk sector */
#define	RXES1_OOPS	0000220		/* read/write electronics failed test */
#define	RXES1_WCOVFL	0000230		/* word count overflow */
#define	RXES1_DENSERR	0000240		/* density error */
#define	RXES1_BADKEY	0000250		/* bad key word for Set Media Density */

/* bits in rxes4 */
/* bits 15-8 contain the track address for header track address errors */
#define	RXES4_UNIT	0000200		/* unit select */
#define	RXES4_D1DENS	0000100		/* drive 1 density */
#define	RXES4_HEAD	0000040		/* head loaded */
#define	RXES4_D0DENS	0000020		/* drive 0 density */

/* bits 3-1 are unused */
#define	RXES4_DD	0000001		/* diskette is double density */
#define	RXES4_BITS	"\10\10DRIVE1\7D1HIDENS\6HEAD\5D0HIDENS\1DDENS"

#define	b_seccnt	av_back
#define	b_state		b_active
