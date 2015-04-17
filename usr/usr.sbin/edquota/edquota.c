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

static char sccsid[] = "@(#)edquota.c	8.1.1 (2.11BSD) 1996/1/21";
#endif /* not lint */

/*
 * Disk quota editor.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/quota.h>
#include <errno.h>
#include <fstab.h>
#include <pwd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>

char *qfname = "quotas";
char tmpfil[] = "/tmp/EdP.aXXXXXX";

struct quotause {
	struct	quotause *next;
	short	flags;
	struct	dqblk dqblk;
	char	fsname[MAXPATHLEN + 1];
	char	qfname[1];	/* actually longer */
} *getprivs();
#define	FOUND	0x01

	uid_t	getentry();

main(argc, argv)
	register char **argv;
	int argc;
{
	register struct quotause *protoprivs, *curprivs;
	extern char *optarg;
	extern int optind;
	uid_t id, protoid;
	int tmpfd;
	char *protoname, ch;
	int pflag = 0;

	if (argc < 2)
		usage();
	if (getuid()) {
		fprintf(stderr, "edquota: permission denied\n");
		exit(1);
	}
	while ((ch = getopt(argc, argv, "p:")) != EOF) {
		switch(ch) {
		case 'p':
			protoname = optarg;
			pflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (pflag) {
		if ((protoid = getentry(protoname)) == -1)
			exit(1);
		protoprivs = getprivs(protoid);
		while (argc-- > 0) {
			if ((id = getentry(*argv++)) < 0)
				continue;
			putprivs(id, protoprivs);
		}
		exit(0);
	}
	tmpfd = mkstemp(tmpfil);
	fchown(tmpfd, getuid(), getgid());
	for ( ; argc > 0; argc--, argv++) {
		if ((id = getentry(*argv)) == -1)
			continue;
		curprivs = getprivs(id);
		if (writeprivs(curprivs, tmpfd, *argv) == 0)
			continue;
		if (editit(tmpfil) && readprivs(curprivs, tmpfd))
			putprivs(id, curprivs);
		freeprivs(curprivs);
	}
	close(tmpfd);
	unlink(tmpfil);
	exit(0);
}

usage()
{
	fputs("Usage: edquota [-p username] username ...\n", stderr);
	exit(1);
}

/*
 * This routine converts a name for a particular quota type to
 * an identifier. This routine must agree with the kernel routine
 * getinoquota as to the interpretation of quota types.
 */
uid_t
getentry(name)
	char *name;
{
	struct passwd *pw;

	if (alldigits(name))
		return (atoi(name));
	if (pw = getpwnam(name))
		return (pw->pw_uid);
	fprintf(stderr, "%s: no such user\n", name);
	sleep(1);
	return (-1);
}

/*
 * Collect the requested quota information.
 */
struct quotause *
getprivs(id)
	u_int id;
{
	register struct fstab *fs;
	register struct quotause *qup, *quptail;
	struct	dqblk *dq;
	struct quotause *quphead;
	struct	stat	statb;
	dev_t	fsdev;
	int qupsize, fd;
	char *qfpathname;
	static int warned = 0;
	extern int errno;

	setfsent();
	quphead = (struct quotause *)0;
	while (fs = getfsent()) {
		if (stat(fs->fs_spec, &statb) < 0)
			continue;
		if (strcmp(fs->fs_vfstype, "ufs"))
			continue;
		if (!hasquota(fs, &qfpathname))
			continue;
/*
 * See the comments in quota.c about the check below.
*/
		fsdev = statb.st_rdev;
		if (stat(qfpathname, &statb) < 0 || statb.st_dev != fsdev)
			continue;
		qupsize = sizeof(*qup) + strlen(qfpathname);
		if ((qup = (struct quotause *)malloc(qupsize)) == NULL) {
			fprintf(stderr, "edquota: out of memory\n");
			exit(2);
		}
		if (quota(Q_GETDLIM, id, fsdev, &qup->dqblk) != 0) {
	    		if (errno == EOPNOTSUPP && !warned) {
				warned++;
				fprintf(stderr, "Warning: %s\n",
				    "Quotas are not compiled into this kernel");
				sleep(3);
			}
			if ((fd = open(qfpathname, O_RDONLY)) < 0) {
				fd = open(qfpathname, O_RDWR|O_CREAT, 0640);
				if (fd < 0 && errno != ENOENT) {
					perror(qfpathname);
					free(qup);
					continue;
				}
				fprintf(stderr, "Creating quota file %s\n",
				    qfpathname);
				sleep(3);
				(void) fchown(fd, getuid(), -1);
				(void) fchmod(fd, 0640);
			}
			lseek(fd, (long)(id * sizeof(struct dqblk)), L_SET);
			switch (read(fd, &qup->dqblk, sizeof(struct dqblk))) {
			case 0:			/* EOF */
				/*
				 * Convert implicit 0 quota (EOF)
				 * into an explicit one (zero'ed dqblk)
				 */
				bzero((caddr_t)&qup->dqblk,
				    sizeof(struct dqblk));
				break;

			case sizeof(struct dqblk):	/* OK */
/*
 * We have to convert from bytes to disc blocks because the quotas file 
 * entries use bytes like the kernel does.
*/
				dq = &qup->dqblk;
				dq->dqb_curblocks = btodb(dq->dqb_curblocks);
				dq->dqb_bhardlimit = btodb(dq->dqb_bhardlimit);
				dq->dqb_bsoftlimit = btodb(dq->dqb_bsoftlimit);
				break;

			default:		/* ERROR */
				fprintf(stderr, "edquota: read error in ");
				perror(qfpathname);
				close(fd);
				free(qup);
				continue;
			}
			close(fd);
		}
		strcpy(qup->qfname, qfpathname);
		strcpy(qup->fsname, fs->fs_file);
		if (quphead == NULL)
			quphead = qup;
		else
			quptail->next = qup;
		quptail = qup;
		qup->next = 0;
	}
	endfsent();
	return (quphead);
}

/*
 * Store the requested quota information.
 */
putprivs(id, quplist)
	uid_t id;
	struct quotause *quplist;
{
	register struct quotause *qup;
	struct stat sb;
	int fd;
	register struct dqblk *dq;

	for (qup = quplist; qup; qup = qup->next) {
		if (stat(qup->qfname, &sb) < 0) {
			fprintf(stderr,"edquota: can't stat %s\n",qup->qfname);
			continue;
		}

		if (quota(Q_SETDLIM, id, sb.st_dev, &qup->dqblk) == 0)
			continue;
/*
 * Have to convert from blocks to bytes now to be compatible with the kernel
*/
		dq = &qup->dqblk;
		dq->dqb_bhardlimit = dbtob(dq->dqb_bhardlimit);
		dq->dqb_bsoftlimit = dbtob(dq->dqb_bsoftlimit);
		dq->dqb_curblocks = dbtob(dq->dqb_curblocks);

		if ((fd = open(qup->qfname, O_WRONLY)) < 0) {
			perror(qup->qfname);
		} else {
			lseek(fd, (long)id * (long)sizeof (struct dqblk), 0);
			if (write(fd, &qup->dqblk, sizeof (struct dqblk)) !=
			    sizeof (struct dqblk)) {
				fprintf(stderr, "edquota: ");
				perror(qup->qfname);
			}
			close(fd);
		}
	}
}

/*
 * Take a list of priviledges and get it edited.
 */
editit(tmpfile)
	char *tmpfile;
{
	long omask;
	pid_t pid;
	union wait stat;
	extern char *getenv();

	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
 top:
	if ((pid = vfork()) < 0) {
		extern int errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno == EAGAIN) {
			sleep(1);
			goto top;
		}
		perror("fork");
		return (0);
	}
	if (pid == 0) {
		register char *ed;

		sigsetmask(omask);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = _PATH_VI;
		execlp(ed, ed, tmpfile, 0);
		perror(ed);
		exit(1);
	}
	waitpid(pid, &stat, 0);
	sigsetmask(omask);
	if (!WIFEXITED(stat) || WEXITSTATUS(stat) != 0)
		return (0);
	return (1);
}

/*
 * Convert a quotause list to an ASCII file.
 */
writeprivs(quplist, outfd, name)
	struct quotause *quplist;
	int outfd;
	char *name;
{
	register struct quotause *qup;
	register FILE *fd;

	ftruncate(outfd, (off_t)0);
	lseek(outfd, (off_t)0, L_SET);
	if ((fd = fdopen(dup(outfd), "w")) == NULL) {
		fprintf(stderr, "edquota: ");
		perror(tmpfil);
		exit(1);
	}
	fprintf(fd, "Quotas for %s:\n", name);
	for (qup = quplist; qup; qup = qup->next) {
		fprintf(fd, "%s: %s %ld, limits (soft = %ld, hard = %ld)\n",
		    qup->fsname, "blocks in use:",
		    qup->dqblk.dqb_curblocks,
		    qup->dqblk.dqb_bsoftlimit,
		    qup->dqblk.dqb_bhardlimit);
		fprintf(fd, "%s %u, limits (soft = %u, hard = %u)\n",
		    "\tinodes in use:", qup->dqblk.dqb_curinodes,
		    qup->dqblk.dqb_isoftlimit, qup->dqblk.dqb_ihardlimit);
	}
	fclose(fd);
	return (1);
}

/*
 * Merge changes to an ASCII file into a quotause list.
 */
readprivs(quplist, infd)
	struct quotause *quplist;
	int infd;
{
	register struct quotause *qup;
	FILE *fd;
	int cnt;
	register char *cp;
	struct dqblk dqblk;
	char *fsp, line1[BUFSIZ], line2[BUFSIZ];

	lseek(infd, (off_t)0, L_SET);
	fd = fdopen(dup(infd), "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't re-read temp file!!\n");
		return (0);
	}
	/*
	 * Discard title line, then read pairs of lines to process.
	 */
	(void) fgets(line1, sizeof (line1), fd);
	while (fgets(line1, sizeof (line1), fd) != NULL &&
	       fgets(line2, sizeof (line2), fd) != NULL) {
		if ((fsp = strtok(line1, " \t:")) == NULL) {
			fprintf(stderr, "%s: bad format\n", line1);
			return (0);
		}
		if ((cp = strtok((char *)0, "\n")) == NULL) {
			fprintf(stderr, "%s: %s: bad format\n", fsp,
			    &fsp[strlen(fsp) + 1]);
			return (0);
		}
		cnt = sscanf(cp,
		    " blocks in use: %ld, limits (soft = %ld, hard = %ld)",
		    &dqblk.dqb_curblocks, &dqblk.dqb_bsoftlimit,
		    &dqblk.dqb_bhardlimit);
		if (cnt != 3) {
			fprintf(stderr, "%s:%s: bad format\n", fsp, cp);
			return (0);
		}
		if ((cp = strtok(line2, "\n")) == NULL) {
			fprintf(stderr, "%s: %s: bad format\n", fsp, line2);
			return (0);
		}
		cnt = sscanf(cp,
		    "\tinodes in use: %u, limits (soft = %u, hard = %u)",
		    &dqblk.dqb_curinodes, &dqblk.dqb_isoftlimit,
		    &dqblk.dqb_ihardlimit);
		if (cnt != 3) {
			fprintf(stderr, "%s: %s: bad format\n", fsp, line2);
			return (0);
		}
		for (qup = quplist; qup; qup = qup->next) {
			if (strcmp(fsp, qup->fsname))
				continue;
			qup->dqblk.dqb_bsoftlimit = dqblk.dqb_bsoftlimit;
			qup->dqblk.dqb_bhardlimit = dqblk.dqb_bhardlimit;
			qup->dqblk.dqb_isoftlimit = dqblk.dqb_isoftlimit;
			qup->dqblk.dqb_ihardlimit = dqblk.dqb_ihardlimit;
			qup->flags |= FOUND;
			if (dqblk.dqb_curblocks == qup->dqblk.dqb_curblocks &&
			    dqblk.dqb_curinodes == qup->dqblk.dqb_curinodes)
				break;
			fprintf(stderr,
			    "%s: cannot change current allocation\n", fsp);
			break;
		}
	}
	fclose(fd);
	/*
	 * Disable quotas for any filesystems that have not been found.
	 */
	for (qup = quplist; qup; qup = qup->next) {
		if (qup->flags & FOUND) {
			qup->flags &= ~FOUND;
			continue;
		}
		qup->dqblk.dqb_bsoftlimit = 0;
		qup->dqblk.dqb_bhardlimit = 0;
		qup->dqblk.dqb_isoftlimit = 0;
		qup->dqblk.dqb_ihardlimit = 0;
	}
	return (1);
}

/*
 * Free a list of quotause structures.
 */
freeprivs(quplist)
	struct quotause *quplist;
{
	register struct quotause *qup, *nextqup;

	for (qup = quplist; qup; qup = nextqup) {
		nextqup = qup->next;
		free(qup);
	}
}

/*
 * Check whether a string is completely composed of digits.
 */
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
