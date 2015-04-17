#ifndef lint
static char	*sccsid = "@(#)serve.c	1.29	(Berkeley) 2/6/88";
#endif

/*
 * Main server routine
 */

#include "common.h"
#include <signal.h>
#ifdef USG
#include <sys/times.h>
#else
#include <sys/time.h>
#endif

#ifdef LOG
# ifndef USG
#  include <sys/resource.h>
# endif not USG
#endif

extern	int	ahbs(), group(), help(), ihave();
extern	int	list(), newgroups(), newnews(), nextlast(), post();
extern	int	slave(), stat(), xhdr();

static struct cmdent {
	char	*cmd_name;
	int	(*cmd_fctn)();
} cmdtbl[] = {
	"article",	ahbs,
	"body",		ahbs,
	"group",	group,
	"head",		ahbs,
	"help",		help,
	"ihave",	ihave,
	"last",		nextlast,
	"list",		list,
	"newgroups",	newgroups,
	"newnews",	newnews,
	"next",		nextlast,
	"post",		post,
	"slave",	slave,
	"stat",		ahbs,
#ifdef XHDR
	"xhdr",		xhdr,
#endif XHDR
};
#define NUMCMDS (sizeof(cmdtbl) / sizeof(struct cmdent))


/*
 * serve -- given a connection on stdin/stdout, serve
 *	a client, executing commands until the client
 *	says goodbye.
 *
 *	Parameters:	None.
 *
 *	Returns:	Exits.
 *
 *	Side effects:	Talks to client, does a lot of
 *			stuff.
 */

serve()
{
	char		line[NNTP_STRLEN];
	char		host[MAXHOSTNAMELEN];
	char		gdbuf[MAXBUFLEN];
	char		**argp;
	char		*timeptr, *cp;
	int		argnum, i;
	double		Tstart, Tfinish;
	double		user, sys;
#ifdef USG
	time_t		start, finish;
#else not USG
	struct timeval	start, finish;
#endif not USG
	extern char	*ctime();
#ifdef POSTER
	struct passwd	*pp;
#endif
#ifdef LOG
# ifdef USG
	struct tms	cpu;
# else not USG
	struct rusage	me, kids;
# endif not USG
# ifdef TIMEOUT
	void		timeout();
# endif
	
	grps_acsd = arts_acsd = 0;
#endif

	/* Not all systems pass fd's 1 and 2 from inetd */

	(void) close(1);
	(void) close(2);
	(void) dup(0);
	(void) dup(0);

	/* If we're ALONE, then we've already opened syslog */

#ifndef ALONE
# ifdef SYSLOG
#  ifdef BSD_42
	openlog("nntpd", LOG_PID);
#  else
	openlog("nntpd", LOG_PID, SYSLOG);
#  endif
# endif
#endif

#ifdef ALONE
#ifndef USG
	(void) signal(SIGCHLD, SIG_IGN);
#endif not USG
#endif

	/* Ignore SIGPIPE, since we'll see closed connections with read */

	(void) signal(SIGPIPE, SIG_IGN);

	/* Get permissions and see if we can talk to this client */

	host_access(&canread, &canpost, &canxfer, gdbuf);

	if (gethostname(host, sizeof(host)) < 0)
		(void) strcpy(host, "Amnesiac");

	if (!canread && !canxfer) {
		printf("%d %s NNTP server can't talk to you.  Goodbye.\r\n",
			ERR_ACCESS, host);
		(void) fflush(stdout);
#ifdef LOG
		syslog(LOG_INFO, "%s refused connection", hostname);
#endif
		exit(1);
	}

	/* If we can talk, proceed with initialization */

	ngpermcount = get_nglist(&ngpermlist, gdbuf);

#ifdef POSTER
	pp = getpwnam(POSTER);
	if (pp != NULL) {
		uid_poster = pp->pw_uid;
		gid_poster = pp->pw_gid;
	} else
#endif
		uid_poster = gid_poster = 0;

#ifndef FASTFORK
	num_groups = 0;
	num_groups = read_groups();	/* Read in the active file */
#else
	signal(SIGALRM, SIG_IGN);	/* Children don't deal with */
					/* these things */
#endif

	art_fp = NULL;
	argp = (char **) NULL;		/* for first time */

#ifdef USG
	(void) time(&start);
	Tstart = (double) start;
	timeptr = ctime(&start);
#else not USG
	(void) gettimeofday(&start, (struct timezone *)NULL);
	Tstart = (double) start.tv_sec - ((double)start.tv_usec)/1000000.0;
	timeptr = ctime(&start.tv_sec);
#endif not USG
	if ((cp = index(timeptr, '\n')) != NULL)
		*cp = '\0';
	else
		timeptr = "Unknown date";

	printf("%d %s NNTP server version %s ready at %s (%s).\r\n",
		canpost ? OK_CANPOST : OK_NOPOST,
		host, nntp_version,
		timeptr,
		canpost ? "posting ok" : "no posting");
	(void) fflush(stdout);

	/*
	 * Now get commands one at a time and execute the
	 * appropriate routine to deal with them.
	 */

#ifdef TIMEOUT
	(void) signal(SIGALRM, timeout);
	(void) alarm(TIMEOUT);
#endif TIMEOUT

	while (fgets(line, sizeof(line), stdin) != NULL) {
#ifdef TIMEOUT
		(void) alarm(0);
#endif TIMEOUT

		cp = index(line, '\r');		/* Zap CR-LF */
		if (cp != NULL)
			*cp = '\0';
		else {
			cp = index(line, '\n');
			if (cp != NULL)
				*cp = '\0';
		}

		if ((argnum = parsit(line, &argp)) == 0)
			continue;		/* Null command */
		else {
			for (i = 0; i < NUMCMDS; ++i)
				if (!strcasecmp(cmdtbl[i].cmd_name, argp[0]))
					break;
			if (i < NUMCMDS)
				(*cmdtbl[i].cmd_fctn)(argnum, argp);
			else {
				if (!strcasecmp(argp[0], "quit"))
					break;
#ifdef LOG
				syslog(LOG_INFO, "%s unrecognized %s",
					hostname,
					line);
#endif
				printf("%d Command unrecognized.\r\n",
					ERR_COMMAND);
				(void) fflush(stdout);
			}
		}
#ifdef TIMEOUT
		(void) alarm(TIMEOUT);
#endif TIMEOUT
	}

	printf("%d %s closing connection.  Goodbye.\r\n", OK_GOODBYE, host);
	(void) fflush(stdout);


#ifdef LOG
	if (ferror(stdout))
		syslog(LOG_ERR, "%s disconnect: %m", hostname);

#ifdef USG
	(void) time(&finish);
	Tfinish = (double) finish;

#ifndef HZ
#define	HZ	60.0	/* typical system clock ticks - param.h */
#endif not HZ

	(void) times(&cpu);
	user = (double)(cpu.tms_utime + cpu.tms_cutime) / HZ;
	sys  = (double)(cpu.tms_stime + cpu.tms_cstime) / HZ;
#else not USG
	(void) gettimeofday(&finish, (struct timezone *)NULL);
	Tfinish = (double) finish.tv_sec - ((double)finish.tv_usec)/1000000.0;

	(void) getrusage(RUSAGE_SELF, &me);
	(void) getrusage(RUSAGE_CHILDREN, &kids);

	user = (double) me.ru_utime.tv_sec + me.ru_utime.tv_usec/1000000.0 +
		kids.ru_utime.tv_sec + kids.ru_utime.tv_usec/1000000.0;
	sys = (double) me.ru_stime.tv_sec + me.ru_stime.tv_usec/1000000.0 +
		kids.ru_stime.tv_sec + kids.ru_stime.tv_usec/1000000.0;
#endif not USG
	if (grps_acsd)
		syslog(LOG_INFO, "%s exit %d articles %d groups",
			hostname, arts_acsd, grps_acsd);
	if (nn_told)
		syslog(LOG_INFO, "%s newnews_stats told %d took %d",
			hostname, nn_told, nn_took);
	if (ih_accepted || ih_rejected || ih_failed)
		syslog(LOG_INFO,
			"%s ihave_stats accepted %d rejected %d failed %d",
			hostname,
			ih_accepted,
			ih_rejected,
			ih_failed);
	(void) sprintf(line, "user %.1f system %.1f elapsed %.1f",
		user, sys, Tfinish - Tstart);
	syslog(LOG_INFO, "%s times %s", hostname, line);
#endif LOG

#ifdef PROFILE
	profile();
#endif

	exit(0);
}


#ifdef TIMEOUT
/*
 * No activity for TIMEOUT seconds, so print an error message
 * and close the connection.
 */

void
timeout()
{
	printf("%d Timeout after %d seconds, closing connection.\r\n",
		ERR_FAULT, TIMEOUT);
	(void) fflush(stdout);

#ifdef LOG
	syslog(LOG_ERR, "%s timeout", hostname);
#endif LOG

	exit(1);
}
#endif TIMEOUT
