/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hkreg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

struct hkdevice
{
	short	hkcs1;		/* control status reg 1 */
	short	hkwc;		/* word count */
	caddr_t	hkba;		/* bus address */
	short	hkda;		/* disk address */
	short	hkcs2;		/* control status reg 2 */
	short	hkds;		/* drive status */
	short	hker;		/* driver error register */
	short	hkatt;		/* attention status/offset register */
	short	hkcyl;		/* current cylinder register */
	short	hkxmem;	        /* extended memory address register */
	short	hkdb;		/* data buffer register */
	short	hkmr1;		/* maint reg 1 */
	short	hkecps;		/* burst error bit position */
	short	hkecpt;		/* burst error bit pattern */
	short	hkmr2;		/* maint reg 2 */
	short	hkmr3;		/* maint reg 3 */
};

/* hkcs1 */
#define	HK_CCLR		0100000		/* controller clear (also error) */
#define	HK_CERR		HK_CCLR
#define	HK_DI		0040000		/* drive interrupt */
#define	HK_DTCPAR	0020000		/* drive to controller parity */
#define	HK_CFMT		0010000		/* 18 bit word format */
#define	HK_CTO		0004000		/* controller timeout */
#define	HK_CDT		0002000		/* drive type (rk07/rk06) */
/* bits 9 and 8 are the extended bus address */
#define	HK_CRDY		0000200		/* controller ready */
#define	HK_IE		0000100		/* interrupt enable */
/* bit 5 is unused */
/* bits 4-1 are the function code */
#define	HK_GO		0000001
#define	HK_BITS		\
"\10\20CCLR\17DI\16DTCPAR\15CFMT\14CTO\13CDT\10CRDY\7IE\1GO"

/* commands */
#define	HK_SELECT	000		/* select drive */
#define	HK_PACK		002		/* pack acknowledge */
#define	HK_DCLR		004		/* drive clear */
#define	HK_UNLOAD	006		/* unload */
#define	HK_START	010		/* start spindle */
#define	HK_RECAL	012		/* recalibrate */
#define	HK_OFFSET	014		/* offset */
#define	HK_SEEK		016		/* seek */
#define	HK_READ		020		/* read data */
#define	HK_WRITE	022		/* write data */
#define	HK_RHDR		026		/* read header */
#define	HK_WHDR		030		/* write header */

/* hkcs2 */
#define	HKCS2_DLT	0100000		/* data late */
#define	HKCS2_WCE	0040000		/* write check */
#define	HKCS2_UPE	0020000		/* UNIBUS parity */
#define	HKCS2_NED	0010000		/* non-existent drive */
#define	HKCS2_NEM	0004000		/* non-existent memory */
#define	HKCS2_PGE	0002000		/* programming error */
#define	HKCS2_MDS	0001000		/* multiple drive select */
#define	HKCS2_UFE	0000400		/* unit field error */
#define	HKCS2_OR	0000200		/* output ready */
#define	HKCS2_IR	0000100		/* input ready */
#define	HKCS2_SCLR	0000040		/* subsystem clear */
#define	HKCS2_BAI	0000020		/* bus address increment inhibit */
#define	HKCS2_RLS	0000010		/* release */
/* bits 2-0 are drive select */

#define	HKCS2_BITS \
"\10\20DLT\17WCE\16UPE\15NED\14NEM\13PGE\12MDS\11UFE\
\10OR\7IR\6SCLR\5BAI\4RLS"

#define	HKCS2_HARD		(HKCS2_NED|HKCS2_PGE)

/* hkds */
#define	HKDS_SVAL	0100000		/* status valid */
#define	HKDS_CDA	0040000		/* current drive attention */
#define	HKDS_PIP	0020000		/* positioning in progress */
/* bit 12 is unused */
#define	HKDS_WRL	0004000		/* write lock */
/* bits 10-9 are unused */
#define	HKDS_DDT	0000400		/* disk drive type */
#define	HKDS_DRDY	0000200		/* drive ready */
#define	HKDS_VV		0000100		/* volume valid */
#define	HKDS_DROT	0000040		/* drive off track */
#define	HKDS_SPLS	0000020		/* speed loss */
#define	HKDS_ACLO	0000010		/* ac low */
#define	HKDS_OFF	0000004		/* offset mode */
/* bit 1 is unused */
#define	HKDS_DRA	0000001		/* drive available */

#define	HKDS_DREADY	(HKDS_DRA|HKDS_VV|HKDS_DRDY)
#define	HKDS_BITS \
"\10\20SVAL\17CDA\16PIP\14WRL\11DDT\10DRDY\7VV\6DROT\5SPLS\4ACLO\3OFF\1DRA"
#define	HKDS_HARD	(HKDS_ACLO|HKDS_SPLS)

/* hker */
#define	HKER_DCK	0100000		/* data check */
#define	HKER_UNS	0040000		/* drive unsafe */
#define	HKER_OPI	0020000		/* operation incomplete */
#define	HKER_DTE	0010000		/* drive timing error */
#define	HKER_WLE	0004000		/* write lock error */
#define	HKER_IDAE	0002000		/* invalid disk address error */
#define	HKER_COE	0001000		/* cylinder overflow error */
#define	HKER_HRVC	0000400		/* header vertical redundancy check */
#define	HKER_BSE	0000200		/* bad sector error */
#define	HKER_ECH	0000100		/* hard ecc error */
#define	HKER_DTYE	0000040		/* drive type error */
#define	HKER_FMTE	0000020		/* format error */
#define	HKER_DRPAR	0000010		/* control-to-drive parity error */
#define	HKER_NXF	0000004		/* non-executable function */
#define	HKER_SKI	0000002		/* seek incomplete */
#define	HKER_ILF	0000001		/* illegal function */

#define	HKER_BITS \
"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IDAE\12COE\11HRVC\
\10BSE\7ECH\6DTYE\5FMTE\4DRPAR\3NXF\2SKI\1ILF"
#define	HKER_HARD	\
	(HKER_WLE|HKER_IDAE|HKER_COE|HKER_DTYE|HKER_FMTE|HKER_ILF)

/* offset bits in hkas */
#define	HKAS_P400	0020		/*  +400 RK06,  +200 RK07 */
#define	HKAS_M400	0220		/*  -400 RK06,  -200 RK07 */
#define	HKAS_P800	0040		/*  +800 RK06,  +400 RK07 */
#define	HKAS_M800	0240		/*  -800 RK06,  -400 RK07 */
#define	HKAS_P1200	0060		/*  +800 RK06,  +400 RK07 */
#define	HKAS_M1200	0260		/* -1200 RK06, -1200 RK07 */
