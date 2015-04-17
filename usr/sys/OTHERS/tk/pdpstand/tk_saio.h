/**********************************************************************
 *   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.    *
 *   All Rights Reserved. 					      *
 *   Reference "/usr/src/COPYRIGHT" for applicable restrictions.      *
 **********************************************************************/

/*
 * SCCSID: @(#)tk_saio.h	3.0	4/21/86
 *
 * Common definitions for all standalone programs
 * that must deal with TK type magtape.
 *
 *
 * Chung_wu Lee		2/8/85
 *
 */

#define	TK50	3
#define	TU81	5
#define NTK	1

struct tk_drv {			/* TK drive information */
	char	tk_dt;		/* TK drive type, 0 = NODRIVE */
	char	tk_online;	/* TK drive on-line flag */
	char	tk_openf;	/* TK drive in use flag */
	char	tk_flags;	/* TK drive misc. flag */
};
