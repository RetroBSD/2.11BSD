/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)math.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

extern	double	fabs(), floor(), ceil(), fmod(), ldexp();
extern	double	sqrt(), hypot(), atof();
extern	double	sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern	double	exp(), log(), log10(), pow();
extern	double	sinh(), cosh(), tanh();
extern	double	gamma();
extern	double	j0(), j1(), jn(), y0(), y1(), yn();

#define	HUGE	1.701411733192644270e38
#define	LOGHUGE	39
