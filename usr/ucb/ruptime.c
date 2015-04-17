/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)ruptime.c	5.3.1 (2.11BSD) 1997/4/18";
#endif

#include <sys/param.h>
#include <stdio.h>
#include <sys/dir.h>
#include <protocols/rwhod.h>

DIR	*dirp;

#define	NHOSTS	100
int	nhosts;
struct	hs {
	struct	whod *hs_wd;
	int	hs_nusers;
} hs[NHOSTS];
struct	whod awhod;
int	hscmp(), ucmp(), lcmp(), tcmp();

#define	WHDRSIZE	(sizeof (awhod) - sizeof (awhod.wd_we))
#define	RWHODIR		"/usr/spool/rwho"

char	*interval();
time_t	now;
char	*malloc(), *sprintf();
int	aflg;
int 	rflg = 1;

#define down(h)		(now - (h)->hs_wd->wd_recvtime > 11 * 60)

main(argc, argv)
	int argc;
	char **argv;
{
	struct direct *dp;
	int f, i;
	time_t t;
	char buf[sizeof(struct whod)]; int cc;
	char *name;
	register struct hs *hsp = hs;
	register struct whod *wd;
	register struct whoent *we;
	int maxloadav = 0;
	int (*cmp)() = hscmp;

	name = *argv;
	while (*++argv) 
		while (**argv)
			switch (*(*argv)++) {
			case 'a':
				aflg++;
				break;
			case 'l':
				cmp = lcmp;
				break;
			case 'u':
				cmp = ucmp;
				break;
			case 't':
				cmp = tcmp;
				break;
			case 'r':
				rflg = -rflg;
				break;
			case '-':
				break;
			default: 
				fprintf(stderr, "Usage: %s [ -ar [ lut ] ]\n",
					name);
				exit (1);
			}
	time(&t);
	if (chdir(RWHODIR) < 0) {
		perror(RWHODIR);
		exit(1);
	}
	dirp = opendir(".");
	if (dirp == NULL) {
		perror(RWHODIR);
		exit(1);
	}
	while (dp = readdir(dirp)) {
		if (dp->d_ino == 0)
			continue;
		if (strncmp(dp->d_name, "whod.", 5))
			continue;
		if (nhosts == NHOSTS) {
			fprintf(stderr, "too many hosts\n");
			exit(1);
		}
		f = open(dp->d_name, 0);
		if (f > 0) {
			cc = read(f, buf, sizeof(struct whod));
			if (cc >= WHDRSIZE) {
				hsp->hs_wd = (struct whod *)malloc(WHDRSIZE);
				wd = (struct whod *)buf;
				bcopy(buf, hsp->hs_wd, WHDRSIZE);
				hsp->hs_nusers = 0;
				for (i = 0; i < 2; i++)
					if (wd->wd_loadav[i] > maxloadav)
						maxloadav = wd->wd_loadav[i];
				we = (struct whoent *)(buf+cc);
				while (--we >= wd->wd_we)
					if (aflg || we->we_idle < 3600)
						hsp->hs_nusers++;
				nhosts++; hsp++;
			}
		}
		(void) close(f);
	}
	(void) time(&now);
	qsort((char *)hs, nhosts, sizeof (hs[0]), cmp);
	if (nhosts == 0) {
		printf("no hosts!?!\n");
		exit(1);
	}
	for (i = 0; i < nhosts; i++) {
		hsp = &hs[i];
		if (down(hsp)) {
			printf("%-12.12s%s\n", hsp->hs_wd->wd_hostname,
			    interval(now - hsp->hs_wd->wd_recvtime, "down"));
			continue;
		}
		printf("%-12.12s%s,  %4d user%s  load %*.2f, %*.2f, %*.2f\n",
		    hsp->hs_wd->wd_hostname,
		    interval(hsp->hs_wd->wd_sendtime -
			hsp->hs_wd->wd_boottime, "  up"),
		    hsp->hs_nusers,
		    hsp->hs_nusers == 1 ? ", " : "s,",
		    maxloadav >= 1000 ? 5 : 4,
			hsp->hs_wd->wd_loadav[0] / 100.0,
		    maxloadav >= 1000 ? 5 : 4,
		        hsp->hs_wd->wd_loadav[1] / 100.0,
		    maxloadav >= 1000 ? 5 : 4,
		        hsp->hs_wd->wd_loadav[2] / 100.0);
		cfree(hsp->hs_wd);
	}
	exit(0);
}

char *
interval(time, updown)
	long time;
	char *updown;
{
	static char resbuf[32];
	long days, hours, minutes;

	if (time < 0 || time > 3L* 365L*24L*60L*60L) {
		(void) sprintf(resbuf, "   %s ??:??", updown);
		return (resbuf);
	}
	minutes = (time + 59) / 60;		/* round to minutes */
	hours = minutes / 60; minutes %= 60;
	days = hours / 24; hours %= 24;
	if (days)
		(void) sprintf(resbuf, "%s %3ld+%02ld:%02ld",
		    updown, days, hours, minutes);
	else
		(void) sprintf(resbuf, "%s    %3ld:%02ld",
		    updown, hours, minutes);
	return (resbuf);
}

hscmp(h1, h2)
	struct hs *h1, *h2;
{

	return (rflg * strcmp(h1->hs_wd->wd_hostname, h2->hs_wd->wd_hostname));
}

/*
 * Compare according to load average.
 */
lcmp(h1, h2)
	struct hs *h1, *h2;
{

	if (down(h1))
		if (down(h2))
			return (tcmp(h1, h2));
		else
			return (rflg);
	else if (down(h2))
		return (-rflg);
	else
		return (rflg * 
			(h2->hs_wd->wd_loadav[0] - h1->hs_wd->wd_loadav[0]));
}

/*
 * Compare according to number of users.
 */
ucmp(h1, h2)
	struct hs *h1, *h2;
{

	if (down(h1))
		if (down(h2))
			return (tcmp(h1, h2));
		else
			return (rflg);
	else if (down(h2))
		return (-rflg);
	else
		return (rflg * (h2->hs_nusers - h1->hs_nusers));
}

/*
 * Compare according to uptime.
 */
tcmp(h1, h2)
	struct hs *h1, *h2;
{
	long t1, t2;

	return (rflg * (
		(down(h2) ? h2->hs_wd->wd_recvtime - now
			  : h2->hs_wd->wd_sendtime - h2->hs_wd->wd_boottime)
		-
		(down(h1) ? h1->hs_wd->wd_recvtime - now
			  : h1->hs_wd->wd_sendtime - h1->hs_wd->wd_boottime)
	));
}
