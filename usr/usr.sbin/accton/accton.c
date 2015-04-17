/*
 * Steven Schultz - sms@moe.2bsd.com
 *
 *	@(#)accton.c	1.1 (2.11BSD) 1999/5/5
 *
 * accton - enable/disable process accounting.
*/

#include	<signal.h>
#include	<stdio.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/acct.h>
#include	<sys/stat.h>

static	int	Suspend = 2;	/* %free when accounting suspended */
static	int	Resume = 4;	/* %free when accounting to be resumed */
static	int	Chkfreq = 30;	/* how often (seconds) to check disk space */
static	char	*Acctfile;
extern	char	*__progname;
	void	usage();

main(argc, argv)
	int	argc;
	char	**argv;
	{
	int	c;
	pid_t	pid;
	char	*cffile = _PATH_ACCTDCF;
	char	*pidfile = _PATH_ACCTDPID;
	char	*cp;
	long	l;
	register FILE	*fp;
	int	status;
	struct	stat	st;

	if	(getuid())
		errx(1, "Only root can run this program");
/*
 * Handle the simple case of no arguments at all.  This turns off accounting
 * completely by killing the accounting daemon 'acctd'.
*/
	if	(argc == 1)
		{
		fp = fopen(pidfile, "r");
/*
 * Note: it is not fatal to fail opening the pid file.  The accounting daemon
 * may not have been run on this system yet.
*/
		if	(!fp)
			exit(0);
		if	(fscanf(fp, "%d\n", &pid) != 1)
			errx(1, "%s corrupt/unparseable",
				pidfile);
		if	(pid < 3 || pid > 30000) /* Paranoia */
			errx(1, "%s content out of bound(30000>pid>3)",
				pidfile);
		fclose(fp);
		if	(kill(pid, SIGTERM) < 0)
			{
/* 
 * It is not an error to turn off accounting if it is already disabled.  Ignore
 * the no such process error from kill.
*/
			if	(errno != ESRCH)
				err(1, "kill(%d,SIGTERM)", pid);
			}
		exit(0);
		}

	while	((c = getopt(argc, argv, "f:r:s:t:")) != EOF)
		{
		switch	(c)
			{
			case	'f':
				Acctfile = optarg;
				break;
			case	'r':
				cp = NULL;
				l = strtol(optarg, &cp, 10);
				if	(l < 0 || l > 99 || (cp && *cp))
					errx(1, "bad -r value");
				Resume = (int)l;
				break;
			case	's':
				cp = NULL;
				l = strtol(optarg, &cp, 10);
				if	(l < 0 || l > 99 || (cp && *cp))
					errx(1, "bad -s value");
				Suspend = (int)l;
				break;
			case	't':
				cp = NULL;
				l = strtol(optarg, &cp, 10);
				if	(l < 5 || l > 3600 || (cp && *cp))
					errx(1, "bad -t value (3600>=t>=5");
				Chkfreq = (int)l;
				break;
			case	'?':
			default:
				usage();
				/* NOTREACHED */
			}
		}
	argc -= optind;
	argv += optind;
/*
 * If we have exactly one argument left then it must be a filename.  This
 * preserves the historical practice of running "accton /usr/adm/acct" to
 * enable accounting.  It is an error to have more than one argument at this
 * point.
*/
	if	(argc == 1)
		{
		if	(Acctfile)
			warnx("'-f %s' being overridden by trailing '%s'",
				Acctfile, *argv);
		Acctfile = *argv;
		}
	else if (argc != 0)
		{
		usage();
		/* NOTREACHED */
		}
/*
 * If after all of that we still don't have a file name then use the 
 * default one.
*/
	if	(!Acctfile)
		Acctfile = _PATH_ACCTFILE;

/*
 * Now open the conf file (creating it if it does not exist) and write out
 * the parameters for the accounting daemon.  The conf file format is simple:
 * "tag=value\n".  NO EXTRA WHITE SPACE or COMMENTS!
 *
 * IF the file already exists it must be owned by root and not writeable by
 * group or other.  This is because the conf file contains a pathname that
 * will be trusted by a daemon running as root.  The same check is made by
 * 'acctd' to prevent believing bogus information.
*/
	if	(stat(cffile, &st) == 0)
		{
		if	(st.st_uid != 0)
			errx(1, "%s not owned by root", cffile);
		if	(st.st_mode & (S_IWGRP|S_IWOTH))
			errx(1, "%s writeable by group/other", cffile);
		}
	fp = fopen(cffile, "w");
	if	(!fp)
		errx(1, "fopen(%s,w)", cffile);
	fprintf(fp, "suspend=%d\n", Suspend);
	fprintf(fp, "resume=%d\n", Resume);
	fprintf(fp, "chkfreq=%d\n", Chkfreq);
	fprintf(fp, "acctfile=%s\n", Acctfile);
	fclose(fp);

/*
 * After writing the conf file we need to send the current running 'acctd'
 * process (if any) a SIGHUP.  If the pidfile can not be opened this may be
 * the first time 'acctd' has ever been run.
*/
	fp = fopen(pidfile, "r");
	if	(fp)
		{
		if	(fscanf(fp, "%d\n", &pid) != 1)
			errx(1, "%s corrupt/unparseable", pidfile);
		if	(pid < 3 || pid > 30000) /* Paranoia */
			errx(1, "%s content out of bound(30000>pid>3)",
				pidfile);
		fclose(fp);
/*
 * If the signal can be successfully posted to the process then do not
 * attempt to start another instance of acctd (it will fail but syslog
 * an annoying error message).  If the signal can not be posted but the
 * acctd process does not exist then go start it.  Otherwise complain.
*/
		if	(kill(pid, SIGHUP) < 0)
			{
			if	(errno != ESRCH)
				err(1, "%d from %s bogus value", pid, pidfile);
			/* process no longer exists, fall thru and start it */
			}
		else
			exit(0);
		}
	pid = vfork();
	switch	(pid)
		{
		case	-1:
			err(1, "vfork");
			/* NOTREACHED */
		case	0:		/* Child process */
			if	(execl(_PATH_ACCTD, "acctd", NULL) < 0)
				err(1, "execl %s", _PATH_ACCTD);
			/* NOTREACHED */
		default:		/* Parent */
			break;
		}
	if	(waitpid(pid, &status, 0) < 0)
		err(1, "waitpid for %d", pid);
	exit(0);
	}

void
usage()
	{

	errx(1,"Usage: %s [-f acctfile] [-s %suspend] [-r %resume] [-t chkfreq] [acctfile]", __progname);
	/* NOTREACHED */
	}
