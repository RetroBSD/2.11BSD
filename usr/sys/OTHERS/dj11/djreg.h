struct	djdevice	{
	short	djcsr;
	short	djrbuf;
	short	djtcr;
#define	djbcr	djtcr
	struct	{
		char	djtbufl;
		char	djtbufh;
	} dj_un;
};

/* bits in djcsr */
#define	DJ_TRDY		0100000		/* transmitter ready */
#define	DJ_TIE		0040000		/* transmitter interrupt enable */
#define	DJ_FIFO		0020000		/* FIFO full */
#define	DJ_BSIE		0010000		/* FIFO buffer status interrupt enable*/
/* bit 11 is unused */
#define	DJ_BRS		0002000		/* break register select */
/* bit 9 is unused */
#define	DJ_MTSE		0000400		/* master transmit scan enable */
#define	DJ_RDONE	0000200		/* receiver done */
#define	DJ_RIE		0000100		/* receiver interrupt enable */
/* bit 5 is unused */
#define	DJ_BCLR		0000020		/* busy clear */
#define	DJ_MCLR		0000010		/* MOS clear */
#define	DJ_MM		0000004		/* maintenance mode */
#define	DJ_HDX		0000002		/* half duplex select */
#define	DJ_RE		0000001		/* receiver enable */
#define	DJ_BITS		\
"\10\20TRDY\17TIE\16FIFO\15BSIE\13BRS\11MTSE\10RDONE\
\7RIE\5BCLR\4MCLR\3MM\2HDX\1RE"

/* bits in djrbuf */
#define	DJRBUF_DPR	0100000		/* data present */
#define	DJRBUF_OVERRUN	0040000		/* overrun */
#define	DJRBUF_FE	0020000		/* framing error */
#define	DJRBUF_RDPE	0010000		/* received data parity error */
/* bits 11-8 are the line number */
/* bits 7-0 are the received data */
#define	DJRBUF_BITS	"\10\20DPR\17OVERRUN\16FE\15RDPE"
