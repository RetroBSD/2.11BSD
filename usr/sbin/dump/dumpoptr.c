#if	!defined(lint) && defined(DOSCCS)
static	char *sccsid = "@(#)dumpoptr.c	1.6 (2.11BSD GTE) 1996/11/16";
#endif

#include "dump.h"

struct	group *getgrnam();
/*
 *	Query the operator; This fascist piece of code requires
 *	an exact response.
 *	It is intended to protect dump aborting by inquisitive
 *	people banging on the console terminal to see what is
 *	happening which might cause dump to croak, destroying
 *	a large number of hours of work.
 *
 *	Every 2 minutes we reprint the message, alerting others
 *	that dump needs attention.
 */
int	timeout;
char	*attnmessage;		/* attemtion message */

query(question)
	char	*question;
{
	char	replybuffer[64];
	int	back;
	FILE	*mytty;

	if ( (mytty = fopen("/dev/tty", "r")) == NULL){
		msg("fopen on /dev/tty fails\n");
		abort();
	}
	attnmessage = question;
	timeout = 0;
	alarmcatch();
	for(;;){
		if ( fgets(replybuffer, 63, mytty) == NULL){
			if (ferror(mytty)){
				clearerr(mytty);
				continue;
			}
		} else if ( (strcmp(replybuffer, "yes\n") == 0) ||
			    (strcmp(replybuffer, "Yes\n") == 0)){
				back = 1;
				goto done;
		} else if ( (strcmp(replybuffer, "no\n") == 0) ||
			    (strcmp(replybuffer, "No\n") == 0)){
				back = 0;
				goto done;
		} else {
			msg("\"Yes\" or \"No\"?\n");
			alarmcatch();
		}
	}
    done:
	/*
	 *	Turn off the alarm, and reset the signal to trap out..
	 */
	alarm(0);
	if (signal(SIGALRM, sigalrm) == SIG_IGN)
		signal(SIGALRM, SIG_IGN);
	fclose(mytty);
	return(back);
}
/*
 *	Alert the console operator, and enable the alarm clock to
 *	sleep for 2 minutes in case nobody comes to satisfy dump
 */
alarmcatch()
{
	if (timeout)
		msgtail("\n");
	msg("NEEDS ATTENTION: %s: (\"yes\" or \"no\") ",
		attnmessage);
	signal(SIGALRM, alarmcatch);
	alarm(120);
	timeout = 1;
}
/*
 *	Here if an inquisitive operator interrupts the dump program
 */
interrupt()
{
	msg("Interrupt received.\n");
	if (query("Do you want to abort dump?"))
		dumpabort();
	signal(SIGINT, interrupt);
}

/*
 *	The following variables and routines manage alerting
 *	operators to the status of dump.
 *	This works much like wall(1) does.
 */
struct	group *gp;

/*
 *	Get the names from the group entry "operator" to notify.
 */	
set_operators()
{
	if (!notify)		/*not going to notify*/
		return;
	gp = getgrnam(OPGRENT);
	endgrent();
	if (gp == (struct group *)0){
		msg("No entry in /etc/group for %s.\n",
			OPGRENT);
		notify = 0;
		return;
	}
}

struct tm *localtime();
struct tm *localclock;

/*
 *	We fork a child to do the actual broadcasting, so
 *	that the process control groups are not messed up
 */
broadcast(message)
	char	*message;
{
	time_t		clock;
	FILE	*f_utmp;
	struct	utmp	utmp;
	char	**np;
	int	pid, s;

	switch (pid = fork()) {
	case -1:
		return;
	case 0:
		break;
	default:
		while (wait(&s) != pid)
			continue;
		return;
	}

	if (!notify || gp == 0)
		exit(0);
	clock = time((time_t *)0);
	localclock = localtime(&clock);

	if((f_utmp = fopen(_PATH_UTMP, "r")) == NULL) {
		msg("Cannot open 'utmp'\n");
		return;
	}

	while (!feof(f_utmp)){
		if (fread((char *)&utmp, sizeof (struct utmp), 1, f_utmp) != 1)
			break;
		if (utmp.ut_name[0] == 0)
			continue;
		for (np = gp->gr_mem; *np; np++){
			if (strncmp(*np, utmp.ut_name, sizeof(utmp.ut_name)) != 0)
				continue;
			/*
			 *	Do not send messages to operators on dialups
			 */
			if (strncmp(utmp.ut_line, DIALUP, strlen(DIALUP)) == 0)
				continue;
#ifdef DEBUG
			msg("Message to %s at %s\n",
				utmp.ut_name, utmp.ut_line);
#endif DEBUG
			sendmes(utmp.ut_line, message);
		}
	}
	fclose(f_utmp);
	Exit(0);	/* the wait in this same routine will catch this */
	/* NOTREACHED */
}

sendmes(tty, message)
char *tty, *message;
{
	char t[50], buf[BUFSIZ];
	register char *cp;
	register int c;
	int	ch, msize;
	register FILE *f_tty;

	msize = strlen(message);
	strcpy(t, "/dev/");
	strcat(t, tty);

	if((f_tty = fopen(t, "w")) != NULL) {
		setbuf(f_tty, buf);
		fprintf(f_tty, "\n\007\007\007Message from the dump program to all operators at %d:%02d ...\r\n\n"
		       ,localclock->tm_hour
		       ,localclock->tm_min);
		for (cp = message, c = msize; c-- > 0; cp++) {
			ch = *cp;
			if (ch == '\n')
				putc('\r', f_tty);
			putc(ch, f_tty);
		}
		fclose(f_tty);
	}
}

/*
 *	print out an estimate of the amount of time left to do the dump
 */

time_t	tschedule = 0;

timeest()
{
	time_t	tnow, deltat;
	long	pleft;
	char buf[BUFSIZ];

	time (&tnow);
	if (tnow >= tschedule){
		tschedule = tnow + 300;
		if (blockswritten < 500L)
			return;	
/*
			/				    \
			|/ estimated blocks \		    |
	delta time =	||----------------  | X time so far |  - time so far
			|\ blocks thus far  /		    |
			\				    /
*/
		pleft  = ((100 * esize)/blockswritten) - 100;
		deltat = (time_t) (tnow - tstart_writing) * pleft;
		deltat = deltat/100;	/* scale back down */
		sprintf(buf,"%3.2f%%%% done, finished in %d:%02d\n",
			(float) (blockswritten*100.0)/esize,
			(int)(deltat/3600L), (int)((deltat%3600L)/60L));
		msg(buf);
	}
}

/* VARARGS1 */
/* ARGSUSED */
msg(fmt, a1, a2, a3, a4, a5)
	char	*fmt;
{
	fprintf(stderr,"  DUMP: ");
#ifdef TDEBUG
	fprintf(stderr,"pid=%d ", getpid());
#endif
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fflush(stdout);
	fflush(stderr);
}

/* VARARGS1 */
/* ARGSUSED */
msgtail(fmt, a1, a2, a3, a4, a5)
	char	*fmt;
{
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
}

/*
 *	Tell the operator what has to be done.
 */
lastdump(arg)
	char	arg;	/* w ==> just what to do; W ==> most recent dumps */
{
			char	*lastname;
			char	*date;
	register	int	i;
			time_t	tnow;
	register	struct	fstab	*dt;
			int	dumpme;
	register	struct	idates	*itwalk;

	int	idatesort();

	time(&tnow);
	inititimes();		/* /etc/dumpdates input */
	qsort(idatev, nidates, sizeof(struct idates *), idatesort);

	if (arg == 'w')
		fprintf(stdout, "Dump these file systems:\n");
	else
		fprintf(stdout, "Last dump(s) done (Dump '>' file systems):\n");
	lastname = "??";
	ITITERATE(i, itwalk){
		if (strncmp(lastname, itwalk->id_name, sizeof(itwalk->id_name)) == 0)
			continue;
		date = (char *)ctime(&itwalk->id_ddate);
		date[16] = '\0';	/* blast away seconds and year */
		lastname = itwalk->id_name;
		dt = getfsspec(deraw(itwalk->id_name));
		dumpme = (  (dt != 0)
			 && (dt->fs_freq != 0)
			 && (itwalk->id_ddate < tnow - (dt->fs_freq*DAY)));
		if ( (arg != 'w') || dumpme)
		  fprintf(stdout,"%c %8s\t(%6s) Last dump: Level %c, Date %s\n",
			dumpme && (arg != 'w') ? '>' : ' ',
			itwalk->id_name,
			dt ? dt->fs_file : 0,
			itwalk->id_incno,
			date
		    );
	}
}

int	idatesort(p1, p2)
	struct	idates	**p1, **p2;
{
	int	diff;

	diff = strncmp((*p1)->id_name, (*p2)->id_name, sizeof((*p1)->id_name));
	if (diff == 0)
		if ((*p2)->id_ddate > (*p1)->id_ddate)
			return (1);
		else
			if ((*p2)->id_ddate == (*p1)->id_ddate)
				return (0);
			else
				return (-1);
	else
		return (diff);
}
