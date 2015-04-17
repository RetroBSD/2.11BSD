/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)enable34.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *	All information relevant to the ENABLE/34 is supplied with
 *	the permission of ABLE Computer and may not be disclosed in
 *	any manner to sites not licensed by the University of California
 *	for the Second Berkeley Software Distribution.
 *
 *	ENABLE/34 support routines.
 *	Warning:  if part of an overlaid kernel, this module must be
 *	loaded in the base segment.
 */

#include "param.h"
#ifdef ENABLE34
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "inode.h"
#include "proc.h"
#include "seg.h"
#include "iopage.h"
#include "enable34.h"

/*
 *	Turn on an ENABLE/34 and fudge the PARs to achieve the ``nominal map,''
 *	i.e.		0163700 - 0163716:	KISA0 - KISA7
 * 			0163720 - 0163736:	UISA0 - UISA7
 * 			0163740 - 0163756:	UDSA0 - UDSA7
 * 			0163760 - 0163776:	KDSA0 - KDSA7
 *
 *	Assumes that DEC memory management is already on.  This routine
 *	cannot be interruptible since we might be changing PARs which
 *	point to overlay pages.
 */

/*
 * Shorthand:
 */
#define	A	((u_short *) 0163700)		/* ENABLE/34 PARs */
#define	KI	((u_short *) 0172340)		/* DEC Kernel I PARs */
#ifndef	KERN_NONSEP
#define	KD	((u_short *) 0172360)		/* DEC Kernel D PARs */
#else
#define	KD	KI				/* DEC Kernel D PARs */
#endif
#define	UI	((u_short *) 0177640)		/* DEC User I PARs */
#ifdef	NONSEPARATE
#define	UD	((u_short *) 0177660)		/* DEC User D PARs */
#else
#define	UD	UI				/* DEC User D PARs */
#endif

void
enableon()
{
	register i, s;
	extern	bool_t	enable34;

	if (!enable34)
		panic ("enableon");
	else
		if (*ENABLE_SSR4)
			return;
		else
			s	= splhigh();

	for (i = 0; i < 31; i++)
		A[i]	= (u_short) 0200 * (u_short) i;
	A[31]	= (u_short) 0200 * (u_short) 0511;

	/*
	 *	Ok so far because the ENABLE mapping is not turned on.
	 *	Since the above map is the identity map, turning on
	 *	ENABLE mapping will cause no harm.
	 */
	*ENABLE_SSR3	= ENABLE_SSR3_PARS_ON | ENABLE_SSR3_UBMAP_ON;
	*ENABLE_SSR4	|= ENABLE_SSR4_MAP_ON;

	for (i = 0; i < 7; i++)
		A[i + 24]	= sep_id ?  KD[i] : KI[i];
	for (i = 0; i < 8; i++)
		A[i + 16]	= KI[i];

	/*
	 *	Wiggle around changing DEC PARs to the nominal map.  This is
	 *	possible because we have extra A[] entries to use.  Even though
	 *	the values of A[] and K?[] change, A[K?[]] is unchanged.
	 */
	for (i = 0; i < 8; i++)	{
#ifndef	KERN_NONSEP
		KD[i]	= (u_short) 0200 * (u_short) (i + 24);
#endif
		KI[i]	= (u_short) 0200 * (u_short) (i + 16);
		A[i]	= A[i + 16];
		KI[i]	= (u_short) 0200 * (u_short) i;
	}

	/*
	 *	The kernel mode part of the nominal map has been established.
	 *	Now do the user mode part.  This is much easier.
	 */
	for (i = 0; i < 8; i++)	{
		A[i + 8]	= UI[i];
		A[i + 16]	= sep_id ?  UD[i] : UI[i];
#ifndef	NONSEPARATE
		UI[i]	= (u_short) 0200 * (u_short) (i + 8);
		if (sep_id)
#endif
			UD[i]	= (u_short) 0200 * (u_short) (i + 16);
	}

	for (i = 0; i < 32; i++)
		if (A[i] == 07600)
			A[i]	= (u_short) 0200 * (u_short) 511;

	splx(s);
}

/*
 *	Routines for probing the I/O page.  We must repoint the DEC
 *	segmentation registers because of the second level of indirection
 *	introduced by the ENABLE/34 segmentation registers.  These routines
 *	cannot be interruptible.
 */

#define	MapUI7								\
			register saveuisa7	= *DEC_UISA7;		\
			register s	= splhigh();			\
			*DEC_UISA7	= 0177600;
#define	UnmapUI7							\
			*DEC_UISA7	= saveuisa7;			\
			splx (s);

fiobyte (addr)
{
	register val;

	MapUI7;
	val	= fuibyte (addr);
	UnmapUI7;
	return (val);
}

fioword (addr)
{
	register val;

	MapUI7;
	val	= fuiword (addr);
	UnmapUI7;
	return (val);
}
#endif ENABLE34
