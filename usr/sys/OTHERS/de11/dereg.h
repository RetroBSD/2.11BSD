From likewise!bandit!dlf Wed Dec 18 19:06:48 1985
Received: from likewise.UUCP by seismo.CSS.GOV with UUCP; Wed, 18 Dec 85 15:05:07 EST
Received: by likewise.UUCP (4.12/4.7)
	id AA10578; Wed, 18 Dec 85 11:18:25 est
Received: by bandit.UUCP (4.12/4.7)
	id AA16395; Wed, 18 Dec 85 11:10:38 est
Date: Wed, 18 Dec 85 11:10:38 est
From: likewise!bandit!dlf (mh6692)
Message-Id: <8512181610.AA16395@bandit.UUCP>
To: bandit!likewise!seismo!keith
Subject: include/sys/dereg.h
Status: R

struct	dedevice	{
	short	dercsr;			/* receiver status register */
	short	derbuf;
	short	detcsr;
	short	detbuf;
};

/* bits in dercsr */
#define	DE_DSI		0100000			
#define	DE_RING		0040000			
#define	DE_CTS		0020000			/* clear to send */
#define	DE_CAR		0010000			/* carrier detect */
#define	DE_RCVA		0004000			/* receiver active */
#define	DE_SECR		0002000			/* secondary receive */
/* bits 9-8 are not used */
#define	DE_DONE		0000200			/* receiver done */
#define	DE_RIE		0000100			/* interrupt rcvr enable */
#define	DE_DSIE		0000040			/* data set interrupt enable */
/* bit 4 is not used */
#define	DE_SXDATA	0000010			/* supervisory transmit */
#define	DE_RTS		0000004			/* request to send */
#define	DE_DTR		0000002			/* data terminal ready */
/* bit 0 is not used */

/* bits in derbuf */
#define DE_ERR		0100000			/* any error */
#define DE_OERR		0040000			/* overrun error */
#define DE_FERR		0020000			/* framing error */
#define DE_PERR		0010000			/* parity error */
/* bits in detcsr */
/* bits 15-12 are the transmitter speed select */
#define DE_PBREN	0004000
/* bits 10-8 are not used */
#define	DETCSR_RDY	0000200			/* ready */
#define	DE_TIE		0000100			/* transmit interrupt enable */
/* bits 5-3 are unused */
#define	DETCSR_MM	0000004			/* maintenance */
/* bit one is not used */
#define DE_BRK		0000001			/* break */

#define	DEDELAY		0000004			/* Extra delay for DEs */


