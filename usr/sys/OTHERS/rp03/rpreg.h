struct	rpdevice
{
	short	rpds;
	short	rper;
	union	{
		short	w;
		char	c[2];
	} rpcs;
	short	rpwc;
	caddr_t	rpba;
	short	rpca;
	short	rpda;
	short	rpm1;
	short	rpm2;
	short	rpm3;
	short	suca;
	short	silo;
};

/* bits in rpds */
#define	RPDS_SURDY	0100000		/* selected unit ready */
#define	RPDS_SUOL	0040000		/* selected unit on line */
#define	RPDS_RP03	0020000		/* selected unit RP03 */
#define	RPDS_HNF	0010000		/* header not found */
#define	RPDS_SUSI	0004000		/* selected unit seek incomplete */
#define	RPDS_SUSU	0002000		/* selected unit seek underway */
#define	RPDS_SUFU	0001000		/* selected unit file unsafe */
#define	RPDS_SUWP	0000400		/* selected unit write protected */
/* bits 7-0 are attention bits */
#define	RPDS_BITS	\
"\10\20SURDY\17SUOL\16RP03\15HNF\14SUSI\13SUSU\12SUFU\11SUWP"

/* bits in rper */
#define	RPER_WPV	0100000		/* write protect violation */
#define	RPER_FUV	0040000		/* file unsafe violation */
#define	RPER_NXC	0020000		/* nonexistent cylinder */
#define	RPER_NXT	0010000		/* nonexistent track */
#define	RPER_NXS	0004000		/* nonexistent sector */
#define	RPER_PROG	0002000		/* program error */
#define	RPER_FMTE	0001000		/* format error */
#define	RPER_MODE	0000400		/* mode error */
#define	RPER_LPE	0000200		/* longitudinal parity error */
#define	RPER_WPE	0000100		/* word parity error */
#define	RPER_CSME	0000040		/* checksum error */
#define	RPER_TIMEE	0000020		/* timing error */
#define	RPER_WCE	0000010		/* write check error */
#define	RPER_NXME	0000004		/* nonexistent memory */
#define	RPER_EOP	0000002		/* end of pack */
#define	RPER_DSKERR	0000001		/* disk error */
#define	RPER_BITS	\
"\10\20WPV\17FUV\16NXC\15NXT\14NXS\13PROG\12FMTE\11MODE\10LPE\
\7WPE\6CSME\5TIMEE\4WCE\3NXME\2EOP\1DSKERR"

/* bits in rpcs */
#define	RP_ERR		0100000		/* error */
#define	RP_HE		0040000		/* hard error */
#define	RP_ATE		0020000		/* attention interrupt enable */
#define	RP_MODE		0010000		/* mode */
#define	RP_HDR		0004000		/* header */
/* bits 10-8 are drive select */
#define	RP_RDY		0000200		/* ready */
#define	RP_IDE		0000100		/* interrupt on done (error) enable */
/* bits 5-4 are the UNIBUS extension bits */
/* bits 3-1 are the function */
#define	RP_GO		0000001		/* go */
#define	RP_BITS		\
"\10\20ERR\17HE\16ATE\15MODE\14HDR\10RDY\7IDE\1GO"

/* commands */
#define	RP_IDLE		0000000		/* idle */
#define	RP_WCOM		0000002		/* write */
#define	RP_RCOM		0000004		/* read */
#define	RP_WCHK		0000006		/* write check */
#define	RP_SEEK		0000010		/* seek */
#define	RP_WNS		0000012		/* write (no seek) */
#define	RP_HSEEK	0000014		/* home seek */
#define	RP_RNS		0000016		/* read (no seek) */

/* bits in rpm1 */
/* bits 15-13 are unused */
#define	RPM1_SORDY	0010000		/* silo out ready */
#define	RPM1_SIRDY	0004000		/* silo in ready */
#define	RPM1_CONTROL	0002000		/* control */
#define	RPM1_SETHEAD	0001000		/* set head */
#define	RPM1_SETCYL	0000400		/* set cylinder */
/* bits 7-0 are the bus out signals */
#define	RPM1_BITS	\
"\10\15SORDY\14SIRDY\13CONTROL\12SETHEAD\11SETCYL"

/* bits in rpm3 */
#define	RPM3_MRO	0100000		/* maintenance read only */
#define	RPM3_MRDY	0040000		/* maintenance ready */
#define	RPM3_MOL	0020000		/* maintenance on line */
#define	RPM3_MIDX	0010000		/* maintenance index */
#define	RPM3_MFU	0004000		/* maintenance file unsafe */
#define	RPM3_MSI	0002000		/* maintenance seek incomplete */
#define	RPM3_MECYL	0001000		/* maintenance end of cylinder */
#define	RPM3_MSECT	0000400		/* maintenance sector */
/* bits 7-1 are unused */
#define	RPM3_CLK	0000001		/* maintenance clock */
#define	RPM3_BITS	\
"\10\20MRO\17MRDY\16MOL\15MIDX\14MFU\13MSI\12MECYL\11MSECT\1CLK"
