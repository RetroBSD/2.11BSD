#define	COMPAT

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)newfs.c	6.3 (2.11BSD) 1996/11/16";
#endif

/*
 * newfs: friendly front end to mkfs
 *
 *  The installation of bootblocks has been moved to 'disklabel(8)'.  This
 *  program expects to find a disklabel present which will contain the
 *  geometry and partition size information.  If 'newfs' is being run on
 *  a system without disklabels implemented in the kernel you must specify
 *  "-T diskname" (where 'diskname' is an entry in /etc/disktab).
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/file.h>

#include <ctype.h>
#include <errno.h>
#include <paths.h>
#include <stdlib.h>
#include <syslog.h>
#include <varargs.h>

#ifdef	COMPAT
	char	*disktype;
	int	unlabeled;
#endif
	int	Nflag;
	struct	disklabel *getdisklabel();

extern	char	*__progname;

main(argc, argv)
	int	argc;
	char	**argv;
{
	register struct disklabel	*lp;
	register struct partition	*pp;
	char	*cp;
	struct stat	st;
	long	fssize, ltmp;
	int	f_n = 0, f_m = 0;
	u_int	f_i = 4096;
	int	ch, status, logsec;
	int	fsi;
	char	device[MAXPATHLEN], cmd[BUFSIZ], *index(), *rindex();
	char	*special;

	while ((ch = getopt(argc,argv,"T:Nvm:s:n:i:")) != EOF)
		switch((char)ch) {
		case 'N':
		case 'v':
			Nflag = 1;
			break;
#ifdef	COMPAT
		case 'T':
			disktype = optarg;
			break;
#endif
		case 'm':
			ltmp = atol(optarg);
			if	(ltmp <= 0 || ltmp > 32)
				fatal("%s: out of 1 - 32 range", optarg);
			f_m = (int)ltmp;
			break;
		case	'n':
			ltmp = atol(optarg);
/*
 * If the upper bound is changed here then mkfs.c must also be changed
 * also else mkfs will cap the value to its limit.
*/
			if	(ltmp <= 0 || ltmp > 500)
				fatal("%s: out of 1 - 500 range", optarg);
			f_n = (int)ltmp;
			break;
		case	'i':
			ltmp = atol(optarg);
			if	(ltmp < 512 || ltmp > 65536L)
				fatal("%s: out of 512 - 65536 range", optarg);
			f_i = (u_int)ltmp;
			break;
		case 's':
			fssize = atol(optarg);
			if (fssize <= 0)
				fatal("%s: bad file system size", optarg);
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;
	if (argc != 2 && argc != 1)
		usage();

	/* figure out device name */
	special = argv[0];
	cp = rindex(special, '/');
	if (cp == 0) {
		/*
		 * No path prefix; try /dev/r%s then /dev/%s
		 */
		 (void)sprintf(device, "%s/r%s", _PATH_DEV, special);
		 if (stat(device, &st) == -1)
			(void)sprintf(device, "%s/%s", _PATH_DEV, special);
		special = device;
	}

	fsi = open(special, O_RDONLY);
	if (fsi < 0)
		fatal("%s: %s", special, strerror(errno));

	/* see if it exists and of a legal type */
	if (fstat(fsi, &st) == -1)
		fatal("%s: %s", special, strerror(errno));
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a character device", special);
	cp = index(argv[0], '\0') - 1;
	if (cp == 0 || (*cp < 'a' || *cp > 'h') && !isdigit(*cp))
		fatal("%s: can't figure out file system partition", argv[0]);

#ifdef	COMPAT
	if (disktype == NULL)
		disktype = argv[1];
#endif
	lp = getdisklabel(special, fsi);
	if (isdigit(*cp))
		pp = &lp->d_partitions[0];
	else
		pp = &lp->d_partitions[*cp - 'a'];
	if (pp->p_size <= 0)
		fatal("%s: '%c' partition is unavailable", argv[0], *cp);
#ifdef	nothere
	if (pp->p_fstype == FS_BOOT)
		fatal("%s: '%c' partition  overlaps boot program",argv[0], *cp);
#endif

	if (fssize == 0)
		fssize = pp->p_size;
	if (fssize > pp->p_size)
		fatal("%s: maximum file system size on '%c' partition is %ld",
			argv[0], *cp, pp->p_size);
	/*
	 * Convert from sectors to logical blocks.  Note that sector size
	 * must evenly devide DEV_BSIZE!!!!!
	 */

	/*
	 * getdisklabel(3) forces the sector size to 512 because any other
	 * choice would wreck havoc in the disklabel(8) program and result
	 * in corrupted/destroyed filesystems.  DEV_BSIZE had better be
	 * 1024!
	*/
	if (lp->d_secsize != 512)
		fatal("%s: sector size not 512", argv[0]);
	logsec = DEV_BSIZE/lp->d_secsize;
	fssize /= logsec;

	/* build command */
	if	(f_m == 0)	/* If never specified then use default of 2 */
		f_m = 2;
	if	(f_n == 0)	/* If never specified then 1/2 the cyl size */
		f_n = lp->d_secpercyl / logsec;

	sprintf(cmd, "/sbin/mkfs -m %d -n %d -i %u -s %ld %s", f_m, f_n, f_i,
		fssize, special);
	printf("newfs: %s\n", cmd);

	close(fsi);

	if (Nflag)
		exit(0);

	if (status = system(cmd))
		exit(status >> 8);
	exit(0);
}

#ifdef COMPAT
char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
char lmsg[] = "%s: can't read disk label";
#endif

struct disklabel *
getdisklabel(s, fd)
	char *s;
	int fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
#ifdef COMPAT
		if (disktype) {
			struct disklabel *lp, *getdiskbyname();

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				fatal("%s: unknown disk type", disktype);
			return (lp);
		}
#endif
		warn("ioctl (GDINFO)");
		fatal(lmsg, s);
	}
	return (&lab);
}

static
usage()
{

	fprintf(stderr,"usage: %s [-N] [-m freelist-gap] [-s filesystem size] ",
		__progname);
	fprintf(stderr, "[-i bytes/inode] [-n freelist-modulus] ");
#ifdef	COMPAT
	fputs("[-T disk-type] ", stderr);
#endif
	fputs("special-device\n", stderr);
	exit(1);
}

/*VARARGS*/
void
fatal(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list ap;

	va_start(ap);

	if (fcntl(fileno(stderr), F_GETFL) < 0) {
		openlog(__progname, LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, fmt, ap);
		closelog();
	} else {
		vwarnx(fmt, ap);
	}
	va_end(ap);
	exit(1);
	/*NOTREACHED*/
}
