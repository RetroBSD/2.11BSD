/*
 * SCCSID: @(#)if_dereg.h	1.1	(2.11BSD GTE)	12/31/93
 *
 * DEC DEUNA interface
 */

/*
 * Define unibus ehternet controller types
 */
#define	DEUNA 0
#define	DELUA 1

struct dedevice {
	union {
		short	p0_w;
		char	p0_b[2];
	} u_p0;
#define	pcsr0		u_p0.p0_w
#define	pclow		u_p0.p0_b[0]
#define	pchigh		u_p0.p0_b[1]
	short	pcsr1;
	short	pcsr2;
	short	pcsr3;
};

/*
 * PCSR 0 bit descriptions
 */
#define	PCSR0_SERI	0x8000		/* Status error interrupt */
#define	PCSR0_PCEI	0x4000		/* Port command error interrupt */
#define	PCSR0_RXI	0x2000		/* Receive done interrupt */
#define	PCSR0_TXI	0x1000		/* Transmit done interrupt */
#define	PCSR0_DNI	0x0800		/* Done interrupt */
#define	PCSR0_RCBI	0x0400		/* Receive buffer unavail intrpt */
#define	PCSR0_FATI	0x0100		/* Fatal error interrupt */
#define	PCSR0_INTR	0x0080		/* Interrupt summary */
#define	PCSR0_INTE	0x0040		/* Interrupt enable */
#define	PCSR0_RSET	0x0020		/* DEUNA reset */
#define	PCSR0_CMASK	0x000f		/* command mask */

#define	PCSR0_BITS	"\20\20SERI\17PCEI\16RXI\15TXI\14DNI\13RCBI\11FATI\10INTR\7INTE\6RSET"

/* bits 0-3 are for the PORT_COMMAND */
#define	CMD_NOOP	0x0
#define	CMD_GETPCBB	0x1		/* Get PCB Block */
#define	CMD_GETCMD	0x2		/* Execute command in PCB */
#define	CMD_STEST	0x3		/* Self test mode */
#define	CMD_START	0x4		/* Reset xmit and receive ring ptrs */
#define	CMD_BOOT	0x5		/* Boot DEUNA */
#define	CMD_PDMD	0x8		/* Polling demand */
#define	CMD_TMRO	0x9		/* Sanity timer on */
#define	CMD_TMRF	0xa		/* Sanity timer off */
#define	CMD_RSTT	0xb		/* Reset sanity timer */
#define	CMD_STOP	0xf		/* Suspend operation */

/*
 * PCSR 1 bit descriptions
 */
#define	PCSR1_XPWR	0x8000		/* Transceiver power BAD */
#define	PCSR1_ICAB	0x4000		/* Interconnect cabling BAD */
#define	PCSR1_STCODE	0x3f00		/* Self test error code */
#define	PCSR1_PCTO	0x0080		/* Port command timed out */
#define	PCSR1_DEVID	0x0070		/* Device id DEUNA=0, DELUA=1 */ 
#define	PCSR1_STMASK	0x0007		/* State */

/* bit 0-3 are for STATE */
#define	STAT_RESET	0x0
#define	STAT_PRIMLD	0x1		/* Primary load */
#define	STAT_READY	0x2
#define	STAT_RUN	0x3
#define	STAT_UHALT	0x5		/* UNIBUS halted */
#define	STAT_NIHALT	0x6		/* NI halted */
#define	STAT_NIUHALT	0x7		/* NI and UNIBUS Halted */

#define	PCSR1_BITS	"\20\20XPWR\17ICAB\10PCTO"
#define	PCSR1_BITS_DELUA "\10\10PCTO"

/*
 * Port Control Block Base
 */
struct de_pcbb {
	short	pcbb0;		/* function */
	short	pcbb2;		/* command specific */
	short	pcbb4;
	short	pcbb6;
};

/* PCBB function codes */
#define	FC_NOOP		0x00		/* NO-OP */
#define	FC_LSUADDR	0x01		/* Load and start microaddress */
#define	FC_RDDEFAULT	0x02		/* Read default physical address */
#define	FC_RDPHYAD	0x04		/* Read physical address */
#define	FC_WTPHYAD	0x05		/* Write physical address */
#define	FC_RDMULTI	0x06		/* Read multicast address list */
#define	FC_WTMULTI	0x07		/* Read multicast address list */
#define	FC_RDRING	0x08		/* Read ring format */
#define	FC_WTRING	0x09		/* Write ring format */
#define	FC_RDCNTS	0x0a		/* Read counters */
#define	FC_RCCNTS	0x0b		/* Read and clear counters */
#define	FC_RDMODE	0x0c		/* Read mode */
#define	FC_WTMODE	0x0d		/* Write mode */
#define	FC_RDSTATUS	0x0e		/* Read port status */
#define	FC_RCSTATUS	0x0f		/* Read and clear port status */
#define	FC_DUMPMEM	0x10		/* Dump internal memory */
#define	FC_LOADMEM	0x11		/* Load internal memory */
#define	FC_RDSYSID	0x12		/* Read system ID parameters */
#define	FC_WTSYSID	0x13		/* Write system ID parameters */
#define	FC_RDSERAD	0x14		/* Read load server address */
#define	FC_WTSERAD	0x15		/* Write load server address */

/*
 * Unibus Data Block Base (UDBB) for ring buffers
 */
struct de_udbbuf {
	short	b_tdrbl;	/* Transmit desc ring base low 16 bits */
	char	b_tdrbh;	/* Transmit desc ring base high 2 bits */
	char	b_telen;	/* Length of each transmit entry */
	short	b_trlen;	/* Number of entries in the XMIT desc ring */
	short	b_rdrbl;	/* Receive desc ring base low 16 bits */
	char	b_rdrbh;	/* Receive desc ring base high 2 bits */
	char	b_relen;	/* Length of each receive entry */
	short	b_rrlen;	/* Number of entries in the RECV desc ring */
};

/*
 * Transmit/Receive Ring Entry
 */
struct de_ring {
	short	r_slen;			/* Segment length */
	short	r_segbl;		/* Segment address (low 16 bits) */
	char	r_segbh;		/* Segment address (hi 2 bits) */
	u_char	r_flags;		/* Status flags */
	u_short	r_tdrerr;		/* Errors */
#define	r_lenerr	r_tdrerr
	short	r_rid;			/* Request ID */
};

#define	XFLG_OWN	0x80		/* If 0 then owned by driver */
#define	XFLG_ERRS	0x40		/* Error summary */
#define	XFLG_MTCH	0x20		/* Address match on xmit request */
#define	XFLG_MORE	0x10		/* More than one entry required */
#define	XFLG_ONE	0x08		/* One collision encountered */
#define	XFLG_DEF	0x04		/* Transmit deferred */
#define	XFLG_STP	0x02		/* Start of packet */
#define	XFLG_ENP	0x01		/* End of packet */

#define	XFLG_BITS	"\10\10OWN\7ERRS\6MTCH\5MORE\4ONE\3DEF\2STP\1ENP"

#define	XERR_BUFL	0x8000		/* Buffer length error */
#define	XERR_UBTO	0x4000		/* UNIBUS tiemout
#define	XERR_UFLO	0x2000		/* Underflow transmit */
#define	XERR_LCOL	0x1000		/* Late collision */
#define	XERR_LCAR	0x0800		/* Loss of carrier */
#define	XERR_RTRY	0x0400		/* Failed after 16 retries */
#define	XERR_TDR	0x03ff		/* TDR value */

#define	XERR_BITS	"\20\20BUFL\17UBTO\16UFLO\15LCOL\14LCAR\13RTRY"

#define	RFLG_OWN	0x80		/* If 0 then owned by driver */
#define	RFLG_ERRS	0x40		/* Error summary */
#define	RFLG_FRAM	0x20		/* Framing error */
#define	RFLG_OFLO	0x10		/* Message overflow */
#define	RFLG_CRC	0x08		/* CRC error */
#define	RFLG_STP	0x02		/* Start of packet */
#define	RFLG_ENP	0x01		/* End of packet */

#define	RFLG_BITS	"\10\10OWN\7ERRS\6FRAM\5OFLO\4CRC\2STP\1ENP"

#define	RERR_BUFL	0x8000		/* Buffer length error */
#define	RERR_UBTO	0x4000		/* UNIBUS tiemout */
#define	RERR_NCHN	0x2000		/* No data chaining */
#define	RERR_OVRN	0x1000		/* overrun message error */
#define	RERR_MLEN	0x0fff		/* Message length */

#define	RERR_BITS	"\20\20BUFL\17UBTO\16NCHN\15OVRN"

/* mode description bits */
#define	MOD_HDX		0x0001		/* Half duplex mode */
#define	MOD_LOOP	0x0004		/* Enable loopback */
#define	MOD_DTCR	0x0008		/* Disables CRC generation */
#define	MOD_INTL	0x0040		/* Internal loopback enable */
#define	MOD_DMNT	0x0200		/* Disable maintenance features */
#define	MOD_ECT		0x0400		/* Enable collision test */
#define	MOD_TPAD	0x1000		/* Transmit message pad enable */
#define	MOD_DRDC	0x2000		/* Disable data chaining */
#define	MOD_ENAL	0x4000		/* Enable all multicast */
#define	MOD_PROM	0x8000		/* Enable promiscuous mode */

struct	de_buf {
	struct ether_header db_head;	/* header */
	char	db_data[ETHERMTU];	/* packet data */
	long	db_crc;			/* CRC - on receive only */
};

#ifdef	DE_DO_PHYSADDR
/*
 * structure used to query de and qe for physical addresses
 */
struct ifdevea {
        char    ifr_name[IFNAMSIZ];             /* if name, e.g. "en0" */
        u_char default_pa[6];                   /* default hardware address */
        u_char current_pa[6];                   /* current physical address */
};
#endif		/* DE_DO_PHYSADDR */

#ifdef	DE_DO_BCTRS
/*
 * Counter list
 */
struct de_counters {
	short	c_length;		/* returned data block length */
	u_short	c_seconds;		/* seconds since last zeroed */
	u_short	c_prcvd[2];		/* packets received */
	u_short	c_mprcvd[2];		/* multicast packets received */
	u_short	c_rbm;			/* receive error bitmap */
	u_short	c_rcverr;		/* packets received with error */
	u_short	c_brcvd[2];		/* bytes received */
	u_short	c_mbrcvd[2];		/* multicast bytes received */
	u_short	c_ibuferr;		/* packets lost - internal buffer error */
	u_short	c_lbuferr;		/* packets lost - local buffer error */
	u_short	c_psent[2];		/* packets sent */
	u_short	c_mpsent[2];		/* multicast packets sent */
	u_short	c_multiple[2];		/* packets sent - multiple collisions */
	u_short	c_single[2];		/* packets sent - single collision */
	u_short	c_defer[2];		/* packets sent - initially deferred */
	u_short	c_bsent[2];		/* bytes sent */
	u_short	c_mbsent[2];		/* multicast bytes sent */
	u_short	c_sbm;			/* send error bitmap */
	u_short	c_snderr;		/* send packet errors */
	u_short	c_collis;		/* collision check failure */
	u_short	c_rsvd;			/* reserved field */
};

/*
 * interface statistics structures
 */
struct estat {				/* Ethernet interface statistics */
	u_short	est_seconds;		/* seconds since last zeroed */
	u_long	est_byrcvd;		/* bytes received */
	u_long	est_bysent;		/* bytes sent */
	u_long	est_blrcvd;		/* data blocks received */
	u_long	est_blsent;		/* data blocks sent */
	u_long	est_mbyrcvd;		/* multicast bytes received */
	u_long	est_mblrcvd;		/* multicast blocks received */
	u_long	est_deferred;		/* blocks sent, initially deferred */
	u_long	est_single;		/* blocks sent, single collision */
	u_long	est_multiple;		/* blocks sent, multiple collisions */
	u_short	est_sfbm;		/*	0 - Excessive collisions */
					/*	1 - Carrier check failed */
					/*	2 - Short circuit */
					/*	3 - Open circuit */
					/*	4 - Frame too long */
					/*	5 - Remote failure to defer */
	u_short	est_sf;			/* send failures: (bit map)*/
	u_short	est_collis;		/* Collision detect check failure */
	u_short	est_rfbm;		/*	0 - Block check error */
					/*	1 - Framing error */
					/*	2 - Frame too long */
	u_short	est_rf;			/* receive failure: (bit map) */
	u_short	est_unrecog;		/* unrecognized frame destination */
	u_short	est_overrun;		/* data overrun */
	u_short	est_sysbuf;		/* system buffer unavailable */
	u_short	est_userbuf;		/* user buffer unavailable */
};

/*
 * interface counter ioctl request
 */
struct ctrreq {
	char	ctr_name[IFNAMSIZ];	/* if name */
	char	ctr_type;		/* type of interface */
	struct estat ctr_ether;/* ethernet counters */
};

#define CTR_ETHER 0			/* Ethernet interface */
#define CTR_DDCMP 1			/* DDCMP pt-to-pt interface */
#define CTR_HDRCRC	0		/* header crc bit index */
#define CTR_DATCRC	1		/* data crc bit index */
#define CTR_BUFUNAVAIL	0		/* buffer unavailable bit index */
#endif		/* DE_DO_BCTRS */
