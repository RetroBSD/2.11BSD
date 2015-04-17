/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)reg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 */
#define	R0	(0)
#define	R1	(-3)
#define	R2	(-11)
#define	R3	(-10)
#define	R4	(-9)
#define	R5	(-7)
#define	R6	(-4)
#define	R7	(1)
#define	PC	(1)
#define	RPS	(2)
