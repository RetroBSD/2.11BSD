struct	mldevice
{
	union {
		short	w;
		char	c[2];
	} mlcs1;			/* Control and Status register 1 */
	short	mlwc;			/* Word count register */
	caddr_t	mlba;			/* UNIBUS address register */
	short	mlda;			/* Desired address register */
	union {
		short	w;
		char	c[2];
	} mlcs2;			/* Control and Status register 2 */
	short	mlds;			/* Drive Status */
	short	mler;			/* Error register */
	short	mlas;			/* Attention Summary */
	short	mlpa;			/* Prom address register */
	short	mldb;			/* Data buffer */
	short	mlmr;			/* Maintenance register */
	short	mldt;			/* Drive type */
	short	mlsn;			/* Serial number */
	short	mle1;			/* ECC CRC word 1 */
	short	mle2;			/* ECC CRC word 2 */
	short	mld1;			/* Data diagnostic 1 register */
	short	mld2;			/* Data diagnostic 2 register */
	short	mlee;			/* ECC error register */
	short	mlel;			/* ECC error location register */
	short	mlpd;			/* Prom data register */
	short	mlbae;			/* RH70 bus address extension */
	short	mlcs3;			/* RH70 control & status register 3 */
};

/* bits in mlcs */
#define	ML_TRE		0040000
#define	ML_IE		0000100
#define	ML_GO		0000001

/* function codes */
#define ML_DCLR		0000010
#define	ML_PRESET	0000020
#define	ML_WCOM		0000060
#define	ML_RCOM		0000070
#define	ML_BITS		"\10\17TRE\7IE\1G0"

/* bits in MLDS */
#define	MLDS_MOL	0010000
#define	MLDS_DPR	0000400
#define	MLDS_DRY	0000200
#define	MLDS_VV		0000100
#define	MLDS_BITS	"\10\15MOL\11DPR\10DRY\7VV"

/* transfer rates */
#define	MLMR_2MB	0000000			/* 2Mb transfer rate */
#define	MLMR_1MB	0000400			/* 1Mb transfer rate */
#define	MLMR_500KB	0001000			/* 500 Kb transfer rate */
#define	MLMR_250KB	0001400			/* 250 Kb transfer rate */

#define	MLDT_ML11	0000110			/* ML11 drive type */
