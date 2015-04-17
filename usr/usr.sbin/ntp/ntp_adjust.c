#if !defined(lint) && defined(DOSCCS)
static char *RCSid = "$Source: /usr/src/new/ntp/ntp_adjust.c,v $ $Revision: 3.4.1.5 $ $Date: 95/01/27 17:23:36 $";
#endif

/*
 * This module implemenets the logical Local Clock, as described in section
 * 5. of the NTP specification.
 *
 * $Log:	ntp_adjust.c,v $
 * Revision 3.4.1.5  95/01/27  17:23:36  sms
 * Remove SETTICKADJ - see rational in ntpd.c
 *
 * Revision 3.4.1.4  89/05/18  18:23:36  louie
 * A couple of changes to debug NeXT support in ntp_adjust.c
 * 
 * Revision 3.4.1.3  89/04/07  18:05:17  louie
 * Removed unused variable from ntp_adjust.c module.
 * 
 * Revision 3.4.1.2  89/03/22  18:30:52  louie
 * patch3: Use new RCS headers.
 * 
 * Revision 3.4.1.1  89/03/20  00:09:06  louie
 * patch1: Don't zero the drift compensation or compliance values when a step
 * patch1: adjustment of the clock occurs.  Use symbolic definition of
 * patch1: CLOCK_FACTOR rather than constant.
 * 
 * Revision 3.4  89/03/17  18:37:03  louie
 * Latest test release.
 * 
 * Revision 3.3.1.2  89/03/17  18:25:03  louie
 * Applied suggested code from Dennis Ferguson for logical clock model based on
 * the equations in section 5.  Many thanks.
 * 
 * Revision 3.3.1.1  89/03/16  19:19:29  louie
 * Attempt to implement using the equations in section 5 of the NTP spec, 
 * rather then modeling the Fuzzball implementation.
 * 
 * Revision 3.3  89/03/15  14:19:45  louie
 * New baseline for next release.
 * 
 * Revision 3.2.1.1  89/03/15  13:47:24  louie
 * Use "%f" in format strings rather than "%lf".
 * 
 * Revision 3.2  89/03/07  18:22:54  louie
 * New version of UNIX NTP daemon and software based on the 6 March 1989
 * draft of the new NTP protocol specification.  This module attempts to
 * conform to the new logical clock described in section 5 of the spec.  Note
 * that the units of the drift_compensation register have changed.
 * 
 * This version also accumulates the residual adjtime() truncation and
 * adds it in on subsequent adjustments.
 * 
 * Revision 3.1.1.1  89/02/15  08:55:48  louie
 * *** empty log message ***
 * 
 * 
 * Revision 3.1  89/01/30  14:43:08  louie
 * Second UNIX NTP test release.
 * 
 * Revision 3.0  88/12/12  16:00:38  louie
 * Test release of new UNIX NTP software.  This version should conform to the
 * revised NTP protocol specification.
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <errno.h>
#include <syslog.h>

#include "ntp.h"

#ifdef	DEBUG
extern int debug;
#endif

extern int doset;
extern int debuglevel;
extern char *ntoa();
extern struct sysdata sys;

double	drift_comp = 0.0,
	compliance,
	clock_adjust;
long	update_timer = 0;

int	adj_precision;
double	adj_residual;
int	firstpass = 1;

#define	abs(x)	((x) < 0 ? -(x) : (x))

void
init_logical_clock()
{
	adj_precision = 1;
	/*
	 *  If you have the "fix" for adjtime() installed in you kernel, you'll
	 *  have to make sure that adj_precision is set to 1 here.
	 */
}


/*
 *  5.0 Logical clock procedure
 *
 *  Only paramter is an offset to vary the clock by, in seconds.  We'll either
 *  arrange for the clock to slew to accomodate the adjustment, or just preform
 *  a step adjustment if the offset is too large.
 *
 *  The update which is to be performed is left in the external
 *  clock_adjust. 
 *
 *  Returns non-zero if clock was reset rather than slewed.
 *
 *  Many thanks for Dennis Ferguson <dennis@gw.ccie.utoronto.ca> for his
 *  corrections to my code.
 */

int
adj_logical(offset)
	double offset;
{
	struct timeval tv1, tv2;
#ifdef	XADJTIME2
	struct timeval delta, olddelta;
#endif
	
	/*
	 *  Now adjust the logical clock
	 */
	if (!doset)
		return 0;

	adj_residual = 0.0;
	if (offset > CLOCK_MAX || offset < -CLOCK_MAX) {
		double steptime = offset;

		(void) gettimeofday(&tv2, (struct timezone *) 0);
		steptime += tv2.tv_sec;
		steptime += tv2.tv_usec / 1000000.0;
		tv1.tv_sec = steptime;
		tv1.tv_usec = (steptime - tv1.tv_sec) * 1000000;
#ifdef	DEBUG
		if (debug > 2) {
			steptime = (tv1.tv_sec + tv1.tv_usec/1000000.0) -
				(tv2.tv_sec + tv2.tv_usec/1000000.0);
			printf("adj_logical: %f %f\n", offset, steptime);
		}
#endif
		if (settimeofday(&tv1, (struct timezone *) 0) < 0) {
			syslog(LOG_ERR, "Can't set time: %m");
			return(-1);
		}
		clock_adjust = 0.0;
		firstpass = 1;
		update_timer = 0;
		return (1);	  /* indicate that step adjustment was done */
	} else 	{
		double ai;

		/*
		 * If this is our very first adjustment, don't touch
		 * the drift compensation (this is f in the spec
		 * equations), else update using the *old* value
		 * of the compliance.
		 */
		clock_adjust = offset;
		if (firstpass)
			firstpass = 0;
		else if (update_timer > 0) {
			ai = abs(compliance);
			ai = (double)(1<<CLOCK_COMP) - 
				(double)(1<<CLOCK_FACTOR) * ai;
			if (ai < 1.0)		/* max(... , 1.0) */
				ai = 1.0;
			drift_comp += offset / (ai * (double)update_timer);
		}

		/*
		 * Set the timer to zero.  adj_host_clock() increments it
		 * so we can tell the period between updates.
		 */
		update_timer = 0;

		/*
		 * Now update the compliance.  The compliance is h in the
		 * equations.
		 */
		compliance += (offset - compliance)/(double)(1<<CLOCK_TRACK);

#ifdef XADJTIME2
		delta.tv_sec = offset;
		delta.tv_usec = (offset - delta.tv_sec) * 1000;
		(void) adjtime2(&delta, &olddelta);
#endif
		return(0);
	}
}

#ifndef	XADJTIME2
extern int adjtime();

/*
 *  This is that routine that performs the periodic clock adjustment.
 *  The procedure is best described in the the NTP document.  In a
 *  nutshell, we prefer to do lots of small evenly spaced adjustments.
 *  The alternative, one large adjustment, creates two much of a
 *  clock disruption and as a result oscillation.
 *
 *  This function is called every 2**CLOCK_ADJ seconds.
 *
 */

/*
 * global for debugging?
 */
double adjustment;

void
adj_host_clock()
{

	struct timeval delta, olddelta;

	if (!doset)
		return;

	/*
	 * Add update period into timer so we know how long it
	 * took between the last update and the next one.
	 */
	update_timer += 1<<CLOCK_ADJ;
	/*
	 * Should check to see if update_timer > 1 day here?
	 */

	/*
	 * Compute phase part of adjustment here and update clock_adjust.
	 * Note that the equations used here are implicit in the last
	 * two equations in the spec (in particular, look at the equation
	 * for g and figure out how to  find the k==1 term given the k==0 term.)
	 */
	adjustment = clock_adjust / (double)(1<<CLOCK_PHASE);
	clock_adjust -= adjustment;

	/*
	 * Now add in the frequency component.  Be careful to note that
	 * the ni occurs in the last equation since those equations take
	 * you from 64 second update to 64 second update (ei is the total
	 * adjustment done over 64 seconds) and we're only deal in the
	 * little 4 second adjustment interval here.
	 */
	adjustment += drift_comp / (double)(1<<CLOCK_FREQ);

	/*
	 * Add in old adjustment residual
	 */
	adjustment += adj_residual;

	/*
	 * Simplify.  Adjustment shouldn't be bigger than 2 ms.  Hope
	 * writer of spec was truth telling.
	 */
#ifdef	DEBUG
	delta.tv_sec = adjustment;
	if (debug && delta.tv_sec) abort();
#else
	delta.tv_sec = 0;
#endif
	delta.tv_usec = ((long)(adjustment * 1000000.0) / adj_precision)
		   * adj_precision;

	adj_residual = adjustment - (double) delta.tv_usec / 1000000.0;

	if (delta.tv_usec == 0)
		return;

	if (adjtime(&delta, &olddelta) < 0)
		syslog(LOG_ERR, "Can't adjust time: %m");

#ifdef	DEBUG
	if(debug > 2)
		printf("adj: %ld us  %f %f\n",
		       delta.tv_usec, drift_comp, clock_adjust);
#endif
}
#endif
