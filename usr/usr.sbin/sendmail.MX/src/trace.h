/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 */

#if !defined(lint) && !defined(NOSCCS)
static char sccsid[] = "@(#)trace.h	5.2 (Berkeley) 3/13/88";
#endif /* not lint */

/*
**  Trace Package.
*/

typedef u_char	*TRACEV;

extern TRACEV	tTvect;			/* current trace vector */

# ifndef tTVECT
# define tTVECT		tTvect
# endif tTVECT

# define tTf(flag, level)	(tTVECT[flag] >= level)
# define tTlevel(flag)		(tTVECT[flag])
