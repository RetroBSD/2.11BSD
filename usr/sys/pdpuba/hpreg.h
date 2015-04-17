/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hpreg.h	2.0 (2.11BSD) 1995/07/20
 */

/*
 * Definitions for SMD-type disk drives and drivers.
 */

#define	XP_CC	0x1	/* drivedata[0] bit indicating presence of current
			 * cylinder 'hpcc' register.  Only set for RP04,5,6,7.
			*/
#define	XP_NOSEARCH 0x2	/* drivedata[0] bit indicating lack of search cmd */

/*
 *	Controller registers and bits
 */
struct hpdevice
{
	union	{
		int	w;
		char	c[2];
	}	hpcs1;		/* control and status 1 register */
	short	hpwc;		/* word count register */
	caddr_t	hpba;		/* UNIBUS address register */
	short	hpda;		/* desired address register */
	union	{
		int	w;
		char	c[2];
	}	hpcs2;		/* control and status 2 register */
	short	hpds;		/* drive status register */
	short	hper1;		/* error register 1 */
	short	hpas;		/* attention summary register */
	short	hpla;		/* look ahead register */
	short	hpdb;		/* data buffer register */
	short	hpmr;		/* maintenance register (1) */ 
	short	hpdt;		/* drive type register */
	short	hpsn;		/* serial number register */
	short	hpof;		/* offset register */
	short	hpdc;		/* desired cylinder address register */
	short	hpcc;		/* HP: current cylinder register */
#define	rmhr	hpcc		/* RM: holding register */
	short	hper2;		/* HP: error register 2 */
#define	rmmr2	hper2		/* RM: maintenance register 2 */
	short	hper3;		/* HP: error register 3 */
#define	rmer2	hper3		/* RM: error register 2 */
	short	hpec1;		/* burst error bit position */
	short	hpec2;		/* burst error bit pattern */
	short	hpbae;		/* bus address extension register (RH70 only) */
	short	hpcs3;		/* control and status 3 register (RH70 only) */
};

/* Other bits of hpcs1 */
#define	HP_SC		0100000		/* special condition */
#define	HP_TRE		0040000		/* transfer error */
#define	HP_MCPE		0020000		/* MASSBUS control bus read parity error */
/* bit 12 is unused */
#define	HP_DVA		0004000		/* drive available */
/* bits 9 and 8 are the extended address bits */
#define	HP_RDY		0000200		/* controller ready */
#define	HP_IE		0000100		/* interrupt enable */
/* bits 5-1 are the command */
#define	HP_GO		0000001
#define	HP_BITS		\
"\10\20SC\17TRE\16MCPE\14DVA\10RDY\7IE\1GO"

/* commands */
#define	HP_NOP		000
#define	HP_UNLOAD	002		/* offline drive */
#define	HP_SEEK		004		/* seek */
#define	HP_RECAL	006		/* recalibrate */
#define	HP_DCLR		010		/* drive clear */
#define	HP_RELEASE	012		/* release */
#define	HP_OFFSET	014		/* offset */
#define	HP_RTC		016		/* return to center-line */
#define	HP_PRESET	020		/* read-in preset */
#define	HP_PACK		022		/* pack acknowledge */
#define	HP_SEARCH	030		/* search */
#define	HP_WCDATA	050		/* write check data */
#define	HP_WCHDR	052		/* write check header and data */
#define	HP_WCOM		060		/* write */
#define	HP_WHDR		062		/* write header and data */
#define	HP_RCOM		070		/* read data */
#define	HP_RHDR		072		/* read header and data */
/* The following two are optionally enabled on some non-DEC controllers */
#define	HP_BOOT		074		/* boot */
#define	HP_FORMAT	076		/* format */

/* hpcs2 */
#define	HPCS2_DLT	0100000		/* data late */
#define	HPCS2_WCE	0040000		/* write check error */
#define	HPCS2_UPE	0020000		/* UNIBUS parity error */
#define	HPCS2_NED	0010000		/* nonexistent drive */
#define	HPCS2_NEM	0004000		/* nonexistent memory */
#define	HPCS2_PGE	0002000		/* programming error */
#define	HPCS2_MXF	0001000		/* missed transfer */
#define	HPCS2_MDPE	0000400		/* MASSBUS data read parity error */
#define	HPCS2_OR	0000200		/* output ready */
#define	HPCS2_IR	0000100		/* input ready */
#define	HPCS2_CLR	0000040		/* controller clear */
#define	HPCS2_PAT	0000020		/* parity test */
#define	HPCS2_BAI	0000010		/* address increment inhibit */
/* bits 2-0 are drive select */

#define	HPCS2_BITS \
"\10\20DLT\17WCE\16UPE\15NED\14NEM\13PGE\12MXF\11MDPE\
\10OR\7IR\6CLR\5PAT\4BAI"

/* hpds */
#define	HPDS_ATA	0100000		/* attention active */
#define	HPDS_ERR	0040000		/* composite drive error */
#define	HPDS_PIP	0020000		/* positioning in progress */
#define	HPDS_MOL	0010000		/* medium on line */
#define	HPDS_WRL	0004000		/* write locked */
#define	HPDS_LST	0002000		/* last sector transferred */
#define	HPDS_DAE	0001000		/* dual access enabled (programmable) */
#define	HPDS_DPR	0000400		/* drive present */
#define	HPDS_DRY	0000200		/* drive ready */
#define	HPDS_VV		0000100		/* volume valid */
/* bits 5-3, 1 are spare */
#define HPDS_ILV	0000004		/* interleaved */
#define	HPDS_OM		0000001		/* offset mode */

#define	HPDS_DREADY	(HPDS_DPR|HPDS_DRY|HPDS_MOL|HPDS_VV)

#define	HPDS_BITS \
"\10\20ATA\17ERR\16PIP\15MOL\14WRL\13LST\12DAE\11DPR\10DRY\7VV\3ILV\1OM"

/* hper1 */
#define	HPER1_DCK	0100000		/* data check */
#define	HPER1_UNS	0040000		/* drive unsafe */
#define	HPER1_OPI	0020000		/* operation incomplete */
#define	HPER1_DTE	0010000		/* drive timing error */
#define	HPER1_WLE	0004000		/* write lock error */
#define	HPER1_IAE	0002000		/* invalid address error */
#define	HPER1_AOE	0001000		/* address overflow error */
#define	HPER1_HCRC	0000400		/* header crc error */
#define	HPER1_HCE	0000200		/* header compare error */
#define	HPER1_ECH	0000100		/* ecc hard error */
#define	HPER1_WCF	0000040		/* write clock fail (0) */
#define	HPER1_FER	0000020		/* format error */
#define	HPER1_PAR	0000010		/* parity error */
#define	HPER1_RMR	0000004		/* register modification refused */
#define	HPER1_ILR	0000002		/* illegal register */
#define	HPER1_ILF	0000001		/* illegal function */

#define	HPER1_BITS \
"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IAE\12AOE\11HCRC\10HCE\
\7ECH\6WCF\5FER\4PAR\3RMR\2ILR\1ILF"

/* hpdt */
#define	HPDT_NBA	0100000		/* not block addressed; always 0 */
#define	HPDT_TAPE	0040000		/* tape drive; always 0 */
#define	HPDT_MH		0020000		/* moving head; always 1 */
/* bit 12 is unused */
#define	HPDT_DRR	0004000		/* drive request required  */
/* bits 10-9 are unused */
/* bits 8-0 are drive type; the correct values are hard to determine */
#define	HPDT_RM02	0000025		/* rm02, possibly rm03? */
#define	HPDT_RM80	0000026		/* rm80 */
#define	HPDT_RM05	0000027		/* rm05 */
#define	HPDT_RM03	0000024		/* rm03 */
#define	HPDT_RP04	0000020		/* rp04 */
#define	HPDT_RP05	0000021		/* rp05 */
#define	HPDT_RP06	0000022		/* rp06 */
#define HPDT_RP07	0000042		/* rp07 */

/* hpof */
#define	HPOF_FMT22	0010000		/* 16 bit format */
#define	HPOF_ECI	0004000		/* ecc inhibit */
#define	HPOF_HCI	0002000		/* header compare inhibit */

/* THE SC21 ACTUALLY JUST IMPLEMENTS ADVANCE/RETARD... */
#define	HPOF_P400	0020		/*  +400 uinches */
#define	HPOF_M400	0220		/*  -400 uinches */
#define	HPOF_P800	0040		/*  +800 uinches */
#define	HPOF_M800	0240		/*  -800 uinches */
#define	HPOF_P1200	0060		/* +1200 uinches */
#define	HPOF_M1200	0260		/* -1200 uinches */

#define	HPOF_BITS	\
"\10\15FMT22\14ECI\13HCI\10OD"

/*
 * rmhr
 *
 * Emulex (i.e non DEC) controllers implement the ability to query for
 * the drive geometry by placing these codes into the 'holding register'.
*/
#define	HPHR_MAXCYL	0x8017		/* max cylinder */
#define	HPHR_MAXTRAK	0x8018		/* max track/cylinder */
#define	HPHR_MAXSECT	0x8019		/* max sector/track */

/* rmer2: These are the bits for an RM error register 2 */
#define	RMER2_BSE	0100000		/* bad sector error */
#define	RMER2_SKI	0040000		/* seek incomplete */
#define	RMER2_DPE	0020000		/* drive plug error */
#define	RMER2_IVC	0010000		/* invalid command */
#define	RMER2_LSC	0004000		/* loss of system clock */
#define	RMER2_LBC	0002000		/* loss of bit clock */
/* bits 9-8 are unused */
#define	RMER2_DVC	0000200		/* device check */
/* bits 6-4 are unused */
#define	RMER2_MDPE	0000010		/* MASSBUS data read parity error */
/* bits 2-0 are unused */

#define	RMER2_BITS \
"\10\20BSE\17SKI\16DPE\15IVC\14LSC\13LBC\10DVC\4MDPE"

/* hpcs3 */
#define	HPCS3_APE	0100000		/* address parity error */
#define	HPCS3_DPE	0060000		/* data parity error */
#define	HPCS3_WCE	0017000		/* write check error */
#define	HPCS3_DW	0002000		/* double word */
/* bits 9-8 are unused */
#define	HPCS3_IE	0000100		/* interrupt enable */
/* bits 5-4 are unused */
/* bits 3-0 are inverted parity check */

#define	HPCS3_BITS	\
"\10\20APE\17DPE\15WCE\13DW\7IE"

/*
 * Systems Industries kludge: use value in
 * the serial # register to figure out real drive type.
 */
#define	SIMB_MB	0xff00		/* model byte value */
#define	SIMB_S6	0x2000		/* switch s6 */
#define	SIMB_LU	0x0007		/* logical unit (should = drive #) */

#define	SI9775D	0x0700		/* 9775 direct */
#define	SI9775M	0x0e00		/* 9775 mapped */
#define	SI9730D	0x0b00		/* 9730 or 9784 direct */
#define	SI9730M	0x0d00		/* 9730 or 9784 mapped */
#define	SI9766	0x0300		/* 9766 */
#define	SI9762	0x0100		/* 9762 */
#define	SICAPD	0x0500		/* Capricorn direct */
#define	SICAPN	0x0400		/* Capricorn mapped */
#define	SI9751D	0x0f00		/* Eagle direct */

#define	SIRM03	0x8000		/* RM03 indication */
#define	SIRM05	0x0000		/* RM05 pseudo-indication */
