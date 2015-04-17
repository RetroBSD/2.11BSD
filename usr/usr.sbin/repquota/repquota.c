/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if	!defined(lint) && defined(DOSCCS)
static char copyright[] =
"@(#) Copyright (c) 1980, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";

static char sccsid[] = "@(#)repquota.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * Quota report
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/quota.h>
#include <fstab.h>
#include <pwd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

char *qfname = QUOTAFILENAME;

struct fileusage {
	struct	fileusage *fu_next;
	struct	dqblk fu_dqblk;
	u_int	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 256	/* must be power of two */
struct fileusage *fuhead[FUHASH];
struct fileusage *lookup();
struct fileusage *addid();
u_int highid;		/* highest addid()'ed identifier per type */

int	vflag;			/* verbose */
int	aflag;			/* all file systems */

main(argc, argv)
	int argc;
	char **argv;
{
	register struct fstab *fs;
	register struct passwd *pw;
	int errs = 0;
	int i, argnum;
	long done = 0;
	extern char *optarg;
	extern int optind;
	char ch, *qfnp;

	while ((ch = getopt(argc, argv, "aguv")) != EOF) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0 && !aflag)
		usage();

	setpwent();
	while ((pw = getpwent()) != 0)
		(void) addid(pw->pw_uid, pw->pw_name);
	endpwent();

	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_vfstype, "ufs"))
			continue;
		if (aflag) {
			if (hasquota(fs, &qfnp))
				errs += repquota(fs, qfnp);
			continue;
		}
		if ((argnum = oneof(fs->fs_file, argv, argc)) >= 0 ||
		    (argnum = oneof(fs->fs_spec, argv, argc)) >= 0) {
			done |= 1 << argnum;
			if (hasquota(fs, &qfnp))
				errs += repquota(fs, qfnp);
		}
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			fprintf(stderr, "%s not found in fstab\n", argv[i]);
	exit(errs);
}

usage()
{
	fprintf(stderr, "Usage:\n\t%s\n\t%s\n",
		"repquota [-v] [-g] [-u] -a",
		"repquota [-v] [-g] [-u] filesys ...");
	exit(1);
}

repquota(fs, qfpathname)
	register struct fstab *fs;
	char *qfpathname;
{
	register struct fileusage *fup;
	FILE *qf;
	u_int id;
	struct dqblk dqbuf;
	struct stat statb;
	static struct dqblk zerodqblk;
	static int warned = 0;
	static int multiple = 0;
	extern int errno;

	if ((qf = fopen(qfpathname, "r")) == NULL) {
		perror(qfpathname);
		return (1);
	}
	if (fstat(fileno(qf), &statb) < 0) {
		perror(qfpathname);
		fclose(qf);
		return(1);
	}
	if (quota(Q_SYNC, 0, statb.st_dev, 0) < 0 &&
	    errno == EOPNOTSUPP && !warned && vflag) {
		warned++;
		fprintf(stdout,
		    "*** Warning: Quotas are not compiled into this kernel\n");
	}
	if (multiple++)
		printf("\n");
	if (vflag)
		fprintf(stdout, "*** Report for quotas on %s (%s)\n",
		    fs->fs_file, fs->fs_spec);
	for (id = 0; ; id++) {
		fread(&dqbuf, sizeof(struct dqblk), 1, qf);
		if (feof(qf))
			break;
		if (dqbuf.dqb_curinodes == 0 && dqbuf.dqb_curblocks == 0)
			continue;
		if ((fup = lookup(id)) == 0)
			fup = addid(id, (char *)0);
		fup->fu_dqblk = dqbuf;
	}
	fclose(qf);
	printf("                        Block limits               File limits\n");
	printf("User            used    soft    hard  warn    used  soft  hard  warn\n");
	for (id = 0; id <= highid; id++) {
		fup = lookup(id);
		if (fup == 0)
			continue;
		if (fup->fu_dqblk.dqb_curinodes == 0 &&
		    fup->fu_dqblk.dqb_curblocks == 0)
			continue;
		printf("%-10s", fup->fu_name);
		printf("%c%c%8ld%8ld%8ld%5u",
			fup->fu_dqblk.dqb_bsoftlimit && 
			    fup->fu_dqblk.dqb_curblocks >= 
			    fup->fu_dqblk.dqb_bsoftlimit ? '+' : '-',
			fup->fu_dqblk.dqb_isoftlimit &&
			    fup->fu_dqblk.dqb_curinodes >=
			    fup->fu_dqblk.dqb_isoftlimit ? '+' : '-',
			fup->fu_dqblk.dqb_curblocks / 1024,
			fup->fu_dqblk.dqb_bsoftlimit / 1024,
			fup->fu_dqblk.dqb_bhardlimit / 1024,
			fup->fu_dqblk.dqb_bwarn);

		printf("  %6u%6u%6u%6u\n",
			fup->fu_dqblk.dqb_curinodes,
			fup->fu_dqblk.dqb_isoftlimit,
			fup->fu_dqblk.dqb_ihardlimit,
			fup->fu_dqblk.dqb_iwarn);
		fup->fu_dqblk = zerodqblk;
	}
	return (0);
}

/*
 * Check to see if target appears in list of size cnt.
 */
oneof(target, list, cnt)
	register char *target, *list[];
	int cnt;
{
	register int i;

	for (i = 0; i < cnt; i++)
		if (strcmp(target, list[i]) == 0)
			return (i);
	return (-1);
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

/*
 * Routines to manage the file usage table.
 *
 * Lookup an id.
 */
struct fileusage *
lookup(id)
	u_int id;
{
	register struct fileusage *fup;

	for (fup = fuhead[id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return (fup);
	return ((struct fileusage *)0);
}

/*
 * Add a new file usage id if it does not already exist.
 */
struct fileusage *
addid(id, name)
	u_short id;
	char *name;
{
	struct fileusage *fup, **fhp;
	int len;
	extern char *calloc();

	if (fup = lookup(id))
		return (fup);
	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = (struct fileusage *)calloc(1, sizeof(*fup) + len)) == NULL) {
		fprintf(stderr, "out of memory for fileusage structures\n");
		exit(1);
	}
	fhp = &fuhead[id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid)
		highid = id;
	if (name) {
		bcopy(name, fup->fu_name, len + 1);
	} else {
		sprintf(fup->fu_name, "%u", id);
	}
	return (fup);
}
