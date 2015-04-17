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

static char sccsid[] = "@(#)quotacheck.c	8.3.1 (2.11BSD) 1996/1/23";
#endif /* not lint */

/*
 * Fix up / report on disk quotas & usage
 */
#include <sys/param.h>
#include <sys/stat.h>

#include <sys/inode.h>
#include <sys/quota.h>
#include <sys/fs.h>

#include <fcntl.h>
#include <fstab.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *qfname = QUOTAFILENAME;
char *quotagroup = QUOTAGROUP;

union {
	struct	fs	sblk;
	char	dummy[MAXBSIZE];
} un;
#define	sblock	un.sblk
ino_t maxino;

struct quotaname {
	char	flags;
	char	usrqfname[MAXPATHLEN + 1];
};
#define	HASUSR	1

struct fileusage {
	struct	fileusage *fu_next;
	ino_t	fu_curinodes;
	u_long	fu_curblocks;
	uid_t	fu_id;
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 256	/* must be power of two */
struct fileusage *fuhead[FUHASH];

int	aflag;			/* all file systems */
int	vflag;			/* verbose */
int	fi;			/* open disk file descriptor */
uid_t	highid;			/* highest addid()'ed identifier */

struct fileusage *addid();
char	*blockcheck();
void	bread();
int	chkquota();
void	freeinodebuf();
struct	dinode *getnextinode();
gid_t	 getquotagid();
int	 hasquota();
struct fileusage *lookup();
void	*needchk();
int	 oneof();
void	 resetinodebuf();
int	 update();
void	 usage();

int
main(argc, argv)
	int argc;
	char *argv[];
{
	register struct fstab *fs;
	register struct passwd *pw;
	struct quotaname *auxdata;
	int i, argnum, maxrun, errs, ch;
	long done = 0;
	char *name;

	errs = maxrun = 0;
	while ((ch = getopt(argc, argv, "avl:")) != EOF) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'l':
			maxrun = atoi(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if ((argc == 0 && !aflag) || (argc > 0 && aflag))
		usage();

	setpwent();
	while ((pw = getpwent()) != 0)
		(void) addid(pw->pw_uid, pw->pw_name);
	endpwent();

	if (aflag)
		exit(checkfstab(1, maxrun, needchk, chkquota));
	if (setfsent() == 0)
		err(1, "can't open %s", FSTAB);
	while ((fs = getfsent()) != NULL) {
		if (((argnum = oneof(fs->fs_file, argv, argc)) >= 0 ||
		    (argnum = oneof(fs->fs_spec, argv, argc)) >= 0) &&
		    (auxdata = (struct quotaname *)needchk(fs)) &&
		    (name = blockcheck(fs->fs_spec))) {
			done |= 1 << argnum;
			errs += chkquota(name, fs->fs_file, auxdata);
		}
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			warn("%s not found in %s", argv[i], FSTAB);
	exit(errs);
}

void
usage()
{
	(void)err(1, "usage:\t%s\n\t%s\n",
		"quotacheck -a [-v] [-l num]",
		"quotacheck [-v] filesys ...");
/* NOTREACHED */
}

void *
needchk(fs)
	register struct fstab *fs;
{
	register struct quotaname *qnp;
	char *qfnp;

	if (strcmp(fs->fs_vfstype, "ufs") ||
	    (strcmp(fs->fs_type, FSTAB_RW) && strcmp(fs->fs_type, FSTAB_RQ)))
		return ((void *)NULL);
	if ((qnp = (struct quotaname *)malloc(sizeof(*qnp))) == NULL)
		nomem();
	qnp->flags = 0;
	if (hasquota(fs, &qfnp)) {
		strcpy(qnp->usrqfname, qfnp);
		qnp->flags |= HASUSR;
	}
	if (qnp->flags)
		return ((void *)qnp);
	free(qnp);
	return ((void *)NULL);
}

/*
 * Scan the specified filesystem to check quota(s) present on it.
 */
int
chkquota(fsname, mntpt, qnp)
	char *fsname, *mntpt;
	register struct quotaname *qnp;
{
	register struct fileusage *fup;
	register struct dinode *dp;
	int mode, errs = 0;
	ino_t ino;

	if ((fi = open(fsname, O_RDONLY, 0)) < 0) {
		perror(fsname);
		return (1);
	}
	if (vflag)
		(void)printf("*** Checking quotas for %s (%s)\n", 
			fsname, mntpt);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	maxino = (sblock.fs_isize - 2) * INOPB;
	resetinodebuf();
	for (ino = ROOTINO; ino < maxino; ino++) {
		if ((dp = getnextinode(ino)) == NULL)
			continue;
		if ((mode = dp->di_mode & IFMT) == 0)
			continue;
		if (qnp->flags & HASUSR) {
			fup = addid(dp->di_uid, (char *)0);
			fup->fu_curinodes++;
			if (mode == IFREG || mode == IFDIR || mode == IFLNK)
				fup->fu_curblocks += dp->di_size;
		}
	}
	freeinodebuf();
	if (qnp->flags & HASUSR)
		errs += update(fsname, mntpt, qnp->usrqfname);
	close(fi);
	return (errs);
}

/*
 * Update a specified quota file.
 */
int
update(fsdev, fsname, quotafile)
	char *fsdev, *fsname, *quotafile;
{
	register struct fileusage *fup;
	register FILE *qfi, *qfo;
	uid_t id, lastid;
	struct dqblk dqbuf;
	struct stat  statb;
	struct dqusage dqu;
	dev_t  quotadev;
	static int warned = 0;
	static struct dqblk zerodqbuf;
	static struct fileusage zerofileusage;

	if ((qfo = fopen(quotafile, "r+")) == NULL) {
		if (errno == ENOENT)
			qfo = fopen(quotafile, "w+");
		if (qfo) {
			(void) warn("creating quota file %s", quotafile);
			(void) fchown(fileno(qfo), getuid(), getquotagid());
			(void) fchmod(fileno(qfo), 0640);
		} else {
			(void) warn("%s: %s", quotafile, strerror(errno));
			return (1);
		}
	}
	if ((qfi = fopen(quotafile, "r")) == NULL) {
		(void) warn("%s: %s", quotafile, strerror(errno));
		(void) fclose(qfo);
		return (1);
	}
/*
 * We check that the quota file resides in the filesystem being checked.  This
 * restriction is imposed by the kernel for now.
*/
	fstat(fileno(qfi), &statb);
	quotadev = statb.st_dev;
	if (stat(unrawname(fsdev), &statb) < 0) {
		warn("Can't stat %s", fsname);
		fclose(qfi);
		fclose(qfo);
		return(1);
	}
	if (quotadev != statb.st_rdev)  {
		warn("%s dev (0x%x) mismatch %s dev (0x%x)", quotafile, 
			quotadev, fsname, statb.st_rdev);
		fclose(qfi);
		fclose(qfo);
		return(1);
	}
	if (quota(Q_SYNC, 0, quotadev, 0) < 0 &&
	    errno == EOPNOTSUPP && !warned && vflag) {
		warned++;
		(void)printf("*** Warning: %s\n",
		    "Quotas are not compiled into this kernel");
	}
	for (lastid = highid, id = 0; id <= lastid; id++) {
		if (fread((char *)&dqbuf, sizeof(struct dqblk), 1, qfi) == 0)
			dqbuf = zerodqbuf;
		if ((fup = lookup(id)) == 0)
			fup = &zerofileusage;
		if (dqbuf.dqb_curinodes == fup->fu_curinodes &&
		    dqbuf.dqb_curblocks == fup->fu_curblocks) {
			fup->fu_curinodes = 0;
			fup->fu_curblocks = 0;
			fseek(qfo, (long)sizeof(struct dqblk), 1);
			continue;
		}
		if (vflag) {
			if (aflag)
				printf("%s: ", fsname);
			printf("%-8s fixed:", fup->fu_name);
			if (dqbuf.dqb_curinodes != fup->fu_curinodes)
				(void)printf("\tinodes %u -> %u",
					dqbuf.dqb_curinodes, fup->fu_curinodes);
			if (dqbuf.dqb_curblocks != fup->fu_curblocks)
				(void)printf("\tbytes %ld -> %ld",
					dqbuf.dqb_curblocks, fup->fu_curblocks);
			(void)printf("\n");
		}
		dqbuf.dqb_curinodes = fup->fu_curinodes;
		dqbuf.dqb_curblocks = fup->fu_curblocks;
		fwrite((char *)&dqbuf, sizeof(struct dqblk), 1, qfo);
/*
 * Need to convert from bytes to blocks for system call interface.
 */
		dqu.du_curblocks = btodb(fup->fu_curblocks);
		dqu.du_curinodes = fup->fu_curinodes;
		(void) quota(Q_SETDUSE, id, quotadev, &dqu);
		fup->fu_curinodes = 0;
		fup->fu_curblocks = 0;
	}
	fclose(qfi);
	fflush(qfo);
	ftruncate(fileno(qfo), (off_t)(highid + 1) * sizeof(struct dqblk));
	fclose(qfo);
	return (0);
}

/*
 * Check to see if target appears in list of size cnt.
 */
int
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
 * Determine the group identifier for quota files.
 */
gid_t
getquotagid()
{
	struct group *gr;

	if (gr = getgrnam(quotagroup))
		return (gr->gr_gid);
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
	uid_t id;
{
	register struct fileusage *fup;

	for (fup = fuhead[id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return (fup);
	return (NULL);
}

/*
 * Add a new file usage id if it does not already exist.
 */
struct fileusage *
addid(id, name)
	uid_t id;
	char *name;
{
	register struct fileusage *fup, **fhp;
	int len;

	if (fup = lookup(id))
		return (fup);
	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = (struct fileusage *)calloc(1, sizeof(*fup) + len)) == NULL)
		err("%s", strerror(errno));
	fhp = &fuhead[id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid)
		highid = id;
	if (name)
		bcopy(name, fup->fu_name, len + 1);
	else
		(void)sprintf(fup->fu_name, "%u", id);
	return (fup);
}

/*
 * Special purpose version of ginode used to optimize pass
 * over all the inodes in numerical order.
 */
ino_t nextino;
long lastinum;
int inobufsize;
struct dinode *inodebuf;
#define	INOBUFSIZE	128	/* number of inodes to read at a time */

struct dinode *
getnextinode(inumber)
	ino_t inumber;
{
	daddr_t dblk;
	static struct dinode *dp;

	if (inumber != nextino++ || inumber >= maxino)
		err(1, "bad inode number %d to nextinode", inumber);
	if (inumber > lastinum) {
		dblk = itod(inumber);
		lastinum += INOBUFSIZE;
		bread(dblk, (char *)inodebuf, inobufsize);
		dp = inodebuf;
	}
	return (dp++);
}

/*
 * Prepare to scan a set of inodes.
 */
void
resetinodebuf()
{

	nextino = 1;
	lastinum = 0;
	inobufsize = INOBUFSIZE * sizeof (struct dinode);
	if (inodebuf == NULL && 
		(inodebuf = (struct dinode *)malloc(inobufsize)) == NULL)
		nomem();
	while (nextino < ROOTINO)
		getnextinode(nextino);
}

/*
 * Free up data structures used to scan inodes.
 */
void
freeinodebuf()
{

	if (inodebuf != NULL)
		free(inodebuf);
	inodebuf = NULL;
}

/*
 * Read specified disk blocks.
 */
void
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{

	if (lseek(fi, (off_t)bno * DEV_BSIZE, 0) == -1 ||
	    read(fi, buf, cnt) != cnt)
		err(1, "block %ld", bno);
}
