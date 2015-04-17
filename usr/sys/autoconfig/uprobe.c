/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uprobe.c	2.1 (2.11BSD GTE) 6/16/93
 */

/*
 * The uprobe table contains the pointers to the user-level probe routines
 * that may attempt to make various devices interrupt.
 *
 * NOTES:
 *	Reads and writes to kmem (done by grab, stuff) are currently done a
 *	byte at a time in the kernel.  Also, many of these are untested and
 *	some of them assume that if something is at the address, it is the
 *	correct device.  Others assume that if something *isn't* at the
 *	address, the correct device *is*.  Why are you looking at me like
 *	that?
 */

#include "uprobe.h"

int	xpprobe(), hkprobe(), rlprobe(), rkprobe(), htprobe(), siprobe(),
	tmprobe(), tsprobe(), cnprobe(), dzprobe(), dhprobe(), dmprobe(),
	drprobe(), lpprobe(), dhuprobe(), raprobe(), rxprobe(), brprobe(),
	dnprobe(), tmsprobe(), dhvprobe();

UPROBE uprobe[] = {
	"hk",	hkprobe,	/* hk -- rk611, rk06/07 */
	"hp",	xpprobe,	/* hp -- rjp04/06, rwp04/06 */
	"ra",	raprobe,	/* ra -- MSCP */
	"rk",	rkprobe,	/* rk -- rk05 */
	"rl",	rlprobe,	/* rl -- rl01/02 */
	"si",	siprobe,	/* si -- SI 9500 for CDC 9766 */
	"xp",	xpprobe,	/* xp -- rm02/03/05, rp04/05/06, Diva, Ampex, SI Eagle */
	"ht",	htprobe,	/* ht -- tju77, twu77, tje16, twe16 */
	"tm",	tmprobe,	/* tm -- tm11 */
	"ts",	tsprobe,	/* ts -- ts11 */
	"dh",	dhprobe,	/* dh -- DH11 */
	"dm",	dmprobe,	/* dm -- DM11 */
	"dr",	drprobe,	/* dr -- DR11W */
	"du",	dhuprobe,	/* du -- DHU11 */
	"dhv",	dhvprobe,	/* dhv -- DHV11 */
	"dz",	dzprobe,	/* dz -- dz11 */
	"cn",	cnprobe,	/* cn -- kl11, dl11 */
	"lp",	lpprobe,	/* lp -- line printer */
	"rx",	rxprobe,	/* rx -- RX01/02 */
	"br",	brprobe,	/* br -- EATON 1538 BR1537/BR1711 */
	"dn",	dnprobe,	/* dn -- dn11 autodialer */
	"tms",	tmsprobe,	/* tms -- TMSCP tape controller */
	0,	0,
};
