struct brdevice {
	short brds;
	short brer;
	union {
		short w;
		char c[2];
	} brcs;
	short brwc;
	caddr_t brba;
	short brca;
	short brda;
	short brae;
};

/* bits in brds */
#define	BRDS_SURDY	0100000		/* selected unit ready */
#define	BRDS_SUOL	0040000		/* selected unit on line */
#define	BRDS_TOE	0020000		/* selected unit BR */
#define	BRDS_HNF	0010000		/* header not found */
#define	BRDS_SUSI	0004000		/* selected unit seek incomplete */
#define	BRDS_SUFU	0001000		/* selected unit file unsafe */
#define	BRDS_SUWP	0000400		/* selected unit write protected */
/* bits 7-0 are attention bits */
#define	BRDS_BITS	"\10\20SURDY\17SUOL\16BR\15HNF\14SUSI\12SUFU\11SUWP"

/* bits in brer */
#define	BRER_WPV	0100000		/* write protect violation */
#define	BRER_FUV	0040000		/* file unsafe violation */
#define	BRER_NXC	0020000		/* nonexistent cylinder */
#define	BRER_NXT	0010000		/* nonexistent track */
#define	BRER_SUBUSY	0004000		/* selected unit busy */
#define	BRER_PROG	0002000		/* program error */
#define	BRER_FMTE	0001000		/* format error */
#define	BRER_BSE	0000400		/* bad sector */
#define	BRER_ATE	0000200		/* aborted transfer error */
#define	BRER_DCE	0000100		/* data check error */
#define	BRER_DSE	0000040		/* data sync error */
#define	BRER_SBPE	0000020		/* system bad parity error */
#define	BRER_WCE	0000010		/* write check error */
#define	BRER_NXME	0000004		/* nonexistent memory */
#define	BRER_EOP	0000002		/* end of pack */
#define	BRER_DSKERR	0000001		/* disk error */
#define	BRER_BITS	\
"\10\20WPV\17FUV\16NXC\15NXT\14SUBUSY\13PROG\12FMTE\11BSE\10ATE\
\7DCE\6DSE\5SBPE\4WCE\3NXME\2EOP\1DSKERR"

/* bits in brcs */
#define	BR_ERR		0100000		/* error */
#define	BR_HE		0040000		/* hard error */
#define	BR_AIE		0020000		/* attention interrupt enable */
#define	BR_HDB		0010000		/* hold drive busy */
#define	BR_HDR		0004000		/* header */
/* bits 10-8 are drive select */
#define	BR_RDY		0000200		/* ready */
#define	BR_IDE		0000100		/* interrupt on done (error) enable */
/* bits 5-4 are the UNIBUS extension bits */
/* bits 3-1 are the function */
#define	BR_GO		0000001		/* go */
#define	BR_BITS		"\10\20ERR\17HE\16AIE\15HDB\14HDR\10RDY\7IDE\1GO"

/* commands */
#define	BR_IDLE		0000000		/* idle */
#define	BR_WCOM		0000002		/* write */
#define	BR_RCOM		0000004		/* read */
#define	BR_WCHK		0000006		/* write check */
#define	BR_SEEK		0000010		/* seek */
#define	BR_WNS		0000012		/* write (no seek) */
#define	BR_HSEEK	0000014		/* home seek */
#define	BR_RNS		0000016		/* read (no seek) */

#define	STBE		0000100		/* Strobe early */
#define	STBL		0000200		/* Strobe late */
#define	OFFP		0140000		/* Positive offset */
#define	OFFM		0100000		/* Negative offset */

/* Bits in br_ae */
#define	AE_DTYP		0007400		/* Disk type */
#define	AE_T50		0000400		/* T-50  drive */
#define	AE_T80		0001000		/* T-80  drive */
#define	AE_T200		0002000		/* T-200 drive */
#define	AE_T300		0004000		/* T-300 drive */
