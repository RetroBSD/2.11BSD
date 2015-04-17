struct	dudevice	{
	short	durcsr;		/* receiver status register */
	short	durdbuf;	/* receiver data buffer register */
#define	duparcsr	durdbuf	/* (parameter control register on write) */
	short	dutcsr;		/* transmitter status register */
	short	dutdbuf;	/* transmitter data buffer register */
};

/* bits in durcsr */
#define	DU_DSCHG	0100000		/* data set change in du11 */
#define	DU_DSCHGA	DU_DSCHG	/* data set change A in dup11 */
#define	DU_RING		0040000		/* ring indicator */
#define	DU_CTS		0020000		/* clear to send */
#define	DU_CAR		0010000		/* carrier */
#define	DU_RACT		0004000		/* receiver active */
#define	DU_SRDATA	0002000		/* secondary received data */
#define	DU_DSRDY	0001000		/* data set ready */
#define	DU_STSYNC	0000400		/* strip sync */
#define	DU_RDONE	0000200		/* receiver done */
#define	DU_RIE		0000100		/* receiver interrupt enable */
#define	DU_DSIE		0000040		/* data set interrupt enable */
#define	DU_SSYNC	0000020		/* search sync in du11 */
#define	DU_RCVEN	DU_SSYNC	/* receiver interrupt enable in dup11 */
#define	DU_STD		0000010		/* secondary transmit data */
#define	DU_RTS		0000004		/* request to send */
#define	DU_DTR		0000002		/* data terminal ready */
/* bit 0 is unused in du11 */
#define	DU_DSCHGB	0000001		/* data set change B in dup11 */
#define	DU_BITS		\
"\10\20DSCHG\17RING\16CTS\15CAR\14RACT\13SRDATA\12DSRDY\11STSYNC\10RDONE\
\7RIE\6DSIE\5SSYNC\4STD\3RTS\2DTR"
#define	DU_PBITS	\
"\10\20DSCHGA\17RING\16CTS\15CAR\14RACT\13SRDATA\12DSRDY\11STSYNC\10RDONE\
\7RIE\6DSIE\5RCVEN\4STD\3RTS\2DTR\1DSCHGB"

/* bits in durdbuf */
#define	DURDBUF_ERR	0100000		/* error */
#define	DURDBUF_OVERRUN	0040000		/* overrun */
/* bit 13 is unused in dup11 */
#define	DURDBUF_FE	0020000		/* framing error in du11 */
#define	DURDBUF_PE	0010000		/* parity error in du11 */
#define	DURDBUF_RCRC	DURDBUF_PE	/* RCRC error + Zero in dup11 */
/* bit 11 is unused */
/* bits 10-8 are unused in dup11 */
#define	DURDBUF_RABORT	0002000		/* received ABORT in dup11 */
#define	DURDBUF_REOM	0001000		/* end of received message in dup11 */
#define	DURDBUF_RSOM	0000400		/* start of received message in dup11 */
/* bits 7-0 are the receiver data buffer */
#define	DURDBUF_BITS	"\10\20ERR\17OVERRUN\16\FE\15PE"
#define	DURDBUF_PBITS	"\10\20ERR\17OVERRUN\15RCRC\13RABORT\12REOM\11RSOM"

/* bits in duparcsr */
/* bit 15 is unused in du11 */
#define	DUPARCSR_DEC	0100000		/* DEC mode in dup11 */
/* bit 14 is unused */
/* bit 13 is unused in dup11 */
/* bits 13-12 are mode select in du11 */
#define	DUPARCSR_SMSEL	0010000		/* secondary mode select in dup11 */
/* bits 11-10 are word length select in du11 and unused in dup11 */
#define	DUPARCSR_PE	0001000		/* parity error in du11 */
#define	DUPARCSR_CRCINH	DUPARCSR_PE	/* CRC inhibit in dup11 */
/* bit 8 is unused in dup11 */
#define	DUPARCSR_EVEN	0000400		/* select even parity in du11 */
/* bits 7-0 are the sync register */
#define	DUPARCSR_BITS	"\10\12PE\11EVEN"
#define	DUPARCSR_PBITS	"\10\20DEC\15SMSEL\12CRCINH"

/* duparcsr mode select bits in du11 */
#define	DUPARCSR_XSYNC	0020000		/* synchronous external */
#define	DUPARCSR_ISYNC	0030000		/* synchronous internal */

/* duparcsr word length select bits in du11 */
#define	DUPARCSR_8BITS	0006000		/* eight bits per character */
#define	DUPARCSR_7BITS	0004000		/* seven bits per character */
#define	DUPARCSR_6BITS	0002000		/* six bits per character */
#define	DUPARCSR_5BITS	0000000		/* five bits per character */

/* bits in dutcsr */
#define	DUTCSR_NODATA	0100000		/* data not available in du 11 */
#define	DUTCSR_DATALATE	DUTCSR_NODATA	/* data late error in dup11 */
#define	DUTCSR_MD	0040000		/* maintenance data */
#define	DUTCSR_MC	0020000		/* maintenance clock */
/* bits 12-11 are maintenance mode select in du11*/
/* bits 12-11 are maintenance mode select A and B in dup11*/
/* bit 10 is called ``maintenance bit window'' in earlier du11 manuals */
#define	DUTCSR_RI	0002000		/* receiver input in du11 */
#define	DUTCSR_MMSELECT	DUTCSR_RI	/* maintenance mode select A and B in dup11 */
/* bit 9 is unused in du11 */
#define	DUTCSR_TACT	0001000		/* transmitter active in dup11 */
#define	DUTCSR_MR	0000400		/* master reset */
#define	DUTCSR_TDONE	0000200		/* transmitter done */
#define	DUTCSR_TIE	0000100		/* transmitter interrupt enable */
/* bit 5 is unused in dup11 */
#define	DUTCSR_DNAIE	0000040		/* data not available interrupt enable in du11 */
#define	DUTCSR_SEND	0000020		/* send */
#define	DUTCSR_HDX	0000010		/* half/full duplex */
/* bits 2-1 are unused */
/* bit 0 is unused in dup11 */
#define	DUTCSR_BRK	0000001		/* break in du11 */
#define	DUTCSR_BITS	\
"\10\20NODATA\17MD\16MC\13RI\11MR\10TDONE\7TIE\6DNAIE\5SEND\4HDX\1BRK"
#define	DUTCSR_PBITS	\
"\10\20DATALATE\17MD\16MC\13MMSELECT\12TACT\11MR\10TDONE\7TIE\5SEND\4HDX"

/* dutcsr maintenance mode select bits in du11 */
#define	DUTCSR_MSYSTST	0014000		/* internal mode for systems testing */
#define	DUTCSR_MEXTERN	0010000		/* external */
#define	DUTCSR_MINTERN	0004000		/* internal */
#define	DUTCSR_MNORMAL	0000000		/* normal */

/* dutcsr maintenance mode select A and B bits in dup11 (some are the same) */
#define	DUTCSR_MPSYSTST	DUTCSR_MINTERN
#define	DUTCSR_MPINTERN	DUTCSR_MSYSTST
