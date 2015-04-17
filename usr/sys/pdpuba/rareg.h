/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rareg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * RA disk controller device registers
*/
typedef	struct	radevice	{
	short	raip;		/* init and polling */
	short	rasa;		/* status and addresses */
} radeviceT;

#define RA_ERR		0100000
#define RA_STEP4	0040000
#define RA_STEP3	0020000
#define RA_STEP2	0010000
#define RA_STEP1	0004000
#define RA_NV		0002000
#define RA_QB		0001000
#define RA_DI		0000400
#define RA_IE		0000200
#define RA_PI		0000001
#define RA_GO		0000001
