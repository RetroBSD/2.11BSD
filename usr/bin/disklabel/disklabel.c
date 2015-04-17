/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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
"@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";

static char sccsid[] = "@(#)disklabel.c	8.1.3 (2.11BSD) 1999/10/25";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#endif

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#define DKTYPENAMES
#include <sys/disklabel.h>
#include <sys/fs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "pathnames.h"

/*
 * Disklabel: read and write disklabels.
 * The label is usually placed on one of the first sectors of the disk.
 * Many machines also place a bootstrap in the same area,
 * in which case the label is embedded in the bootstrap.
 * The bootstrap source must leave space at the proper offset
 * for the label on such machines.
 */

/*
 * 2.11BSD has to do some things differently than other systems.  BBSIZE
 * may be 1024 bytes but the disk boot roms only read the 512 byte sector 0.
 * The boot and label areas each occupy 1/2 of the 1kb 'filesystem block' 
 * which precedes the superblock.  The bootblock is sector 0 and the label block
 * is sector 1.  Each is a disjoint and independent area.  Thus the logic in
 * this program which treated the label area as a subset or offset of the
 * boot area had to change.
 *
 * The 'a' partition is used to access the raw device - as did the 'tahoe'.
*/

#define RAWPARTITION	'a'

#define	DEFEDITOR	_PATH_VI
#define	streq(a,b)	(strcmp(a,b) == 0)

char	*dkname;
char	*specname;
char	tmpfil[] = _PATH_TMP;

char	namebuf[256], *np = namebuf;
struct	disklabel lab;
struct	disklabel *readlabel(), *makebootarea();
char	bootarea[BBSIZE];	/* 512 for bootblock, 512 for label */

int	installboot;	/* non-zero if we should install a boot program */
char	*xxboot;	/* primary boot */
char	boot0[MAXPATHLEN];

enum	{
	UNSPEC, EDIT, NOWRITE, READ, RESTORE, WRITE, WRITEABLE, WRITEBOOT
} op = UNSPEC;

int	rflag;

#ifdef DEBUG
int	debug;
#define OPTIONS	"BNRWb:derw"
#else
#define OPTIONS	"BNRWb:erw"
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	register struct disklabel *lp;
	FILE *t;
	int ch, f, flag, error = 0;
	char *name = 0;

	while ((ch = getopt(argc, argv, OPTIONS)) != EOF)
		switch (ch) {
			case 'B':
				++installboot;
				break;
			case 'b':
				xxboot = optarg;
				break;
			case 'N':
				if (op != UNSPEC)
					usage();
				op = NOWRITE;
				break;
			case 'R':
				if (op != UNSPEC)
					usage();
				op = RESTORE;
				break;
			case 'W':
				if (op != UNSPEC)
					usage();
				op = WRITEABLE;
				break;
			case 'e':
				if (op != UNSPEC)
					usage();
				op = EDIT;
				break;
			case 'r':
				++rflag;
				break;
			case 'w':
				if (op != UNSPEC)
					usage();
				op = WRITE;
				break;
#ifdef DEBUG
			case 'd':
				debug++;
				break;
#endif
			case '?':
			default:
				usage();
		}
	argc -= optind;
	argv += optind;
	if (installboot) {
		rflag++;
		if (op == UNSPEC)
			op = WRITEBOOT;
	} else {
		if (op == UNSPEC)
			op = READ;
		xxboot = 0;
	}

	if (argc < 1)
		usage();

	dkname = argv[0];
	if (dkname[0] != '/') {
		(void)sprintf(np, "%s/r%s%c", _PATH_DEV, dkname, RAWPARTITION);
		specname = np;
		np += strlen(specname) + 1;
	} else
		specname = dkname;
	f = open(specname, op == READ ? O_RDONLY : O_RDWR);
	if (f < 0 && errno == ENOENT && dkname[0] != '/') {
		(void)sprintf(specname, "%s/r%s", _PATH_DEV, dkname);
		np = namebuf + strlen(specname) + 1;
		f = open(specname, op == READ ? O_RDONLY : O_RDWR);
	}
	if (f < 0)
		Perror(specname);

	switch(op) {

	case EDIT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		error = edit(lp, f);
		break;

	case NOWRITE:
		flag = 0;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;

	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		display(stdout, lp);
		error = checklabel(lp);
		break;

	case RESTORE:
		if (installboot && argc == 3) {
			makelabel(argv[2], 0, &lab);
			argc--;
		}
		if (argc != 2)
			usage();
		lp = makebootarea(bootarea, &lab, f);
		if (!(t = fopen(argv[1], "r")))
			Perror(argv[1]);
		if (getasciilabel(t, lp))
			error = writelabel(f, bootarea, lp);
		break;

	case WRITE:
		if (argc == 3) {
			name = argv[2];
			argc--;
		}
		if (argc != 2)
			usage();
		makelabel(argv[1], name, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = lab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;

	case WRITEABLE:
		flag = 1;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;

	case WRITEBOOT:
	{
		struct disklabel tlab;

		lp = readlabel(f);
		tlab = *lp;
		if (argc == 2)
			makelabel(argv[1], 0, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = tlab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;
	}
	}
	exit(error);
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
makelabel(type, name, lp)
	char *type, *name;
	register struct disklabel *lp;
{
	register struct disklabel *dp;
	char *strcpy();

	dp = getdiskbyname(type);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type\n", type);
		exit(1);
	}
	*lp = *dp;

	/*
	 * Set bootstrap name.
	 * 1. If set from command line, use it,
	 * 2. otherwise, check if disktab specifies it (b0),
	 * 3. otherwise, makebootarea() will choose one based on the name
	 *    of the disk special file. E.g. /dev/ra0 -> rauboot
	 */
	if (!xxboot && lp->d_boot0) {
		if (*lp->d_boot0 != '/')
			(void)sprintf(boot0, "%s/%s",
				      _PATH_BOOTDIR, lp->d_boot0);
		else
			(void)strcpy(boot0, lp->d_boot0);
		xxboot = boot0;
	}
	/* d_packname is union d_boot0, so zero */
	bzero(lp->d_packname, sizeof(lp->d_packname));
	if (name)
		(void)strncpy(lp->d_packname, name, sizeof(lp->d_packname));
}

writelabel(f, boot, lp)
	int f;
	char *boot;
	register struct disklabel *lp;
{
	register int i;
	int flag;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (rflag) {
		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.  Normally EFAULT would not be here - but the ioctl
		 * maximum data length changed and old kernels would generate
		 * an error prematurely in the ioctl dispatch.
		 */
		if (ioctl(f, DIOCSDINFO, lp) < 0 &&
		    errno != EFAULT && errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		(void)lseek(f, (off_t)0, L_SET);
		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		flag = 1;
		if (ioctl(f, DIOCWLABEL, &flag) < 0)
			perror("ioctl DIOCWLABEL");
/*
 * Write the boot block sector (512 bytes) followed immediately by the
 * label sector (512 bytes) = BBSIZE (1kb) total.
*/
		if (write(f, boot, BBSIZE) != BBSIZE) {
			perror("write");
			return (1);
		}
		flag = 0;
		(void) ioctl(f, DIOCWLABEL, &flag);
	} else if (ioctl(f, DIOCWDINFO, lp) < 0) {
		l_perror("ioctl DIOCWDINFO");
		return (1);
	}
	return (0);
}

l_perror(s)
	char *s;
{
	int saverrno = errno;

	fprintf(stderr, "disklabel: %s: ", s);

	switch (saverrno) {

	case ESRCH:
		fprintf(stderr, "No disk label on disk;\n");
		fprintf(stderr,
		    "use \"disklabel -r\" to install initial label\n");
		break;

	case EINVAL:
		fprintf(stderr, "Label magic number or checksum is wrong!\n");
		fprintf(stderr, "(disklabel or kernel is out of date?)\n");
		break;

	case EBUSY:
		fprintf(stderr, "Open partition would move or shrink\n");
		break;

	case EXDEV:
		fprintf(stderr,
	"Labeled partition or 'a' partition must start at beginning of disk\n");
		break;

	default:
		errno = saverrno;
		perror((char *)NULL);
		break;
	}
}

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 *
 * We use the search logic for the label even though the label will be
 * in the second half of the bootarea.  If there ever is a valid label
 * residing in the boot sector things are going to get weird.
 */
struct disklabel *
readlabel(f)
	int f;
{
	register struct disklabel *lp;
register struct disklabel *xx = (struct disklabel *)(bootarea + BBSIZE - sizeof (struct disklabel));

	if (rflag) {
		if (lseek(f, (daddr_t)0, L_SET) < 0)
			Perror(specname);
		if (read(f, bootarea, BBSIZE) < BBSIZE)
			Perror(specname);
		for (lp = (struct disklabel *)bootarea;
		    lp <= xx;
		    lp = (struct disklabel *)((char *)lp + 16))
			if (lp->d_magic == DISKMAGIC &&
			    lp->d_magic2 == DISKMAGIC)
				break;
		if (lp > xx ||
		    lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
		    dkcksum(lp) != 0) {
			fprintf(stderr,
	"Bad pack magic number (label is damaged, or pack is unlabeled)\n");
			exit (1);
		}
	} else {
		lp = &lab;
		if (ioctl(f, DIOCGDINFO, lp) < 0)
			Perror("ioctl DIOCGDINFO");
	}
	return (lp);
}

/*
 * Construct a bootarea (d_bbsize bytes) in the specified buffer ``boot''
 * Returns a pointer to the disklabel portion of the bootarea.
 */
struct disklabel *
makebootarea(boot, dp, f)
	char *boot;
	register struct disklabel *dp;
	int f;
{
	struct disklabel *lp;
	register char *p;
	int b;
	char *dkbasename;
	struct stat sb;

	/* XXX */
	/*
	 * sectors are 512 bytes, filesystem blocks are DEV_BSIZE (1024) bytes.
	 * We override the sector size so that things like the RL02 work 
	 * right.  The RL drives use a hardware sector size of 256 bytes
	 * which would wreck havoc in the calculation below.  Actually the
	 * disktab file should be "fixed" - all entries should omit 'se' and
	 * use the default of 512.
	*/
	dp->d_secsize = 512;
	dp->d_bbsize = 512;
	lp = (struct disklabel *)
		(boot + (LABELSECTOR * dp->d_secsize) + LABELOFFSET);
	bzero((char *)lp, sizeof *lp);

	/*
	 * If we are not installing a boot program but we are installing a
	 * label on disk then we must read the current bootarea so we don't
	 * clobber the existing boot.
	 */
	if (!installboot) {
		if (rflag) {
			if (lseek(f, (daddr_t)0, L_SET) < 0)
				Perror(specname);
			if (read(f, boot, BBSIZE) < BBSIZE)
				Perror(specname);
			bzero((char *)lp, sizeof *lp);
		}
		return (lp);
	}
	/*
	 * We are installing a boot program.  Determine the name(s) and
	 * read them into the appropriate places in the boot area.
	 */
	if (!xxboot) {
		dkbasename = np;
		if ((p = rindex(dkname, '/')) == NULL)
			p = dkname;
		else
			p++;
		while (*p && !isdigit(*p))
			*np++ = *p++;
		*np++ = '\0';

		(void)sprintf(np, "%s/%suboot",
			      _PATH_BOOTDIR, dkbasename);
		if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
			dkbasename++;
		xxboot = np;
		(void)sprintf(xxboot, "%s/%suboot",
			      _PATH_BOOTDIR, dkbasename);
		np += strlen(xxboot) + 1;
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr, "bootstrap: xxboot = %s\n", xxboot);
#endif

	/*
	 * Rule:
	 * 1. One-piece bootstrap up to 512 bytes is all that is supported.
	 */
	b = open(xxboot, O_RDONLY);
	if (b < 0)
		Perror(xxboot);
	if (read(b, boot, 512) < 0)
		Perror(xxboot);
	(void)fstat(b, &sb);
	if (sb.st_size > 512)
		Warning("boot > 512 bytes - filesystem likely not bootable");
	(void)close(b);
	return (lp);
}

display(f, lp)
	FILE *f;
	register struct disklabel *lp;
{
	register int i, j;
	register struct partition *pp;

	fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		fprintf(f, "type: %d\n", lp->d_type);
	fprintf(f, "disk: %.*s\n", sizeof(lp->d_typename), lp->d_typename);
	fprintf(f, "label: %.*s\n", sizeof(lp->d_packname), lp->d_packname);
	fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		fprintf(f, " removeable");
	if (lp->d_flags & D_ECC)
		fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		fprintf(f, " badsect");
	fprintf(f, "\n");
	fprintf(f, "bytes/sector: %d\n", lp->d_secsize);
	fprintf(f, "sectors/track: %d\n", lp->d_nsectors);
	fprintf(f, "tracks/cylinder: %d\n", lp->d_ntracks);
	fprintf(f, "sectors/cylinder: %d\n", lp->d_secpercyl);
	fprintf(f, "cylinders: %d\n", lp->d_ncylinders);
	fprintf(f, "rpm: %d\n", lp->d_rpm);
	fprintf(f, "interleave: %d\n", lp->d_interleave);
	fprintf(f, "trackskew: %d\n", lp->d_trackskew);
	fprintf(f, "cylinderskew: %d\n", lp->d_cylskew);
	fprintf(f, "headswitch: %d\t\t# milliseconds\n", lp->d_headswitch);
	fprintf(f, "track-to-track seek: %d\t# milliseconds\n", lp->d_trkseek);
	fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		fprintf(f, "%ld ", lp->d_drivedata[j]);
	fprintf(f, "\n\n%d partitions:\n", lp->d_npartitions);
	fprintf(f,
	    "#        size   offset    fstype   [fsize bsize]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			fprintf(f, "  %c: %8ld %8ld  ", 'a' + i,
			   pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				fprintf(f, "%8.8s", fstypenames[pp->p_fstype]);
			else
				fprintf(f, "%8d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_V71K:
			case FS_UNUSED:				/* XXX */
				fprintf(f, "    %5d %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag);
				break;

			default:
				fprintf(f, "%20.20s", "");
				break;
			}
			fprintf(f, "\t# (Cyl. %4ld",
			    pp->p_offset / lp->d_secpercyl);
			if (pp->p_offset % lp->d_secpercyl)
			    putc('*', f);
			else
			    putc(' ', f);
			fprintf(f, "- %ld",
			    (pp->p_offset + 
			    pp->p_size + lp->d_secpercyl - 1) /
			    lp->d_secpercyl - 1);
			if (pp->p_size % lp->d_secpercyl)
			    putc('*', f);
			fprintf(f, ")\n");
		}
	}
	fflush(f);
}

edit(lp, f)
	struct disklabel *lp;
	int f;
{
	register int c;
	struct disklabel label;
	FILE *fd;
	char *mktemp();

	(void) mktemp(tmpfil);
	fd = fopen(tmpfil, "w");
	if (fd == NULL) {
		fprintf(stderr, "%s: Can't create\n", tmpfil);
		return (1);
	}
	(void)fchmod(fileno(fd), 0600);
	display(fd, lp);
	fclose(fd);
	for (;;) {
		if (!editit())
			break;
		fd = fopen(tmpfil, "r");
		if (fd == NULL) {
			fprintf(stderr, "%s: Can't reopen for reading\n",
				tmpfil);
			break;
		}
		bzero((char *)&label, sizeof(label));
		if (getasciilabel(fd, &label)) {
			*lp = label;
			if (writelabel(f, bootarea, lp) == 0) {
				(void) unlink(tmpfil);
				return (0);
			}
		}
		printf("re-edit the label? [y]: "); fflush(stdout);
		c = getchar();
		if (c != EOF && c != (int)'\n')
			while (getchar() != (int)'\n')
				;
		if  (c == (int)'n')
			break;
	}
	(void) unlink(tmpfil);
	return (1);
}

editit()
{
	register int pid, xpid;
	int stat;
	sigset_t set, oset;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGQUIT);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
	while ((pid = fork()) < 0) {

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno != EAGAIN) {
			perror("fork");
			return(0);
		}
		sleep(1);
	}
	if (pid == 0) {
		register char *ed;

		(void)sigprocmask(SIG_SETMASK, &oset, NULL);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	return(!stat);
}

char *
skip(cp)
	register char *cp;
{

	while (*cp != '\0' && isspace(*cp))
		cp++;
	if (*cp == '\0' || *cp == '#')
		return ((char *)NULL);
	return (cp);
}

char *
word(cp)
	register char *cp;
{
	register char c;

	while (*cp != '\0' && !isspace(*cp) && *cp != '#')
		cp++;
	if ((c = *cp) != '\0') {
		*cp++ = '\0';
		if (c != '#')
			return (skip(cp));
	}
	return ((char *)NULL);
}

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by display(),
 * and fill in lp.
 */
getasciilabel(f, lp)
	FILE	*f;
	register struct disklabel *lp;
{
	register char **cpp, *cp;
	register struct partition *pp;
	char *tp, *s, line[BUFSIZ];
	int lineno = 0, errors = 0;
	long v;

	lp->d_bbsize = 512;				/* XXX */
	lp->d_sbsize = SBSIZE;
	while (fgets(line, sizeof(line) - 1, f)) {
		lineno++;
		if (cp = index(line,'\n'))
			*cp = '\0';
		cp = skip(line);
		if (cp == NULL)
			continue;
		tp = index(cp, ':');
		if (tp == NULL) {
			fprintf(stderr, "line %d: syntax error\n", lineno);
			errors++;
			continue;
		}
		*tp++ = '\0', tp = skip(tp);
		if (streq(cp, "type")) {
			if (tp == NULL)
				tp = "unknown";
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, tp)) {
					lp->d_type = cpp - dktypenames;
					goto next;
				}
			v = atoi(tp);
			if ((unsigned)v >= DKMAXTYPES)
				fprintf(stderr, "line %d:%s %d\n", lineno,
				    "Warning, unknown disk type", v);
			lp->d_type = v;
			continue;
		}
		if (streq(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = word(cp);
				if (streq(cp, "removeable"))
					v |= D_REMOVABLE;
				else if (streq(cp, "ecc"))
					v |= D_ECC;
				else if (streq(cp, "badsect"))
					v |= D_BADSECT;
				else {
					fprintf(stderr,
					    "line %d: %s: bad flag\n",
					    lineno, cp);
					errors++;
				}
			}
			lp->d_flags = v;
			continue;
		}
		if (streq(cp, "drivedata")) {
			register int i;

			for (i = 0; (cp = tp) && *cp != '\0' && i < NDDATA;) {
				lp->d_drivedata[i++] = atol(cp);
				tp = word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%ld partitions", &v) == 1) {
			if (v == 0 || (unsigned)v > MAXPARTITIONS) {
				fprintf(stderr,
				    "line %d: bad # of partitions\n", lineno);
				lp->d_npartitions = MAXPARTITIONS;
				errors++;
			} else
				lp->d_npartitions = v;
			continue;
		}
		if (tp == NULL)
			tp = "";
		if (streq(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof (lp->d_typename));
			continue;
		}
		if (streq(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof (lp->d_packname));
			continue;
		}
/*
 * ARGH!  Have to fix the 'se#256' entries in disktab after all or this
 * check will cause errors.
*/
		if (streq(cp, "bytes/sector")) {
			v = atoi(tp);
			if (v <= 0 || (v % 512) != 0) {
				fprintf(stderr,
				    "line %d: %s: bad sector size\n",
				    lineno, tp);
				errors++;
			} else
				lp->d_secsize = v;
			continue;
		}
		if (streq(cp, "sectors/track")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_nsectors = v;
			continue;
		}
		if (streq(cp, "sectors/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_secpercyl = v;
			continue;
		}
		if (streq(cp, "tracks/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_ntracks = v;
			continue;
		}
		if (streq(cp, "cylinders")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_ncylinders = v;
			continue;
		}
		if (streq(cp, "rpm")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_rpm = v;
			continue;
		}
		if (streq(cp, "interleave")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_interleave = v;
			continue;
		}
		if (streq(cp, "trackskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_trackskew = v;
			continue;
		}
		if (streq(cp, "cylinderskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_cylskew = v;
			continue;
		}
		if (streq(cp, "headswitch")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_headswitch = v;
			continue;
		}
		if (streq(cp, "track-to-track seek")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n",
				    lineno, tp, cp);
				errors++;
			} else
				lp->d_trkseek = v;
			continue;
		}
		if ('a' <= *cp && *cp <= 'z' && cp[1] == '\0') {
			unsigned part = *cp - 'a';

			if (part > lp->d_npartitions) {
				fprintf(stderr,
				    "line %d: bad partition name '%c' %d %d\n",
					lineno, *cp, part, lp->d_npartitions);
				errors++;
				continue;
			}
			pp = &lp->d_partitions[part];
#define NXTNUM(n) { \
	cp = tp, tp = word(cp); \
	if (tp == NULL) \
		tp = cp; \
	(n) = atol(cp); \
     }

			NXTNUM(v);
			if (v < 0) {
				fprintf(stderr,
				    "line %d: %s: bad partition size\n",
				    lineno, cp);
				errors++;
			} else
				pp->p_size = v;
			NXTNUM(v);
			if (v < 0) {
				fprintf(stderr,
				    "line %d: %s: bad partition offset\n",
				    lineno, cp);
				errors++;
			} else
				pp->p_offset = v;
			cp = tp, tp = word(cp);
			cpp = fstypenames;
			for (; cpp < &fstypenames[FSMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, cp)) {
					pp->p_fstype = cpp - fstypenames;
					goto gottype;
				}
			if (isdigit(*cp))
				v = atoi(cp);
			else
				v = FSMAXTYPES;
			if ((unsigned)v >= FSMAXTYPES) {
				fprintf(stderr, "line %d: %s %s\n", lineno,
				    "Warning, unknown filesystem type", cp);
				v = FS_UNUSED;
			}
			pp->p_fstype = v;
	gottype:

			switch (pp->p_fstype) {

			case FS_UNUSED:				/* XXX */
			case FS_V71K:
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				break;

			default:
				break;
			}
			continue;
		}
		fprintf(stderr, "line %d: %s: Unknown disklabel field\n",
		    lineno, cp);
		errors++;
	next:
		;
	}
	errors += checklabel(lp);
	return (errors == 0);
}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 */
checklabel(lp)
	register struct disklabel *lp;
{
	register struct partition *pp;
	int i, errors = 0;
	char part;

	if (lp->d_secsize == 0) {
		fprintf(stderr, "sector size %d\n", lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		fprintf(stderr, "sectors/track %d\n", lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		fprintf(stderr, "tracks/cylinder %d\n", lp->d_ntracks);
		return (1);
	}
	if  (lp->d_ncylinders == 0) {
		fprintf(stderr, "cylinders/unit %d\n", lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		Warning("revolutions/minute %d", lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = (long) lp->d_secpercyl * lp->d_ncylinders;
	if (lp->d_bbsize == 0) {
		fprintf(stderr, "boot block size %d\n", lp->d_bbsize);
		errors++;
	} else if (lp->d_bbsize % lp->d_secsize)
		Warning("boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		fprintf(stderr, "super block size %d\n", lp->d_sbsize);
		errors++;
	} else if (lp->d_sbsize % lp->d_secsize)
		Warning("super block size %% sector-size != 0");
	if (lp->d_npartitions > MAXPARTITIONS)
		Warning("number of partitions (%d) > MAXPARTITIONS (%d)",
		    lp->d_npartitions, MAXPARTITIONS);
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			Warning("partition %c: size 0, but offset %ld",
			    part, pp->p_offset);
#ifdef notdef
		if (pp->p_size % lp->d_secpercyl)
			Warning("partition %c: size %% cylinder-size != 0",
			    part);
		if (pp->p_offset % lp->d_secpercyl)
			Warning("partition %c: offset %% cylinder-size != 0",
			    part);
#endif
		if (pp->p_offset > lp->d_secperunit) {
			fprintf(stderr,
			    "partition %c: offset past end of unit %ld %ld\n",
				part, pp->p_offset, lp->d_secperunit);
			errors++;
		}
		if (pp->p_offset + pp->p_size > lp->d_secperunit) {
			fprintf(stderr,
			    "partition %c: extends past end of unit %ld %ld %ld\n",
			    part, pp->p_offset, pp->p_size, lp->d_secperunit);
			errors++;
		}
	}
	for (; i < MAXPARTITIONS; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size || pp->p_offset)
			Warning("unused partition %c: size %ld offset %ld",
			    'a' + i, pp->p_size, pp->p_offset);
	}
	return (errors);
}

/*VARARGS1*/
Warning(fmt, a1, a2, a3, a4, a5)
	char *fmt;
{

	fprintf(stderr, "Warning, ");
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fprintf(stderr, "\n");
}

Perror(str)
	char *str;
{
	fputs("disklabel: ", stderr); perror(str);
	exit(4);
}

usage()
{
	fprintf(stderr,
"%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n",
"usage: disklabel [-r] disk",
		"(to read label)",
"or disklabel -w [-r] disk type [ packid ]",
		"(to write label with existing boot program)",
"or disklabel -e [-r] disk",
		"(to edit label)",
"or disklabel -R [-r] disk protofile",
		"(to restore label with existing boot program)",
"or disklabel -B [ -b bootprog ] disk [ type ]",
		"(to install boot program with existing on-disk label)",
"or disklabel -w -B [ -b bootprog ] disk type [ packid ]",
		"(to write label and install boot program)",
"or disklabel -R -B [ -b bootprog ] disk protofile [ type ]",
		"(to restore label and install boot program)",
"or disklabel [-NW] disk",
		"(to write disable/enable label)");
	exit(1);
}
