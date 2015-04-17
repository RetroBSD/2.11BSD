/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)quota.c	5.4.3 (2.11BSD GTE) 1996/2/7";
#endif

/*
 * Disk quota reporting program.
 */
#include <stdio.h>
#include <fstab.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>

#include <sys/param.h>
#include <sys/quota.h>
#include <sys/file.h>
#include <sys/stat.h>

int	qflag;
int	vflag;
int	done;
int	morethanone;
char	*qfname = "quotas";

main(argc, argv)
	char *argv[];
{
	register char *cp;
	extern int errno;

	if (quota(Q_SYNC, 0, 0, (caddr_t)0) < 0 && errno == EINVAL) {
		fprintf(stderr, "There are no quotas on this system\n");
		exit(0);
	}
	argc--,argv++;
	while (argc > 0) {
		if (argv[0][0] == '-')
			for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

			case 'v':
				vflag++;
				break;

			case 'q':
				qflag++;
				break;

			default:
				fprintf(stderr, "quota: %c: unknown option\n",
					*cp);
				exit(1);
			}
		else
			break;
		argc--, argv++;
	}
	morethanone = argc > 1;
	if (argc == 0) {
		showuid(getuid());
		exit(0);
	}
	for (; argc > 0; argc--, argv++) {
		if (alldigits(*argv))
			showuid(atoi(*argv));
		else
			showname(*argv);
	}
}

showuid(uid)
	int uid;
{
	struct passwd *pwd = getpwuid(uid);

	if (pwd == NULL)
		showquotas(uid, "(no account)");
	else
		showquotas(uid, pwd->pw_name);
}

showname(name)
	char *name;
{
	struct passwd *pwd = getpwnam(name);

	if (pwd == NULL) {
		fprintf(stderr, "quota: %s: unknown user\n", name);
		return;
	}
	showquotas(pwd->pw_uid, name);
}

showquotas(uid, name)
	int uid;
	char *name;
{
	register struct fstab *fs;
	register char *msgi, *msgb;
	register enab = 1;
	dev_t fsdev;
	struct	stat statb;
	char *qfpathname;
	struct	dqblk dqblk;
	int myuid, fd;
	char iwarn[8], dwarn[8];

	myuid = getuid();
	if (uid != myuid && myuid != 0) {
		printf("quota: %s (uid %d): permission denied\n", name, uid);
		return;
	}
	done = 0;
	(void) setfsent();
	while (fs = getfsent()) {
		if (strcmp(fs->fs_vfstype, "ufs"))
			continue;
		if (!hasquota(fs, &qfpathname))
			continue;
		if (stat(fs->fs_spec, &statb) < 0)
			continue;
		msgi = msgb = (char *) 0;
/*
 * This check for the quota file being in the filesystem to which the quotas
 * belong is silly but the kernel enforces it.   When the kernel is fixed the
 * check can be removed.
*/
		fsdev = statb.st_rdev;
		if (stat(qfpathname, &statb) < 0 || statb.st_dev != fsdev)
			continue;
		if (quota(Q_GETDLIM, uid, fsdev, (caddr_t)&dqblk)) {
			fd = open(qfpathname, O_RDONLY);
			if (fd < 0)
				continue;
			(void) lseek(fd, (off_t)(uid * sizeof (dqblk)), L_SET);
			switch (read(fd, (char *)&dqblk, sizeof dqblk)) {
			case 0:			/* EOF */
				/*
				 * Convert implicit 0 quota (EOF)
				 * into an explicit one (zero'ed dqblk).
				 */
				bzero((caddr_t)&dqblk, sizeof dqblk);
				break;

			case sizeof dqblk:	/* OK */
#ifdef pdp11
				dqblk.dqb_curblocks= btodb(dqblk.dqb_curblocks);
				dqblk.dqb_bsoftlimit = btodb(dqblk.dqb_bsoftlimit);
				dqblk.dqb_bhardlimit = btodb(dqblk.dqb_bhardlimit);
#endif
				break;

			default:		/* ERROR */
				fprintf(stderr, "quota: read error in ");
				perror(qfpathname);
				(void) close(fd);
				continue;
			}
			(void) close(fd);
			if (!vflag && dqblk.dqb_isoftlimit == 0 &&
			    dqblk.dqb_bsoftlimit == 0)
				continue;
			enab = 0;
		}
		if (dqblk.dqb_ihardlimit &&
		    dqblk.dqb_curinodes >= dqblk.dqb_ihardlimit)
			msgi = "File count limit reached on %s";
		else if (enab && dqblk.dqb_iwarn == 0)
			msgi = "Out of inode warnings on %s";
		else if (dqblk.dqb_isoftlimit &&
		    dqblk.dqb_curinodes >= dqblk.dqb_isoftlimit)
			msgi = "Too many files on %s";
		if (dqblk.dqb_bhardlimit &&
		    dqblk.dqb_curblocks >= dqblk.dqb_bhardlimit)
			msgb = "Block limit reached on %s";
		else if (enab && dqblk.dqb_bwarn == 0)
			msgb = "Out of block warnings on %s";
		else if (dqblk.dqb_bsoftlimit &&
		    dqblk.dqb_curblocks >= dqblk.dqb_bsoftlimit)
			msgb = "Over disc quota on %s";
		if (dqblk.dqb_iwarn < MAX_IQ_WARN)
			(void) sprintf(iwarn, "%d", dqblk.dqb_iwarn);
		else
			iwarn[0] = '\0';
		if (dqblk.dqb_bwarn < MAX_DQ_WARN)
			(void) sprintf(dwarn, "%d", dqblk.dqb_bwarn);
		else
			dwarn[0] = '\0';
		if (qflag) {
			if (msgi != (char *)0 || msgb != (char *)0)
				heading(uid, name);
			if (msgi != (char *)0)
				xprintf(msgi, fs->fs_file);
			if (msgb != (char *)0)
				xprintf(msgb, fs->fs_file);
			continue;
		}
		if (vflag || dqblk.dqb_curblocks || dqblk.dqb_curinodes) {
			heading(uid, name);
#ifdef pdp11
			printf("%10s%8ld%c%7ld%8ld%8s%8d%c%7u%8u%8s\n"
				, fs->fs_file
				, dqblk.dqb_curblocks
				, (msgb == (char *)0) ? ' ' : '*'
				, dqblk.dqb_bsoftlimit
				, dqblk.dqb_bhardlimit
#else
			printf("%10s%8d%c%7d%8d%8s%8d%c%7d%8d%8s\n"
				, fs->fs_file
				, dbtob(dqblk.dqb_curblocks) / 1024
				, (msgb == (char *)0) ? ' ' : '*'
				, dbtob(dqblk.dqb_bsoftlimit) / 1024
				, dbtob(dqblk.dqb_bhardlimit) / 1024
#endif
				, dwarn
				, dqblk.dqb_curinodes
				, (msgi == (char *)0) ? ' ' : '*'
				, dqblk.dqb_isoftlimit
				, dqblk.dqb_ihardlimit
				, iwarn
			);
		}
	}
	(void) endfsent();
	if (!done && !qflag) {
		if (morethanone)
			(void) putchar('\n');
		xprintf("Disc quotas for %s (uid %d):", name, uid);
		xprintf("none.");
	}
	xprintf((char *)0);
}

heading(uid, name)
	int uid;
	char *name;
{

	if (done++)
		return;
	xprintf((char *)0);
	if (qflag) {
		if (!morethanone)
			return;
		xprintf("User %s (uid %d):", name, uid);
		xprintf((char *)0);
		return;
	}
	(void) putchar('\n');
	xprintf("Disc quotas for %s (uid %d):", name, uid);
	xprintf((char *)0);
	printf("%10s%8s %7s%8s%8s%8s %7s%8s%8s\n"
		, "Filsys"
		, "current"
		, "quota"
		, "limit"
		, "#warns"
		, "files"
		, "quota"
		, "limit"
		, "#warns"
	);
}

/*VARARGS1*/
xprintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6)
	char *fmt;
{
	char	buf[100];
	static int column;

	if (fmt == 0 && column || column >= 40) {
		(void) putchar('\n');
		column = 0;
	}
	if (fmt == 0)
		return;
	(void) sprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	if (column != 0 && strlen(buf) < 39)
		while (column++ < 40)
			(void) putchar(' ');
	else if (column) {
		(void) putchar('\n');
		column = 0;
	}
	printf("%s", buf);
	column += strlen(buf);
}

/*
 * Check to see if a particular quota is to be enabled.
 */
hasquota(fs, qfnamep)
	register struct fstab *fs;
	char **qfnamep;
{
	register char *opt;
	char *cp;
	static char initname, usrname[100];
	static char buf[BUFSIZ];

	if (!initname) {
		strcpy(usrname, qfname);
		initname = 1;
	}
	strcpy(buf, fs->fs_mntops);
	for (opt = strtok(buf, ","); opt; opt = strtok(NULL, ",")) {
		if (cp = index(opt, '='))
			*cp++ = '\0';
		if (strcmp(opt, usrname) == 0)
			break;
		if (strcmp(opt, FSTAB_RQ) == 0)	/* XXX compatibility */
			break;
	}
	if (!opt)
		return (0);
	if (cp) {
		*qfnamep = cp;
		return (1);
	}
	(void) sprintf(buf, "%s/%s", fs->fs_file, qfname);
	*qfnamep = buf;
	return (1);
}

alldigits(s)
	register char *s;
{
	register c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while (c = *s++);
	return (1);
}
