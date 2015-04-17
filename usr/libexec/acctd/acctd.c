/*
 * Steven Schultz - sms@moe.2bsd.com
 *
 *	@(#)acctd.c	1.0 (2.11BSD) 1999/2/10
 *
 * acctd - process accounting daemon
*/

#include	<signal.h>
#include	<stdio.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<string.h>
#include	<syslog.h>
#include	<varargs.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/acct.h>
#include	<sys/mount.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/resource.h>

	struct	ACCTPARM
		{
		int	suspend;
		int	resume;
		int	chkfreq;
		char	*acctfile;	/* malloc'd */
		};

	int	Suspend = 2;	/* %free when accounting suspended */
	int	Resume = 4;	/* %free when accounting to be resumed */
	int	Chkfreq = 30;	/* how often (seconds) to check disk space */
	int	Debug;
	int	Disabled;
	char	*Acctfile;
	int	Alogfd;
	struct	ACCTPARM Acctparms;
	FILE	*Acctfp;
	char	*Acctdcf = _PATH_ACCTDCF;
	int	checkacctspace(), hupcatch(), terminate();
	void	usage(), errline();
	void	die(), reportit();

extern	char	*__progname;

main(argc, argv)
	int	argc;
	char	**argv;
	{
	int	c, i;
	pid_t	pid;
	register FILE *fp;
	struct	ACCTPARM ajunk;
	sigset_t smask;
	struct	sigaction sa;

	if	(getuid())
		die("%s", "Only root can run this program");

	opterr = 0;
	while	((c = getopt(argc, argv, "d")) != EOF)
		{
		switch	(c)
			{
			case	'd':
				Debug++;
				break;
			case	'?':
			default:
				usage();
				/* NOTREACHED */
			}
		}
	argc -= optind;
	argv += optind;
	if	(argc != 0)
		{
		usage();
		/* NOTREACHED */
		}
/*
 * Catch the signals of interest and ignore the ones that could get generated 
 * from the keyboard.  If additional signals are caught remember to add them
 * to the masks of the other signals!
*/
	daemon(0,0);

	sigemptyset(&smask);
	sigaddset(&smask, SIGTERM);
	sigaddset(&smask, SIGHUP);
	sa.sa_handler = checkacctspace;
	sa.sa_mask = smask;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);

	sigemptyset(&smask);
	sigaddset(&smask, SIGALRM);
	sigaddset(&smask, SIGHUP);
	sa.sa_handler = terminate;
	sa.sa_mask = smask;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGTERM, &sa, NULL);

	sigemptyset(&smask);
	sigaddset(&smask, SIGALRM);
	sigaddset(&smask, SIGTERM);
	sa.sa_handler = hupcatch;
	sa.sa_mask = smask;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGHUP, &sa, NULL);

	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	if	(parseconf(&ajunk) < 0)
		die("%s owner/mode/reading/parsing error", Acctdcf);
	reconfig(&ajunk);
/*
 * The conf file has been opened, parsed/validated and output file created.
 * It's time to open the accounting log device.
 *
 * The open is retried a few times (using usleep which does not involve
 * signals or alarms) because the previous 'acctd' may be in its SIGTERM 
 * handling - see the comments in terminate().   Could try longer perhaps.
*/
	for	(i = 0; i < 4; i++)
		{
		Alogfd = open(_PATH_DEVALOG, O_RDONLY);
		if	(Alogfd > 0)
			break;
		usleep(1100000L);
		}
	if	(Alogfd < 0)
		die("open(%s) errno: %d", _PATH_DEVALOG, errno);
/*
 * Save our pid for 'accton' to use
*/
	fp = fopen(_PATH_ACCTDPID, "w");
	pid = getpid();
	if	(!fp)
		die("fopen(%s,w) error %d\n", _PATH_ACCTDPID, errno);
	fprintf(fp, "%d\n", pid);
	fclose(fp);

/*
 * Raise our priority slightly.  The kernel can buffer quite a bit but
 * if the system gets real busy we might be starved for cpu time and lose
 * accounting events.  We do not run often or for long so this won't impact
 * the system too much.
*/
	setpriority(PRIO_PROCESS, pid, -1);
	doit();
	/* NOTREACHED */
	}

/*
 * The central loop is here.  Try to read 4 accounting records at a time
 * to cut the overhead down some.  
*/

doit()
	{
	struct	acct	abuf[4];
	struct	ACCTPARM ajunk;
	sigset_t	smask, omask;
	int	len;
	
	while	(1)
		{
/*
 * Should a check for 'n' being a multiple of 'sizeof struct acct' be made?
 * No.   The kernel's operations are atomic and we're using SA_RESTART, either
 * we get all that we asked for or we stay suspended.
*/
		len = read(Alogfd, abuf, sizeof (abuf));
		if	(len < 0)
			{
/*
 * Shouldn't happen.  If it does then it's best to log the error and die
 * rather than go into an endless loop of retrying the read.  Since SA_RESTART
 * is used on the signals we will not see EINTR.
*/
			die("doit read(%d,...): %d\n", Alogfd, errno);
			}
/*
 * If accounting has not been disabled and an accounting file is open
 * write the data out.  Probably should save the current position and 
 * truncate the file if the write fails.   Hold off signals so things don't
 * change while writing (this makes it safe for the signal handlers to do
 * more than just set a flag).
*/
		sigemptyset(&smask);
		sigaddset(&smask, SIGHUP);
		sigaddset(&smask, SIGTERM);
		sigaddset(&smask, SIGALRM);
		if	(sigprocmask(SIG_BLOCK, &smask, &omask) < 0)
			die("doit() sigprocmask(BLOCK) errno=%d\n", errno);
		if	(!Disabled)
			fwrite(abuf, len, 1, Acctfp);
		sigprocmask(SIG_SETMASK, &omask, NULL);
		}
	}

checkacctspace()
	{
	struct	statfs	fsb;
	float	suspendfree, totalfree, resumefree;

	if	(fstatfs(fileno(Acctfp), &fsb) < 0)
		die("checkacctspace(%d) errno: %d\n", fileno(Acctfp), errno);
	totalfree = (float)fsb.f_bfree;
	suspendfree = ((float)fsb.f_blocks * (float)Acctparms.suspend) / 100.0;

	if	(totalfree <= suspendfree)
		{
		if	(!Disabled)
			reportit("less than %d%% freespace on %s, accounting suspended\n", Acctparms.suspend, fsb.f_mntfromname);
		Disabled = 1;
		return(0);
		}
/*
 * If accounting is not disabled then just return.  If it has been disabled
 * check if enough space is free to resume accounting.
*/
	if	(!Disabled)
		return(0);

	resumefree = ((float)fsb.f_blocks * (float)Acctparms.resume) / 100.0;
	if	(totalfree >= resumefree)
		{
		reportit("more than %d%% freespace on %s, accounting resumed\n",
			Acctparms.resume, fsb.f_mntfromname);
		Disabled = 0;
		}
	return(0);
	}

/*
 * When a SIGHUP is received parse the config file.  It is safe to do this
 * in the signal handler because other signals are blocked.
*/

hupcatch()
	{
	struct	ACCTPARM ajunk;

/*
 * What to do if the config file is banged up or has wrong mode/owner...?
 * Safest thing to do is log a message and exit rather than continue with
 * old information or trust corrupted new information.
*/
	if	(parseconf(&ajunk) < 0)
		die("%s owner/mode/reading/parsing error", Acctdcf);
	reconfig(&ajunk);
	}

/*
 * init(8) used to turn off accounting via the old acct(2) syscall when
 * the system went into single user mode on a shutdown.  Since 'acctd' is
 * just another user process as far as init(8) is concerned we receive a
 * SIGTERM when the system is being shutdown.  In order to capture as much
 * data as possible we delay exiting for a few seconds (can't be too long
 * because init(8) will SIGKILL 'hung' processes).  
 *
 * Mark the accounting device nonblocking and read data until either 
 * nothing is available or we've gone thru the maximum delay.  The same
 * assumption is made here as in doit() - that the reads are atomic, we
 * either get all that we asked for or nothing.
*/
terminate()
	{
	register int	i, cnt;
	struct	acct	a;

	if	(fcntl(Alogfd, F_SETFL, O_NONBLOCK) < 0)
		reportit("fcntl(%d): %d\n", Alogfd, errno); 
	for	(i = 0; Acctfp && i < 3; i++)
		{
		while	((cnt = read(Alogfd, &a, sizeof (a)) > 0))
			fwrite(&a, sizeof (a), 1, Acctfp);
		usleep(1000000L);
		}
	if	(Acctfp)
		fclose(Acctfp);
	close(Alogfd);
	exit(0);
	}

/*
 * Parse the conf file.  The parse is _extremely_ simple minded because
 * only 'accton' should be writing the file.  If manual editing is done
 * be very careful not to add extra whitespace (or comments).  Sanity/range
 * checking of the arguments is performed here.
*/
parseconf(ap)
	register struct ACCTPARM *ap;
	{
	int	err = 0, count;
	register FILE *fp;
	char	line[256], *cp;
	long	l;
	struct	stat st;

/*
 * The conf file must be owned by root and not writeable by group or other.  
 * This is because the conf file contains a pathname that will be trusted 
 * by this program and it is running as root.
*/
	fp = fopen(Acctdcf, "r");
	if	(!fp)
		return(-1);
	if	(fstat(fileno(fp), &st) == 0)
		{
		if	((st.st_uid != 0) || (st.st_mode & (S_IWGRP|S_IWOTH)))
			{
			fclose(fp);
			return(-1);
			}
		}
	bzero(ap, sizeof (*ap));
	for	(count = 1; fgets(line, sizeof (line), fp) && !err; count++)
		{
		cp = index(line, '\n');
		if	(cp)
			*cp = '\0';
		if	(bcmp(line, "suspend=", 8) == 0)
			{
			l = strtol(line + 8, &cp, 10);
			if	(l < 0 || l > 99 || (cp && *cp))
				{
				errline(count);
				err = -1;
				}
			ap->suspend = (int)l;
			}
		else if	(bcmp(line, "resume=", 7) == 0)
			{
			l = strtol(line + 7, &cp, 10);
			if	(l < 0 || l > 99 || (cp && *cp))
				{
				errline(count);
				err = -1;
				}
			ap->resume = (int)l;
			}
		else if	(bcmp(line, "chkfreq=", 8) == 0)
			{
			l = strtol(line + 8, &cp, 10);
/*
 * Doesn't make sense to check more often than every 10 seconds.  Put a
 * upper bound of an hour.
*/
			if	(l < 10 || l > 3600 || (cp && *cp))
				{
				errline(count);
				err = -1;
				}
			ap->chkfreq = (int)l;
			}
		else if	(bcmp(line, "acctfile=", 9) == 0)
			{
			cp = line + 9;
			if	(ap->acctfile)
				free(ap->acctfile);
			ap->acctfile = strdup(cp);
			}
		else
/*
 * An unknown string could be the sign of a corrupted file.  Declare an error
 * so we don't trust potential garbage.
*/
			{
			errline(count);
			err = -1;
			}
		}
	fclose(fp);
	if	(err)
		{
		if	(ap->acctfile)
			{
			free(ap->acctfile);
			ap->acctfile = NULL;
			}
		return(err);
		}
/* 
 * Now see which fields were not filled in and apply the defaults.  The
 * 'accton' program does this but if the conf file was manually edited some
 * fields may have been left out.  Basic range checking has already been done
 * if the fields were present.
*/
	if	(ap->suspend == 0)
		ap->suspend = Suspend;
	if	(ap->resume == 0)
		ap->resume = Resume;
	if	(ap->chkfreq == 0)
		ap->chkfreq  = Chkfreq;
	if	(ap->acctfile == NULL)
		ap->acctfile = strdup(_PATH_ACCTFILE);
	return(0);
	}

void
errline(l)
	{
	reportit("error in line %d of %s\n", l, Acctdcf);
	}

/*
 * This routine completes the reconfiguration of the accounting daemon.  The
 * parsing and validation has been performed by parseconf() and the results
 * stored in a structure (a pointer to which is passed to this routine). 
*/

reconfig(new)
	struct ACCTPARM *new;
	{
	struct	itimerval  itmr;
	int	fd;

	if	(Acctfp)
		fclose(Acctfp);
	if	(Acctparms.acctfile)
		free(Acctparms.acctfile);
	Acctparms = *new;

	fd = open(Acctparms.acctfile, O_WRONLY | O_APPEND, 644);
	if	(fd < 0)
		die("open(%s,O_WRONLY|O_APPEND): %d\n", Acctparms.acctfile, 
			errno);
	Acctfp = fdopen(fd, "a");
	if	(!Acctfp)
		die("fdopen(%d,a): %d\n", fd, errno);
	itmr.it_interval.tv_sec = Acctparms.chkfreq;
	itmr.it_interval.tv_usec = 0;
	itmr.it_value.tv_sec = Acctparms.chkfreq;
	itmr.it_value.tv_usec = 0;
	if	(setitimer(ITIMER_REAL, &itmr, NULL) < 0)
		die("setitmer: %d\n", errno);
	}

/*
 * The logfile is opened/closed per message to conserve resources
 * (file table and descriptor).  In the case of die() this isn't terribly
 * important since we're about to exit anyhow ;)  For reportit() the
 * messages are of such low frequency that an extra openlog/closelog
 * pair isn't too much extra overhead.
*/

void
die(str, va_alist)
	char	*str;
	va_dcl
	{
	va_list ap;

	openlog("acctd", LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
	va_start(ap);
	vsyslog(LOG_ERR, str, ap);
	va_end(ap);
	exit(1);
	}

void
reportit(str, va_alist)
	char	*str;
	va_dcl
	{
	va_list ap;

	openlog("acctd", LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
	va_start(ap);
	vsyslog(LOG_WARNING, str, ap);
	va_end(ap);
	}

void
usage()
	{

	die("Usage: %s [-f acctfile] [-s %suspend] [-r %resume] [-t chkfreq] [acctfile]", __progname);
	/* NOTREACHED */
	}
