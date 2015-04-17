struct	rx2device	{
	short	rx2cs;			/* command and status register */
	short	rx2db;			/* multipurpose register: */
#define	rx2ba	rx2db			/* 	bus address register */
#define	rx2ta	rx2db			/*	track address register */
#define	rx2sa	rx2db			/*	sector address register */
#define	rx2wc	rx2db			/*	word count register */
#define	rx2es	rx2db			/*	error and status register */
};

/* bits in rx2cs */
#define	RX2_ERR		0100000		/* error */
#define	RX2_INIT	0040000		/* initialize */
/* bits 13-12 are the extension bits */
#define	RX2_RX02	0004000		/* rx02 (read only) */
/* bit 10 is unused (bit 9 is also unused in the standard rx211) */
#define	RX2_HD		0001000		/* DSD 480 head select */
#define	RX2_DD		0000400		/* double density */
#define	RX2_XREQ	0000200		/* transfer request */
#define	RX2_IE		0000100		/* interrupt enable */
#define	RX2_DONE	0000040		/* done */
#define	RX2_UNIT	0000020		/* unit select */
/* bits 3-1 are the function code */
#define	RX2_GO		0000001		/* go */

/* function codes */
#define	RX2_FILL	0000000		/* fill buffer */
#define	RX2_EMPTY	0000002		/* empty */
#define	RX2_WSECT	0000004		/* write sector */
#define	RX2_RSECT	0000006		/* read sector */
#define	RX2_SMD		0000010		/* set media density */
#define	RX2_RDSTAT	0000012		/* read status */
#define	RX2_WDDSECT	0000014		/* write deleted data sector */
#define	RX2_RDEC	0000016		/* read error code */

#define	RX2_BITS	\
"\10\20ERR\17INIT\14RX02\12HD\11DD\10XREQ\7IE\6DONE\5UNIT1\1GO"

/* bits in rx2es */
/* bits 15-12 are unused in the standard rx211 */
#define	RX2ES_IBMDD	0010000		/* DSD 480 IBM double density select */
#define	RX2ES_NXM	0004000		/* nonexistent memory */
#define	RX2ES_WCOVFL	0002000		/* word count overflow */
/* bit 9 is unused in the standard rx211 */
#define	RX2ES_HD	0001000		/* DSD 480 head select */
#define	RX2ES_UNIT	0000400		/* unit select */
#define	RX2ES_RDY	0000200		/* ready */
#define	RX2ES_DDATA	0000100		/* deleted data */
#define	RX2ES_DD	0000040		/* double density */
#define	RX2ES_DENSERR	0000020		/* density error */
#define	RX2ES_ACLO	0000010		/* ac low */
#define	RX2ES_INITDONE	0000004		/* initialization done */
/* bit 1 is unused in the standard rx211 */
#define	RX2ES_S1RDY	0000002		/* DSD 480 side 1 ready */
#define	RX2ES_CRC	0000001		/* crc error */
#define	RX2ES_BITS	\
"\10\15IBMDD\14NXM\13WCOVFL\12HD\11UNIT1\10RDY\7DDATA\
\6DDENS\5DENSERR\4ACLO\3INIT\2S1RDY\1CRC"

/* bits in rx2es1 */
/* bits 15-8 contain the word count */
/* bits 7-0 contain the error code */
#define	RX2ES1_D0NOHOME	0000010		/* drive 0 failed to see home on init */
#define	RX2ES1_D1NOHOME	0000020		/* drive 1 failed to see home on init */
#define	RX2ES1_XTRK	0000040		/* track number > 076 */
#define	RX2ES1_WRAP	0000050		/* found home before desired track */
#define	RX2ES1_HNF	0000070		/* header not found after 2 revs */
#define	RX2ES1_NSEP	0000110		/* up controller found not SEP clock */
#define	RX2ES1_NOPREAMB	0000120		/* preamble not found */
#define	RX2ES1_NOID	0000130		/* preamble found;ID burst timeout */
#define	RX2ES1_HNEQTRK	0000150		/* track reached doesn't match header */
#define	RX2ES1_XIDAM	0000160		/* up made to many attempts for IDAM */
#define	RX2ES1_NOAM	0000170		/* data AM timeout */
#define	RX2ES1_CRC	0000200		/* crc error reading disk sector */
#define	RX2ES1_OOPS	0000220		/* read/write electronics failed test */
#define	RX2ES1_WCOVFL	0000230		/* word count overflow */
#define	RX2ES1_DENSERR	0000240		/* density error */
#define	RX2ES1_BADKEY	0000250		/* bad key word for Set Media Density */

/* bits in rx2es4 */
/* bits 15-8 contain the track address for header track address errors */
#define	RX2ES4_UNIT	0000200		/* unit select */
#define	RX2ES4_D1DENS	0000100		/* drive 1 density */
#define	RX2ES4_HEAD	0000040		/* head loaded */
#define	RX2ES4_D0DENS	0000020		/* drive 0 density */
/* bits 3-1 are unused */
#define	RX2ES4_DD	0000001		/* diskette is double density */
#define	RX2ES4_BITS	"\10\10DRIVE1\7D1HIDENS\6HEAD\5D0HIDENS\1DDENS"

#define	b_seccnt	av_back
#define	b_state		b_active
