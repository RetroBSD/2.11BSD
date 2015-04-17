/*
 * RP04/RP06 disk driver
 */

#include <sys/param.h>
#include <sys/inode.h>
#include "../saio.h"

struct	device
{
	union {
		int	w;
		char	c[2];
	} xpcs1;		/* Control and Status register 1 */
	int	xpwc;		/* Word count register */
	caddr_t	xpba;		/* UNIBUS address register */
	int	xpda;		/* Desired address register */
	union {
		int	w;
		char	c[2];
	} xpcs2;		/* Control and Status register 2*/
	int	xpds;		/* Drive Status */
	int	xper1;		/* Error register 1 */
	int	xpas;		/* Attention Summary */
	int	xpla;		/* Look ahead */
	int	xpdb;		/* Data buffer */
	int	xpmr;		/* Maintenance register */
	int	xpdt;		/* Drive type */
	int	xpsn;		/* Serial number */
	int	xpof;		/* Offset register */
	int	xpdc;		/* Desired Cylinder address register*/
	int	xpcc;		/* Current Cylinder */
	int	xper2;		/* Error register 2 */
	int	xper3;		/* Error register 3 */
	int	xpec1;		/* Burst error bit position */
	int	xpec2;		/* Burst error bit pattern */
	int	xpbae;		/* 11/70 bus extension */
	int	xpcs3;
};

#define	XPADDR	((struct device *)0176700)

/*
 * Defines for Disk Type Independence
 */
#define	RP	022		/* RP04/5/6 */
#define	RM2	025		/* RM02/3 */
#define	RM3	024		/* RM02/3 */
#define	RM5	027		/* RM05 */
#define RM5X	076		/* Emulex + Ampex 9300CD */
#define	DVHP	077		/* Diva */
#ifdef MEGATEST
#define RM2X	075		/* Emulex SC01B + Fuji 160 */
#endif MEGATEST
#define	HP_SECT	22
#define	HP_TRAC	19
#define	RM_SECT	32
#define	RM_TRAC	5
#ifdef MEGATEST
#define	RM2X_SECT 32
#define	RM2X_TRAC 10
#endif MEGATEST
#define	DV_TRAC	19
#define	DV_SECT	33

#define	P400	020
#define	M400	0220
#define	P800	040
#define	M800	0240
#define	P1200	060
#define	M1200	0260

#define	GO	01
#define	PRESET	020
#define	RTC	016
#define	OFFSET	014
#define	SEARCH	030
#define	RECAL	06
#define DCLR	010
#define	WCOM	060
#define	RCOM	070

#define	IE	0100
#define	PIP	020000
#define	DRY	0200
#define	ERR	040000
#define	TRE	040000
#define	DCK	0100000
#define	WLE	04000
#define	ECH	0100
#define VV	0100
#define FMT22	010000

extern	char haveCSW;		/* bool, set if switch register exists */
#ifdef MEGATEST
int	xptype = RM2X;		/* drive type; declared so we can patch */
#else
int	xptype = RM2;		/* drive type; declared so we can patch */
#endif MEGATEST

xpstrategy(io, func)
register struct iob *io;
{
	register unit;
	register i;
	register nm_sect_per_cyl,nsect;
	daddr_t bn;
	int sn, cn, tn;

	if (((unit = io->i_unit) & 04) == 0)
		bn = io->i_bn;
	else {
		unit &= 03;
		bn = io->i_bn;
		bn -= io->i_boff;
		i = unit + 1;
		unit = bn%i;
		bn /= i;
		bn += io->i_boff;
	}

	XPADDR->xpcs2.w = unit;

	if((XPADDR->xpds & VV) == 0) {
		XPADDR->xpcs1.c[0] = PRESET|GO;
		XPADDR->xpof = FMT22;
	}
	/*
 	 *	This next weirdness handled the look up into the Drive Type
 	 *	register to tell what type of disk we have here.
	 *	Look in switch register first (if there is one).
 	 *
 	 *	Note: No need to look up after the first time.
 	 */
	
	if (xptype == 0) {
		if (haveCSW && (*CSW == RM2 || *CSW == RM3 || *CSW == RM5
		    || *CSW == RM5X || *CSW == RP || *CSW == DVHP 
#ifdef MEGATEST
		    || *CSW == RM2X
#endif MEGATEST
		    ))
			xptype = *CSW;
		else
			xptype = (XPADDR->xpdt & 077);
	}

	switch(xptype)
	{

	case RM2:
	case RM3:
		nm_sect_per_cyl = RM_SECT * RM_TRAC;
		nsect = RM_SECT;
		break;

#ifdef MEGATEST
	case RM2X:
		nm_sect_per_cyl = RM2X_SECT * RM2X_TRAC;
		nsect = RM2X_SECT;
		break;

#endif MEGATEST
	case RM5:
	case RM5X:
		nm_sect_per_cyl = RM_SECT * HP_TRAC;
		nsect = RM_SECT;
		break;

	case RP:
		nm_sect_per_cyl = HP_SECT * HP_TRAC;
		nsect = HP_SECT;
		break;

	case DVHP:
		nm_sect_per_cyl = DV_SECT * DV_TRAC;
		nsect = DV_SECT;
		break;

	default:
		printf("xp: unknown device type 0%o\n", xptype);
		return(-1);	
	}
	cn = bn/(nm_sect_per_cyl);
	sn = bn%(nm_sect_per_cyl);
	tn = sn/nsect;
	sn = sn%nsect;

	XPADDR->xpdc = cn;
	XPADDR->xpda = (tn << 8) + sn;
	XPADDR->xpba = io->i_ma;
	XPADDR->xpwc = -(io->i_cc>>1);
	unit = (segflag << 8) | GO;
	if (func == READ)
		unit |= RCOM;
	else if (func == WRITE)
		unit |= WCOM;
	XPADDR->xpcs1.w = unit;
	while ((XPADDR->xpcs1.w&DRY) == 0)
			;
	if (XPADDR->xpcs1.w & TRE) {
		printf("disk error: cyl=%d track=%d sect=%d cs2=%o, er1=%o\n",
		    cn, tn, sn, XPADDR->xpcs2, XPADDR->xper1);
		return(-1);
	}
	return(io->i_cc);
}
