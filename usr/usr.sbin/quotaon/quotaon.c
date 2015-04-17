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

static char sccsid[] = "@(#)quotaon.c	8.1.1 (2.11BSD) 1996/1/21";
#endif /* not lint */

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <stdio.h>
#include <fstab.h>
#include <string.h>
#include <stdlib.h>

char *qfname = "quotas";

int	aflag;		/* all file systems */
int	vflag;		/* verbose */

main(argc, argv)
	int argc;
	char **argv;
{
	register struct fstab *fs;
	char ch, *qfnp, *whoami;
	long done = 0;
	int i, argnum, offmode = 0, errs = 0;

	whoami = rindex(*argv, '/') + 1;
	if (whoami == (char *)1)
		whoami = *argv;
	if (strcmp(whoami, "quotaoff") == 0)
		offmode++;
	else if (strcmp(whoami, "quotaon") != 0) {
		fprintf(stderr, "Name must be quotaon or quotaoff not %s\n",
			whoami);
		exit(1);
	}
	while ((ch = getopt(argc, argv, "avug")) != EOF) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage(whoami);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc <= 0 && !aflag)
		usage(whoami);
	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (strcmp(fs->fs_type, FSTAB_RQ) == 0)  /* XXX compatibility */
			fs->fs_type = FSTAB_RW;
		if (strcmp(fs->fs_vfstype, "ufs") ||
		    strcmp(fs->fs_type, FSTAB_RW))
			continue;
		if (aflag) {
			if (hasquota(fs, &qfnp))
				errs += quotaonoff(fs, offmode, qfnp);
			continue;
		}
		if ((argnum = oneof(fs->fs_file, argv, argc)) >= 0 ||
		    (argnum = oneof(fs->fs_spec, argv, argc)) >= 0) {
			done |= 1 << argnum;
			if (hasquota(fs, &qfnp))
				errs += quotaonoff(fs, offmode, qfnp);
		}
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			fprintf(stderr, "%s not found in fstab\n",
				argv[i]);
	exit(errs);
}

usage(whoami)
	char *whoami;
{

	fprintf(stderr, "Usage:\n\t%s [-v] -a\n", whoami);
	fprintf(stderr, "\t%s [-v] filesys ...\n", whoami);
	exit(1);
}

quotaonoff(fs, offmode, qfpathname)
	register struct fstab *fs;
	int offmode;
	char *qfpathname;
{

	if (strcmp(fs->fs_file, "/") && readonly(fs))
		return (1);
	if (offmode) {
		if (setquota(fs->fs_spec, (char *)NULL) < 0) {
			fprintf(stderr, "quotaoff: ");
			perror(fs->fs_spec);
			return (1);
		}
		if (vflag)
			printf("%s: quotas turned off\n", fs->fs_file);
		return (0);
	}
	if (setquota(fs->fs_spec, qfpathname) < 0) {
		fprintf(stderr, "quotaon: using %s on", qfpathname);
		perror(fs->fs_file);
		return (1);
	}
	if (vflag)
		printf("%s: quotas turned on\n", fs->fs_file);
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
 * Verify file system is mounted and not readonly.
 */
readonly(fs)
	register struct fstab *fs;
{
	struct statfs fsbuf;

	if (statfs(fs->fs_file, &fsbuf) < 0 ||
	    strcmp(fsbuf.f_mntonname, fs->fs_file) ||
	    strcmp(fsbuf.f_mntfromname, fs->fs_spec)) {
		printf("%s: not mounted\n", fs->fs_file);
		return (1);
	}
	if (fsbuf.f_flags & MNT_RDONLY) {
		printf("%s: mounted read-only\n", fs->fs_file);
		return (1);
	}
	return (0);
}
