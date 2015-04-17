struct	hsdevice
{
	short	hscs1;		/* Control and Status register 1 */
	short	hswc;		/* Word count register */
	caddr_t	hsba;		/* UNIBUS address register */
	short	hsda;		/* Desired address register */
	short	hscs2;		/* Control and Status register 2 */
	short	hsds;		/* Drive Status */
	short	hser;		/* Error register */
	short	hsas;		/* Attention summary */
	short	hsla;		/* Look ahead register used */
	short	hsdb;		/* Data buffer register */
	short	hsmr;		/* Maintenance register */
	short	hsdt;		/* Drive type register */
	short	hsbae;		/* Bus extension (11/70) */
};

/* bits in hscs1 */
#define	HS_SC		0100000		/* special condition */
#define	HS_TRE		0040000		/* transfer error */
#define	HS_MCPE		0020000		/* MASSBUS control bus parity error */
/* bit 12 is unused */
#define	HS_DVA		0004000		/* drive available */
#define	HS_PSEL		0002000		/* port select */
/* bits 9-8 are the UNIBUS extension bits */
#define	HS_RDY		0000200		/* ready */
#define	HS_IE		0000100		/* interrupt enable */
/* bits 5-1 are the function */
#define	HS_GO		0000001		/* go */
#define	HS_BITS		\
"\10\20SC\17TRE\16MCPE\14DVA\13PSEL\10RDY\7IE\1GO"

/* commands */
#define	HS_NOOP		0000000		/* no operation */
#define	HS_DCLR		0000010		/* drive clear */
#define	HS_SEARCH	0000030		/* search */
#define	HS_WCHK		0000050		/* write check */
#define	HS_WCOM		0000060		/* write */
#define	HS_RCOM		0000070		/* read */

/* bits in hscs2 */
#define	HSCS2_DLT	0100000		/* data late */
#define	HSCS2_WCE	0040000		/* write check error */
#define	HSCS2_PE	0020000		/* parity error */
#define	HSCS2_NED	0010000		/* nonexistent drive */
#define	HSCS2_NEM	0004000		/* nonexistent memory */
#define	HSCS2_PGE	0002000		/* program error */
#define	HSCS2_MXF	0001000		/* missed transfer */
#define	HSCS2_MDPE	0000400		/* MASSBUS data bus parity error */
#define	HSCS2_OR	0000200		/* output ready */
#define	HSCS2_IR	0000100		/* input ready */
#define	HSCS2_CLR	0000040		/* controller clear */
#define	HSCS2_PAT	0000020		/* parity test */
#define	HSCS2_BAI	0000010		/* UNIBUS address increment inhibit */
/* bits 2-0 are unit select */
#define	HSCS2_BITS	\
"\10\20DLT\17WCE\16PE\15NED\14NEM\13PGE\12MXF\11MDPE\10OR\7IR\6CLR\5PAT\4BAI"

/* bits in hsds */
#define	HSDS_ATA	0100000		/* attention active */
#define	HSDS_ERR	0040000		/* error summary */
#define	HSDS_PIP	0020000		/* positioning in progress */
#define	HSDS_MOL	0010000		/* medium on line */
#define	HSDS_WRL	0004000		/* write locked */
#define	HSDS_LBT	0002000		/* last block transferred */
/* bit 9 is unused */
#define	HSDS_DPR	0000400		/* drive present */
#define	HSDS_DRY	0000200		/* drive ready */
/* bits 6-0 are unused */
#define	HSDS_BITS	\
"\10\20ATA\17ERR\16PIP\15MOL\14WRL\13LBT\11DPR\10DRY"

/* bits in hser */
#define	HSER_DCK	0100000		/* data check */
#define	HSER_UNS	0040000		/* unsafe */
#define	HSER_OPI	0020000		/* operation incomplete */
#define	HSER_DTE	0010000		/* drive timing error */
#define	HSER_WLE	0004000		/* write lock error */
#define	HSER_IAE	0002000		/* invalid address error */
#define	HSER_AO		0001000		/* address overflow */
/* bits 8-4 are unused */
#define	HSER_PAR	0000010		/* bus parity error */
#define	HSER_RMR	0000004		/* register modify refused */
#define	HSER_ILR	0000002		/* illegal register */
#define	HSER_ILF	0000001		/* illegal function */
#define	HSER_BITS	\
"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IAE\12AO\4PAR\3RMR\2ILR\1ILF"

/* bits in hsdt */
/* bits 15-9 always read as 0 */
#define	HSDT_HS03	0000000		/* RS03 */
#define	HSDT_HS03SI	0000001		/* RS03 with sector interleave */
#define	HSDT_HS04	0000002		/* RS04 */
#define	HSDT_HS04SI	0000003		/* RS04 with sector interleave */
