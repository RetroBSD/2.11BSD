struct	dcdevice	{
	short	dcrcsr;			/* receiver status register */
	short	dcrbuf;
	short	dctcsr;
	short	dctbuf;
};

/* bits in dcrcsr */
#define	DC_ERR		0100000			/* error */
#define	DC_CTR		0040000			/* carrier transition */
#define	DC_RING		0020000			/* ring indicator */
#define	DC_OVERRUN	0010000			/* data overrun */
/* bit 11 is unused */
/* bits 10-9 are the character length */
#define	DC_SXDATA	0000400			/* superv. transmit data */
#define	DC_DONE		0000200			/* done */
#define	DC_IE		0000100			/* interrupt enable */
#define	DC_PCHK		0000040			/* parity check */
/* bits 4-3 are the receiver speed */
#define	DC_CAR		0000004			/* carrier detect */
#define	DC_BRK		0000002			/* break */
#define	DC_DTR		0000001			/* data terminal ready */
#define	DC_BITS		\
"\10\20ERR\17CTR\16RING\15OVERRUN\11SXDATA\10DONE\7IE\6PCHK\3CAR\2BRK\1DTR"

/* character lengths */
#define	DC_8BITS	0000000			/* 8 bits per character */
#define	DC_7BITS	0001000			/* 7 bits per character */
#define	DC_6BITS	0002000			/* 6 bits per character */
#define	DC_5BITS	0003000			/* 5 bits per character */

/* receiver (and transmitter) speeds */
#define	DC_SPEED0	0000000			/* lowest */
#define	DC_SPEED1	0000010
#define	DC_SPEED2	0000020
#define	DC_SPEED3	0000030			/* highest */

/* bits in dctcsr */
#define	DCTCSR_SRDATA	0100000			/* supervisory receive data */
/* bits 14-9 are unused */
#define	DCTCSR_STOP1	0000400			/* stop code:  0= 2 stop bits */
#define	DCTCSR_RDY	0000200			/* ready */
#define	DCTCSR_TIE	0000100			/* transmit interrupt enable */
/* bit 5 is unused */
/* bits 4-3 are the transmitter speed select */
#define	DCTCSR_MM	0000004			/* maintenance */
#define	DCTCSR_CTS	0000002			/* clear to send */
#define	DCTCSR_RTS	0000001			/* request to send */
#define	DCTCSR_BITS	"\10\20SRDATA\11STOP\10RDY\7TIE\3MM\2CTS\1RTS"
