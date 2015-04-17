struct	rfdevice {
	short	rfdcs;		/* disk control status register */
	short	rfwc;		/* word count register */
	char	*rfcma;		/* current memory address register */
	short	rfdar;		/* disk address register */
	short	rfdae;		/* disk address ext. and error register */
	short	rfdbr;		/* disk data buffer register */
	short	rfmar;		/* maintenance register */
	short	rfads;		/* address of disk segment register */
};

/* bits in rfcs */
#define	RF_ERR		0100000			/* error */
#define	RF_FRZ		0040000			/* freeze */
#define	RF_WCE		0020000			/* write check error */
#define	RF_DPE		0010000			/* data parity error */
#define	RF_NED		0004000			/* nonexistent drive */
#define	RF_WLO		0002000			/* write lockout */
#define	RF_MXF		0001000			/* missed transfer */
#define	RF_CTLCLR	0000400			/* disk control init */
#define	RF_RDY		0000200			/* controller ready */
#define	RF_IENABLE	0000100
/* bits 5 and 4 are the memory extension bits */
#define	RF_MA		0000010			/* maintenance mode */
/* bits 2 and 1 are the function */
#define	RF_GO		0000001			/* go */

/* function codes */
#define	RF_WCHK		0000006			/* write check */
#define	RF_RCOM		0000004			/* read */
#define	RF_WCOM		0000002			/* write */
#define	RF_NOP		0000000			/* no op */

#define	RF_BITS		\
"\10\20ERR\17FRZ\16WCE\15DPE\14NED\13WLO\12MXF\11CTLCLR\10RDY\7IENABLE\4MA\1GO"

/* bits in rfdae */
#define	RFDAE_APE	0100000			/* address parity error */
#define	RFDAE_ATER	0040000			/* A timing track error */
#define	RFDAE_BTER	0020000			/* B timing track error */
#define	RFDAE_CTER	0010000			/* C timing track error */
/* bit 11 is unused */
#define	RFDAE_NEM	0002000			/* nonexistent memory */
/* bit 9 is unused */
#define	RFDAE_CMA_INH	0000400			/* memory address inhibit */
#define	RFDAE_DRL	0000200			/* data request late */
/* bit 6 is unused */
#define	RFDAE_DA14	0000040			/* disk address -010 */
/* bits 4-2 are the disk address */
/* bits 1-0 are the track address */
#define	RFDAE_BITS	\
"\10\20APE\17ATER\16BTER\15CTER\13NEM\11CMA_INH\10DRL\6DA14"

/* bits in rmar */
/* bits 15-13 are unused */
#define	RFMAR_MDT	0010000			/* maintenance data */
#define	RFMAR_MCT	0004000			/* maintenance C timing */
#define	RFMAR_MBT	0002000			/* maintenance B timing */
#define	RFMAR_MAT	0001000			/* maintenance A timing */
/* bits 7-6 are data track maintenance */
/* bits 5-4 are C timing maintenance */
/* bits 3-2 are B timing maintenance */
/* bits 1-0 are A timing maintenance */
#define	RFMAR_BITS	"\10\15MDT\14MCT\13MBT\12MAT"
