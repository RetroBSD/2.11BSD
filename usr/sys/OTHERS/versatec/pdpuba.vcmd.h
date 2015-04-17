/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vcmd.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

#define	VPRINT		0100
#define	VPLOT		0200
#define	VPRINTPLOT	0400

#define	GETSTATE	(('v'<<8)|0)
#define	SETSTATE	(('v'<<8)|1)
#define BUFWRITE	(('v'<<8)|2)	/* async write */
