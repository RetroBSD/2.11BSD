/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)lastcomm.c	5.2.4 (2.11BSD GTE) 1997/5/7";
#endif

/*
 * lastcomm command
 */
#include <sys/param.h>
#include <sys/acct.h>
#include <sys/file.h>

#include <stdio.h>
#include <pwd.h>
#include <sys/stat.h>
#include <utmp.h>
#include <struct.h>
#include <ctype.h>
#include <stdlib.h>

struct	acct buf[DEV_BSIZE / sizeof (struct acct)];

time_t	expand();
char	*flagbits();
char	*getname();
char	*getdev();

main(argc, argv)
	char *argv[];
{
	register int bn, cc;
	register struct acct *acp;
	int fd, ch;
	struct stat sb;
	char *acctfile = "/usr/adm/acct";

	while	((ch = getopt(argc, argv, "f:")) != EOF)
		{
		switch	(ch)
			{
			case	'f':
				acctfile = optarg;
				break;
			case	'?':
			default:
				usage();
			}
		}
	argc -= optind;
	argv += optind;

	fd = open(acctfile, O_RDONLY);
	if (fd < 0)
		err(1, "%s", acctfile);

	fstat(fd, &sb);
	for (bn = btodb(sb.st_size); bn >= 0; bn--) {
		lseek(fd, (off_t)dbtob(bn), L_SET);
		cc = read(fd, buf, DEV_BSIZE);
		if (cc < 0) {
			perror("read");
			break;
		}
		acp = buf + (cc / sizeof (buf[0])) - 1;
		for (; acp >= buf; acp--) {
			register char *cp;
			time_t x;

			if (acp->ac_comm[0] == '\0')
				strcpy(acp->ac_comm, "?");
			for (cp = &acp->ac_comm[0];
			     cp < &acp->ac_comm[fldsiz(acct, ac_comm)] && *cp;
			     cp++)
				if (!isascii(*cp) || iscntrl(*cp))
					*cp = '?';
			if (*argv && !ok(argv, acp))
				continue;
			x = expand(acp->ac_utime) + expand(acp->ac_stime);
			printf("%-*.*s %s %-*s %-*s %6.2f secs %.16s\n",
				fldsiz(acct, ac_comm), fldsiz(acct, ac_comm),
				acp->ac_comm,
				flagbits(acp->ac_flag),
				fldsiz(utmp, ut_name), getname(acp->ac_uid),
				fldsiz(utmp, ut_line), getdev(acp->ac_tty),
				x / (double)AHZ, ctime(&acp->ac_btime));
		}
	}
}

time_t
expand (t)
	unsigned t;
{
	register time_t nt;

	nt = t & 017777;
	t >>= 13;
	while (t) {
		t--;
		nt <<= 3;
	}
	return (nt);
}

char *
flagbits(f)
	register int f;
{
	register int i = 0;
	static char flags[20];

#define BIT(flag, ch)	flags[i++] = (f & flag) ? ch : ' '
	BIT(ASU, 'S');
	BIT(AFORK, 'F');
	BIT(ACOMPAT, 'C');
	BIT(ACORE, 'D');
	BIT(AXSIG, 'X');
	flags[i] = '\0';
	return (flags);
}

ok(argv, acp)
	register char *argv[];
	register struct acct *acp;
{
	do {
		if (!strcmp(getname(acp->ac_uid), *argv))
			return(1);
		if (!strcmp(getdev(acp->ac_tty), *argv))
			return(1);
		if (!strncmp(acp->ac_comm, *argv, fldsiz(acct, ac_comm)))
			return(1);
	} while (*++argv);
	return (0);
}

/* should be done with nameserver or database */

struct	utmp utmp;
#define	NMAX	(sizeof (utmp.ut_name))
#define SCPYN(a, b)	strncpy(a, b, NMAX)

#define NCACHE	64		/* power of 2 */
#define CAMASK	NCACHE - 1

char *
getname(uid)
	uid_t uid;
{
	static struct ncache {
		uid_t	uid;
		char	name[NMAX+1];
	} c_uid[NCACHE];
	register struct passwd *pw;
	register struct ncache *cp;

	setpassent(1);
	cp = c_uid + (uid & CAMASK);
	if (cp->uid == uid && *cp->name)
		return(cp->name);
	if (!(pw = getpwuid(uid)))
		return((char *)0);
	cp->uid = uid;
	SCPYN(cp->name, pw->pw_name);
	return(cp->name);
}

char *
getdev(dev)
	dev_t dev;
{
	static dev_t lastdev = (dev_t) -1;
	static char *lastname;

	if (dev == NODEV)		/* Special case */
		return ("__");
	if (dev == lastdev)		/* One-element cache. */
		return (lastname);
	lastdev = dev;
	lastname = devname(dev, S_IFCHR);
	return (lastname);
}

void
usage()
	{

	(void)fprintf(stderr, 
		"lastcomm [ -f file ] [ command ...] [ user ...] [ tty ...]\n");
	exit(1);
	}
