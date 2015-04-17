/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_acct.c	3.1 (2.11BSD) 1999/4/29
 *
 * This module is a shadow of its former self.  This comment:
 *
 * 	SHOULD REPLACE THIS WITH A DRIVER THAT CAN BE READ TO SIMPLIFY.
 *
 * is all that is left of the original kern_acct.c module.
 */

#include "param.h"
#include "systm.h"
#include "user.h"
#include "msgbuf.h"
#include "kernel.h"
#include "acct.h"

	comp_t	compress();
	int	Acctthresh = 10;

/*
 * On exit, write a record on the accounting file.
 */
acct()
	{
	struct	acct acctbuf;
	register struct acct *ap = &acctbuf;
	static	short acctcnt = 0;

	bcopy(u.u_comm, ap->ac_comm, sizeof(acctbuf.ac_comm));
/*
 * The 'user' and 'system' times need to be converted from 'hz' (linefrequency)
 * clockticks to the AHZ pseudo-tick unit of measure.  The elapsed time is
 * converted from seconds to AHZ ticks.
*/
	ap->ac_utime = compress(((u_long)u.u_ru.ru_utime * AHZ) / hz);
	ap->ac_stime = compress(((u_long)u.u_ru.ru_stime * AHZ) / hz);
	ap->ac_etime = compress((u_long)(time.tv_sec - u.u_start) * AHZ);
	ap->ac_btime = u.u_start;
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	ap->ac_mem = (u.u_dsize+u.u_ssize) / 16; /* fast ctok() */
/*
 * Section 3.9 of the 4.3BSD book says that I/O is measured in 1/AHZ units too.
*/
	ap->ac_io = compress((u_long)(u.u_ru.ru_inblock+u.u_ru.ru_oublock)*AHZ);
	if	(u.u_ttyp)
		ap->ac_tty = u.u_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_flag = u.u_acflag;
/*
 * Not a lot that can be done if logwrt fails so ignore any errors.  Every
 * few (10 by default) commands call the wakeup routine.  This isn't perfect 
 * but does cut down the overhead of issuing a wakeup to the accounting daemon 
 * every single accounting record.  The threshold is settable via sysctl(8)
*/
	logwrt(ap, sizeof (*ap), logACCT);
	if	(++acctcnt >= Acctthresh)
		{
		logwakeup(logACCT);
		acctcnt = 0;
		}
	}

/*
 * Raise exponent and drop bits off the right of the mantissa until the
 * mantissa fits.  If we run out of exponent space, return max value (all
 * one bits).  With AHZ set to 64 this is good for close to 8.5 years:
 * (8191 * (1 << (3*7)) / 64 / 60 / 60 / 24 / 365 ~= 8.5)
 */

#define	MANTSIZE	13			/* 13 bit mantissa. */
#define	EXPSIZE		3			/* Base 8 (3 bit) exponent. */

comp_t
compress(mant)
	u_long mant;
	{
	register int exp;

	for	(exp = 0; exp < (1 << EXPSIZE); exp++, mant >>= EXPSIZE)
		if	(mant < (1L << MANTSIZE))
			return(mant | (exp << MANTSIZE));
	return(~0);
	}
