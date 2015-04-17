/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)htreg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

struct	htdevice
{
	short	htcs1;		/* control and status 1 register */
	short	htwc;		/* word count register */
	caddr_t	htba;		/* bus address register */
	short	htfc;		/* frame counter */
	short	htcs2;		/* control and status 2 register */
	short	htfs;		/* formatter status register */
	short	hter;		/* error register */
	short	htas;		/* attention summary register */
	short	htcc;		/* check character register */
	short	htdb;		/* data buffer register */
	short	htmr;		/* maintenance register */
	short	htdt;		/* drive type register */
	short	htsn;		/* serial number register */
	short	httc;		/* tape control register */
	short	htbae;		/* bus address extension register (RH70 only) */
	short	htcs3;		/* control and status 3 register (RH70 only) */
};

/* htcs1 */
#define	HT_SC		0100000		/* special condition */
#define	HT_TRE		0040000		/* transfer error */
#define	HT_MCPE		0020000		/* MASSBUS control bus read parity error */
/* bit 12 is unused */
#define	HT_DVA		0004000		/* drive available */
/* bit 10 is unused */
/* bits 9-8 are UNIBUS extension bits */
#define	HT_RDY		0000200		/* ready */
#define	HT_IE		0000100		/* interrupt enable */
#define	HT_GO		0000001		/* go bit */
/* bits 5-1 are the command */
#define	HTCS1_BITS	\
"\10\20SC\17TRE\16MCPE\14DVA\12A17\11A16\10RDY\7IE\1GO"

/* commands */
#define	HT_SENSE	0000000		/* no operations (sense) */
#define	HT_REWOFFL	0000002		/* rewind offline */
#define	HT_REW		0000006		/* rewind */
#define	HT_DCLR		0000010		/* drive clear */
#define	HT_RIP		0000020		/* read in preset */
#define	HT_ERASE	0000024		/* erase */
#define	HT_WEOF		0000026		/* write tape mark */
#define	HT_SFORW	0000030		/* space forward */
#define	HT_SREV		0000032		/* space reverse */
#define	HT_WCHFWD	0000050		/* write check forward */
#define	HT_WCHREV	0000056		/* write check reverse */
#define	HT_WCOM		0000060		/* write forward */
#define	HT_RCOM		0000070		/* read forward */
#define	HT_RREV		0000076		/* read reverse */

/* htcs2 */
#define	HTCS2_DLT	0100000		/* data late */
/*
 * The 1981-1982 DEC peripherals handbook says bit 14 is unused.
 * The 1980 one says it's WCE.  Take your choice.
 */
#define	HTCS2_WCE	0040000		/* write check error */
#define	HTCS2_UPE	0020000		/* UNIBUS parity error */
#define	HTCS2_NEF	0010000		/* nonexistent formatter */
#define	HTCS2_NEM	0004000		/* nonexistent memory */
#define	HTCS2_PRE	0002000		/* program error */
#define	HTCS2_MXF	0001000		/* missed transfer */
#define	HTCS2_MDPE	0000400		/* MASSBUS data read parity error */
#define	HTCS2_OR	0000200		/* output ready */
#define	HTCS2_IR	0000100		/* input ready */
#define	HTCS2_CLR	0000040		/* controller clear */
#define	HTCS2_PAT	0000020		/* parity test */
#define	HTCS2_MAII	0000010		/* memory address increment inhibit */
/* bits 2-0 select the formatter */
#define	HTCS2_BITS	\
"\10\20DLT\16UPE\16NEF\14NEM\13PRE\12MXF\11MDPE\10OR\7IR\6CLR\5PAT\4MAII"
#define	HTCS2_ERR	\
	(HTCS2_MDPE|HTCS2_MXF|HTCS2_PRE|HTCS2_NEM|HTCS2_NEF|HTCS2_UPE|HTCS2_WCE)

/* htfs */
#define	HTFS_ATA	0100000		/* attention active */
#define	HTFS_ERR	0040000		/* composite error */
#define	HTFS_PIP	0020000		/* positioning in progress */
#define	HTFS_MOL	0010000		/* medium on line */
#define	HTFS_WRL	0004000		/* write lock */
#define	HTFS_EOT	0002000		/* end of tape */
/* bit 9 is unused */
#define	HTFS_DPR	0000400		/* drive present (always 1) */
#define	HTFS_DRY	0000200		/* drive ready */
#define	HTFS_SSC	0000100		/* slave status change */
#define	HTFS_PES	0000040		/* phase-encoded status */
#define	HTFS_SDWN	0000020		/* slowing down */
#define	HTFS_IDB	0000010		/* identification burst */
#define	HTFS_TM		0000004		/* tape mark */
#define	HTFS_BOT	0000002		/* beginning of tape */
#define	HTFS_SLA	0000001		/* slave attention */

#define	HTFS_BITS \
"\10\20ATA\17ERR\16PIP\15MOL\14WRL\13EOT\11DPR\10DRY\
\7SSC\6PES\5SDWN\4IDB\3TM\2BOT\1SLA"

/* hter */
#define	HTER_CORCRC	0100000		/* correctible data/crc error */
#define	HTER_UNS	0040000		/* unsafe */
#define	HTER_OPI	0020000		/* operation incomplete */
#define	HTER_CTE	0010000		/* controller timing error */
#define	HTER_NEF	0004000		/* non-executable function */
#define	HTER_CSITM	0002000		/* correctable skew/illegal tape mark */
#define	HTER_FCE	0001000		/* frame count error */
#define	HTER_NSG	0000400		/* non-standard gap */
#define	HTER_PEFLRC	0000200		/* pe format error/lrc error */
#define	HTER_INCVPE	0000100		/* incorrectable data/vertical parity error */
#define	HTER_DPAR	0000040		/* MASSBUS data write parity error */
#define	HTER_FMT	0000020		/* format select error */
#define	HTER_CPAR	0000010		/* MASSBUS control bus write parity error */
#define	HTER_RMR	0000004		/* register modification refused */
#define	HTER_ILR	0000002		/* illegal register */
#define	HTER_ILF	0000001		/* illegal function */

#define	HTER_BITS \
"\10\20CORCRC\17UNS\16OPI\15CTE\14NEF\13CSITM\12FCE\11NSG\10PEFLRC\
\7INCVPE\6DPAR\5FMT\4CPAR\3RMR\2ILR\1ILF"
#define	HTER_HARD \
	(HTER_UNS|HTER_OPI|HTER_NEF|HTER_DPAR|HTER_FMT|HTER_CPAR| \
	HTER_RMR|HTER_ILR|HTER_ILF)

/* htcc */
/* bits 15-9 are unused */
#define	HTCC_DTPAR	0000400		/* dead track parity/crc parity */
/* bits 7-0 contain dead track/crc information */
#define	HTCC_BITS	\
"\10\11DTPAR"

/* htmr */
/* bits 15-8 contain lrc/maintenance data */
#define	HTMR_MDPB	0000400		/* maintenance data parity bit */
#define	HTMR_TSC	0000200		/* tape speed clock */
#define	HTMR_MC		0000100		/* maintenance clock */
/* bits 4-1 contain the maintenance operation code */
#define	HTMR_MM		0000001		/* maintenance mode */
#define	HTMR_BITS	\
"\10\10MDPB\7TSC\6MC\1MM"

/* htdt */
#define	HTDT_NSA	0100000		/* not sector addressed; always 1 */
#define	HTDT_TAP	0040000		/* tape; always 1 */
#define	HTDT_MOH	0020000		/* moving head; always 0 */
#define	HTDT_7CH	0010000		/* 7 channel; always 0 */
#define	HTDT_DRQ	0004000		/* drive request required; always 0 */
#define	HTDT_SPR	0002000		/* slave present */
/* bit 9 is unused */
/* bits 8-0 are formatter/transport type */
#define	HTDT_BITS	\
"\10\20NSA\17TAP\16MOH\15SCH\14DRQ\13SPR"

/* httc */
#define	HTTC_ACCL	0100000		/* transport is not up to speed */
#define	HTTC_FCS	0040000		/* frame count status */
#define	HTTC_SAC	0020000		/* slave address change */
#define	HTTC_EAODTE	0010000		/* enable abort on data xfer errors */
/* bit 11 is unused */
/* bits 10-8 are density select */
#define	HTTC_800BPI	0001400		/* in bits 10-8, dens=800 */
#define	HTTC_1600BPI	0002000		/* in bits 10-8, dens=1600 */
/* bits 7-4 are format select */
#define	HTTC_PDP11	0000300		/* in bits 7-4, pdp11 normal format */
#define	HTTC_EVEN	0000010		/* select even parity */
/* bits 2-0 are slave select */

#define	HTTC_BITS	\
"\10\20ACCL\17FCS\16SAC\15EAODTE\4EVEN"

/* htcs3 */
#define	HTCS3_APE	0100000		/* address parity error */
/* bits 14-11 are unused */
#define	HTCS3_DW	0002000		/* double word */
/* bits 9-7 are unused */
#define	HTCS3_IE	0000100		/* interrupt enable */
/* bits 5-4 are unused */
/* bits 3-0 are inverted parity check */
#define	HTCS3_BITS	\
"\10\20APE\13DW\7IE"

#define	b_repcnt  b_bcount
#define	b_command b_resid
