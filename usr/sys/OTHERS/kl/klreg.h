/*
 *  KL11/DL11 registers and bits
 */
struct	dldevice
{
	short	dlrcsr;
	short	dlrbuf;
	short	dlxcsr;
	short	dlxbuf;
};

/* bits in dlrcsr */
#define	DL_DSI		0100000		/* data set interrupt (read only) */
#define	DL_RNG		0040000		/* ring indicator (read only) */
#define	DL_CTS		0020000		/* clear to send (read only) */
#define	DL_CARDET	0010000		/* carrier detector (read only) */
#define	DL_RA		0004000		/* receiver active (read only) */
#define	DL_SRD		0002000		/* secondary received data (read only) */
/* bits 9-8 are unused */
#define	DL_RDONE	0000200		/* receiver done (read only) */
#define	DL_RIE		0000100		/* receiver interrupt enable */
#define	DL_DIE		0000040		/* dataset interrupt enable */
/* bit 4 is unused */
#define	DL_STD		0000010		/* secondary transmitted data */
#define	DL_RTS		0000004		/* request to send */
#define	DL_DTR		0000002		/* data terminal ready */
#define	DL_RE		0000001		/* reader enable (write only) */
#define	DL_BITS		\
"\10\20DSI\17RNG\16CTS\15CARDET\14RA\13SRD\10RDONE\7RIE\6DIE\4STD\3RTS\2DTR\1RE"

/* bits in dlrbuf */
#define	DLRBUF_ERR	0100000		/* error (read only) */
#define	DLRBUF_OVR	0040000		/* overrun (read only) */
#define	DLRBUF_FRE	0020000		/* framing error (read only) */
#define	DLRBUF_RDPE	0010000		/* receive data parity error (read only) */
#define	DLRBUF_BITS	\
"\10\20ERR\17OVR\16FRE\15RDPE"

/* bits in dlxcsr */
/* bits 15-12 select baud rate when bit 11 is high on DLV11-E only */
#define DLXCSR_PSE	0004000		/* programable rate enable for DLV11-E*/
/* bits 10-8 are unused */
#define	DLXCSR_TRDY	0000200		/* transmitter ready (read only) */
#define	DLXCSR_TIE	0000100		/* transmitter interrupt enable */
/* bits 5-3 are unused */
#define	DLXCSR_MM	0000004		/* maintenance */
/* bit 1 is unused */
#define	DLXCSR_BRK	0000001		/* break */
#define	DLXCSR_BITS	\
"\10\10TRDY\7TIE\3MM\1BRK"

#define	DLDELAY		0000004		/* Extra delay for DLs */
