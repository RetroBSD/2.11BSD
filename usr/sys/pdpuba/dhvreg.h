/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dhvreg.h	1.2 (2.11BSD) 1997/5/1
 */

/* 
 * DHV-11 device register definitions.
 */
struct dhvdevice {
	union {
		u_short	csr;		/* control-status register */
		struct {
			char	csrl;	/* low byte for line select */
			char	csrh;	/* high byte for tx line */
		} cb;
	} un1;
#define	dhvcsr	un1.csr
#define	dhvcsrl	un1.cb.csrl
#define	dhvcsrh	un1.cb.csrh
	union {
		u_short	rbuf;		/* recv.char/ds.change register (R) */
		u_short	timo;		/* delay between recv -> intr (W) */
	} un2;
#define	dhvrbuf	un2.rbuf
#define	dhvtimo	un2.timo
	u_short	dhvlpr;			/* line parameter register */
	union {
		char	fbyte[1];	/* fifo data byte (low byte only) (W) */
		u_short	fdata;		/* fifo data word (W) */
		char	sbyte[2];	/* line status/fifo size (R) */
	} un3;
#define	dhvbyte	un3.fbyte[0]
#define dhvfifo	un3.fdata
#define dhvsize	un3.sbyte[0]
#define dhvstat	un3.sbyte[1]
	u_short	dhvlcr;			/* line control register */
	u_short	dhvbar1;		/* buffer address register 1 */
	char	dhvbar2;		/* buffer address register 2 */
	char	dhvlcr2;		/* xmit enable bit */
	short	dhvbcr;			/* buffer count register */
};

/* Bits in dhvcsr */
#define	DHV_CS_TIE	0x4000		/* transmit interrupt enable */
#define	DHV_CS_DFAIL	0x2000		/* diagnostic fail */
#define	DHV_CS_RI	0x0080		/* receiver interrupt */
#define	DHV_CS_RIE	0x0040		/* receiver interrupt enable */
#define	DHV_CS_MCLR	0x0020		/* master clear */
#define	DHV_CS_IAP	0x0007		/* indirect address pointer */

#define	DHV_IE	(DHV_CS_TIE|DHV_CS_RIE)

/* map unit into iap register select */
#define DHV_SELECT(unit)	((unit) & DHV_CS_IAP)

/* Transmitter bits in high byte of dhvcsr */
#define	DHV_CSH_TI	0x80		/* transmit interrupt */
#define	DHV_CSH_NXM	0x10		/* transmit dma err: non-exist-mem */
#define	DHV_CSH_TLN	0x0f		/* transmit line number */

/* map csrh line bits into line */
#define	DHV_TX_LINE(csrh)	((csrh) & DHV_CSH_TLN)

/* Bits in dhvrbuf */
#define	DHV_RB_VALID	0x8000		/* data valid */
#define	DHV_RB_STAT	0x7000		/* status bits */
#define	DHV_RB_DO	0x4000		/* data overrun */
#define	DHV_RB_FE	0x2000		/* framing error */
#define	DHV_RB_PE	0x1000		/* parity error */
#define	DHV_RB_RLN	0x0f00		/* receive line number */
#define	DHV_RB_RDS	0x00ff		/* receive data/status */
#define DHV_RB_DIAG	0x0001		/* if DHV_RB_STAT -> diag vs modem */

/* map rbuf line bits into line */
#define	DHV_RX_LINE(rbuf)	(((rbuf) & DHV_RB_RLN) >> 8)

/* Bits in dhvlpr */
#define	DHV_LP_TSPEED	0xf000
#define	DHV_LP_RSPEED	0x0f00
#define	DHV_LP_TWOSB	0x0080
#define	DHV_LP_EPAR	0x0040
#define	DHV_LP_PENABLE	0x0020
#define	DHV_LP_BITS8	0x0018
#define	DHV_LP_BITS7	0x0010
#define	DHV_LP_BITS6	0x0008

/* Bits in dhvstat */
#define	DHV_ST_DSR	0x80		/* data set ready */
#define	DHV_ST_RI	0x20		/* ring indicator */
#define	DHV_ST_DCD	0x10		/* carrier detect */
#define	DHV_ST_CTS	0x08		/* clear to send */
#define	DHV_ST_DHU	0x01		/* always one on a dhu, zero on dhv */

/* Bits in dhvlcr */
#define	DHV_LC_RTS	0x1000		/* request to send */
#define	DHV_LC_DTR	0x0200		/* data terminal ready */
#define	DHV_LC_MODEM	0x0100		/* modem control enable */
#define	DHV_LC_MAINT	0x00c0		/* maintenance mode */
#define	DHV_LC_FXOFF	0x0020		/* force xoff */
#define	DHV_LC_OAUTOF	0x0010		/* output auto flow */
#define	DHV_LC_BREAK	0x0008		/* break control */
#define	DHV_LC_RXEN	0x0004		/* receiver enable */
#define	DHV_LC_IAUTOF	0x0002		/* input auto flow */
#define	DHV_LC_TXABORT	0x0001		/* transmitter abort */

/* Bits in dhvlcr2 */
#define	DHV_LC2_TXEN	0x80		/* transmitter enable */

/* Bits in dhvbar2 */
#define	DHV_BA2_DMAGO	0x80		/* transmit dma start */
#define	DHV_BA2_XBA	0x03		/* top two bits of dma address */
#define DHV_XBA_SHIFT	16		/* amount to shift xba bits */

/* Bits for dhvmctl only:  stat bits are shifted up 16 */
#define	DHV_ON	(DHV_LC_DTR|DHV_LC_RTS|DHV_LC_MODEM)
#define	DHV_OFF	DHV_LC_MODEM

#define	DHV_DSR	((long)DHV_ST_DSR << 16)
#define	DHV_RNG	((long)DHV_ST_RI << 16)
#define	DHV_CAR	((long)DHV_ST_DCD << 16)
#define	DHV_CTS	((long)DHV_ST_CTS << 16)

#define	DHV_RTS	DHV_LC_RTS
#define	DHV_DTR	DHV_LC_DTR
#define DHV_BRK	DHV_LC_BREAK
#define DHV_LE	DHV_LC_MODEM
