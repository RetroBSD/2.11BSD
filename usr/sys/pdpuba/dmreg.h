/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dmreg.h	1.1 (Berkeley) 12/1/86
 */

/*
 * DM-11 device register definitions.
 */
struct dmdevice {
	short	dmcsr;		/* control status register */
	union	{               /* Q22:Need to access high byte independently */
		short dmlst_w;
		char dmlst_b[2];
		} cs02un;
#define dmlstat cs02un.dmlst_w        /* line status register */
#define dmlst_l cs02un.dmlst_b[0]     /* low byte */
#define dmlst_h cs02un.dmlst_b[1]     /* high byte */
	short   dmpad1[2];
};

/* bits in dm csr */
#define	DM_RF		0100000		/* ring flag */
#define	DM_CF		0040000		/* carrier flag */
#define	DM_CTS		0020000		/* clear to send */
#define	DM_SRF		0010000		/* secondary receive flag */
#define	DM_CS		0004000		/* clear scan */
#define	DM_CM		0002000		/* clear multiplexor */
#define	DM_MM		0001000		/* maintenance mode */
#define	DM_STP		0000400		/* step */
#define	DM_DONE		0000200		/* scanner is done */
#define	DM_IE		0000100		/* interrupt enable */
#define	DM_SE		0000040		/* scan enable */
#define	DM_BUSY		0000020		/* scan busy */

/* bits in dm lsr */
#define	DML_RNG		0000200		/* ring */
#define	DML_CAR		0000100		/* carrier detect */
#define	DML_CTS		0000040		/* clear to send */
#define	DML_SR		0000020		/* secondary receive */
#define	DML_ST		0000010		/* secondary transmit */
#define	DML_RTS		0000004		/* request to send */
#define	DML_DTR		0000002		/* data terminal ready */
#define	DML_LE		0000001		/* line enable */

#define	DML_ON		(DML_DTR|DML_RTS|DML_LE)
#define	DML_OFF		(DML_LE)
