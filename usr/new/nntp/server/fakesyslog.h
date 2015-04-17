/*
 * Fake syslog definitions.
 *
 * @(#)fakesyslog.h	1.1	(Berkeley) 12/18/87
 */

#ifdef FAKESYSLOG

/*
 *  Facility codes
 */

#define LOG_KERN	0
#define LOG_USER	0
#define LOG_MAIL	0
#define LOG_DAEMON	0
#define LOG_AUTH	0
#define LOG_SYSLOG	0
#define LOG_LPR		0
#define LOG_NEWS	0
#define LOG_LOCAL0	0
#define LOG_LOCAL1	0
#define LOG_LOCAL2	0
#define LOG_LOCAL3	0
#define LOG_LOCAL4	0
#define LOG_LOCAL5	0
#define LOG_LOCAL6	0
#define LOG_LOCAL7	0

#define LOG_NFACILITIES	0
#define LOG_FACMASK	0

/*
 *  Priorities
 */

#define LOG_EMERG	0
#define LOG_ALERT	0
#define LOG_CRIT	0
#define LOG_ERR		0
#define LOG_WARNING	0
#define LOG_NOTICE	0
#define LOG_INFO	0
#define LOG_DEBUG	0

#define LOG_PRIMASK	0

/*
 * arguments to setlogmask.
 */

#define	LOG_MASK(pri)	0
#define	LOG_UPTO(pri)	0

/*
 *  Option flags for openlog.
 */

#define	LOG_PID		0
#define	LOG_CONS	0
#define	LOG_ODELAY	0
#define LOG_NDELAY	0
#define LOG_NOWAIT	0

#endif FAKESYSLOG
