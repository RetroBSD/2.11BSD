/*
 * afio.c
 *
 * Manipulate archives and files.
 *
 * Copyright (c) 1985 Lachman Associates, Inc..
 *
 * This software was written by Mark Brukhartz at Lachman Associates,
 * Inc.. It may be distributed within the following restrictions:
 *	(1) It may not be sold at a profit.
 *	(2) This credit and notice must remain intact.
 * This software may be distributed with other software by a commercial
 * vendor, provided that it is included at no additional charge.
 *
 * Please report bugs to "..!ihnp4!laidbak!mdb".
 *
 * Options:
 *  o Define INDEX to use index() in place of strchr() (v7, BSD).
 *  o Define MEMCPY when an efficient memcpy() exists (SysV).
 *  o Define MKDIR when a mkdir() system call is present (4.2BSD, SysVr3).
 *  o Define NOVOID if your compiler doesn't like void casts.
 *  o Define SYSTIME to use <sys/time.h> rather than <time.h> (4.2BSD).
 *  o Define VOIDFIX to allow pointers to functions returning void (non-PCC).
 *  o Define CTC3B2 to support AT&T 3B2 streaming cartridge tape.
 */

static char *ident = "$Header: afio.c,v 1.68.1 96/3/21 13:07:11 mdb Exp $";

#include <stdio.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#ifndef	major
#	include <sys/sysmacros.h>
#endif	/* major */

#ifdef	SYSTIME
#	include <sys/time.h>
#else	/* SYSTIME */
#	include <time.h>
#endif	/* SYSTIME */

#ifdef	CTC3B2
#	include <sys/vtoc.h>
#	include <sys/ct.h>
#endif	/* CTC3B2 */

/*
 * Address link information base.
 */
#define	linkhash(ino)	\
	(linkbase + (ino) % nel(linkbase))

/*
 * Mininum value.
 */
#define	min(one, two)	\
	(one < two ? one : two)

/*
 * Number of array elements.
 */
#define	nel(a)		\
	(sizeof(a) / sizeof(*(a)))

/*
 * Remove a file or directory.
 */
#define	remove(name, asb) \
	(((asb)->sb_mode & S_IFMT) == S_IFDIR ? rmdir(name) : unlink(name))

/*
 * Swap bytes.
 */
#define	swab(n)		\
	((((ushort)(n) >> 8) & 0xff) | (((ushort)(n) << 8) & 0xff00))

/*
 * Cast and reduce to unsigned short.
 */
#define	ush(n)		\
	(((ushort) (n)) & 0177777)

/*
 * Definitions.
 */
#define	reg	register	/* Convenience */
#define	uint	unsigned int	/* Not always in types.h */
#define	ushort	unsigned short	/* Not always in types.h */
#define	BLOCK	5120		/* Default archive block size */
#define	FSBUF	(8*1024)	/* Filesystem buffer size */
#define	H_COUNT	10		/* Number of items in ASCII header */
#define	H_PRINT	"%06o%06o%06o%06o%06o%06o%06o%011lo%06o%011lo"
#define	H_SCAN	"%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6o%11lo"
#define	H_STRLEN 70		/* ASCII header string length */
#define	M_ASCII "070707"	/* ASCII magic number */
#define	M_BINARY 070707		/* Binary magic number */
#define	M_STRLEN 6		/* ASCII magic number length */
#define	NULLDEV	-1		/* Null device code */
#define	NULLINO	0		/* Null inode number */
#define	PATHELEM 256		/* Pathname element count limit */
#define	PATHSIZE 1024		/* Pathname length limit */
#define	S_IFSHF	12		/* File type shift (shb in stat.h) */
#define	S_IPERM	07777		/* File permission bits (shb in stat.h) */
#define	S_IPEXE	07000		/* Special execution bits (shb in stat.h) */
#define	S_IPOPN	0777		/* Open access bits (shb in stat.h) */
#define	STDIN	0		/* Standard input file descriptor */
#define	STDOUT	1		/* Standard output file descriptor */
#define	TTY	"/dev/tty"	/* For volume-change queries */

/*
 * Some versions of the portable "C" compiler (PCC) can't handle
 * pointers to functions returning void.
 */
#ifdef	VOIDFIX
#	define	VOIDFN	void	/* Expect "void (*fnptr)()" to work */
#else	/* VOIDFIX */
#	define	VOIDFN	int	/* Avoid PCC "void (*fnptr)()" bug */
#endif	/* VOIDFIX */

/*
 * Trailer pathnames. All must be of the same length.
 */
#define	TRAILER	"TRAILER!!!"	/* Archive trailer (cpio compatible) */
#define	TRAILZ	11		/* Trailer pathname length (including null) */

/*
 * Open modes; there is no <fcntl.h> with v7 UNIX.
 */
#define	O_RDONLY 0		/* Read-only */
#define	O_WRONLY 1		/* Write-only */
#define	O_RDWR	2		/* Read/write */

/*
 * V7 and BSD UNIX use old-fashioned names for a couple of
 * string functions.
 */
#ifdef	INDEX
#	define	strchr	index	/* Forward character search */
#	define	strrchr	rindex	/* Reverse character search */
#endif	/* INDEX */

/*
 * Some compilers can't handle void casts.
 */
#ifdef	NOVOID
#	define	VOID		/* Omit void casts */
#else	/* NOVOID */
#	define	VOID	(void)	/* Quiet lint about ignored return values */
#endif	/* NOVOID */

/*
 * Adb is more palatable when static functions and variables are
 * declared as globals. Lint gives more useful information when
 * statics are truly static.
 */
#ifdef	lint
#	define	STATIC	static	/* Declare static variables for lint */
#else	/* lint */
#	define	STATIC		/* Make static variables global for adb */
#endif	/* lint */

/*
 * Simple types.
 */
typedef struct group	Group;	/* Structure for getgrgid(3) */
typedef struct passwd	Passwd;	/* Structure for getpwuid(3) */
typedef struct tm	Time;	/* Structure for localtime(3) */

#ifdef	S_IFLNK
	/*
	 * File status with symbolic links. Kludged to hold symbolic
	 * link pathname within structure.
	 */
	typedef struct {
		struct stat	sb_stat;
		char		sb_link[PATHSIZE];
	} Stat;
#	define	STAT(name, asb)		stat(name, &(asb)->sb_stat)
#	define	FSTAT(fd, asb)		fstat(fd, &(asb)->sb_stat)
#	define	LSTAT(name, asb)	lstat(name, &(asb)->sb_stat)
#	define	sb_dev		sb_stat.st_dev
#	define	sb_ino		sb_stat.st_ino
#	define	sb_mode		sb_stat.st_mode
#	define	sb_nlink	sb_stat.st_nlink
#	define	sb_uid		sb_stat.st_uid
#	define	sb_gid		sb_stat.st_gid
#	define	sb_rdev		sb_stat.st_rdev
#	define	sb_size		sb_stat.st_size
#	define	sb_atime	sb_stat.st_atime
#	define	sb_mtime	sb_stat.st_mtime
#	define	sb_ctime	sb_stat.st_ctime
#	define	sb_blksize	sb_stat.st_blksize
#	define	sb_blocks	sb_stat.st_blocks
#else	/* S_IFLNK */
	/*
	 * File status without symbolic links.
	 */
	typedef	struct stat	Stat;
#	define	STAT(name, asb)		stat(name, asb)
#	define	FSTAT(fd, asb)		fstat(fd, asb)
#	define	LSTAT(name, asb)	stat(name, asb)
#	define	sb_dev		st_dev
#	define	sb_ino		st_ino
#	define	sb_mode		st_mode
#	define	sb_nlink	st_nlink
#	define	sb_uid		st_uid
#	define	sb_gid		st_gid
#	define	sb_rdev		st_rdev
#	define	sb_size		st_size
#	define	sb_atime	st_atime
#	define	sb_mtime	st_mtime
#	define	sb_ctime	st_ctime
#endif	/* S_IFLNK */

/*
 * Binary archive header (obsolete).
 */
typedef struct {
	short	b_dev;			/* Device code */
	ushort	b_ino;			/* Inode number */
	ushort	b_mode;			/* Type and permissions */
	ushort	b_uid;			/* Owner */
	ushort	b_gid;			/* Group */
	short	b_nlink;		/* Number of links */
	short	b_rdev;			/* Real device */
	ushort	b_mtime[2];		/* Modification time (hi/lo) */
	ushort	b_name;			/* Length of pathname (with null) */
	ushort	b_size[2];		/* Length of data */
} Binary;

/*
 * Child process structure.
 */
typedef struct child {
	struct child	*c_forw;	/* Forward link */
	int		c_pid;		/* Process ID */
	int		c_flags;	/* Flags (CF_) */
	int		c_status;	/* Exit status */
} Child;

/*
 * Child process flags (c_flags).
 */
#define	CF_EXIT	(1<<0)			/* Exited */

/*
 * Hard link sources. One or more are chained from each link
 * structure.
 */
typedef struct name {
	struct name	*p_forw;	/* Forward chain (terminated) */
	struct name	*p_back;	/* Backward chain (circular) */
	char		*p_name;	/* Pathname to link from */
} Path;

/*
 * File linking information. One entry exists for each unique
 * file with with outstanding hard links.
 */
typedef struct link {
	struct link	*l_forw;	/* Forward chain (terminated) */
	struct link	*l_back;	/* Backward chain (terminated) */
	dev_t		l_dev;		/* Device */
	ino_t		l_ino;		/* Inode */
	ushort		l_nlink;	/* Unresolved link count */
	off_t		l_size;		/* Length */
	Path		*l_path;	/* Pathname(s) to link from */
} Link;

/*
 * Pathnames to (or to not) be processed.
 */
typedef struct pattern {
	struct pattern	*p_forw;	/* Forward chain */
	char		*p_str;		/* String */
	int		p_len;		/* Length of string */
	int		p_not;		/* Reverse logic */
} Pattern;

/*
 * External functions.
 */
void	_exit();
void	exit();
void	free();
char	*getenv();
ushort	getgid();
Group	*getgrgid();
Passwd	*getpwuid();
ushort	getuid();
Time	*localtime();
off_t	lseek();
char	*malloc();
VOIDFN	(*signal())();
uint	sleep();
char	*strcat();
char	*strchr();
char	*strcpy();
char	*strncpy();
char	*strrchr();
time_t	time();
ushort	umask();

/*
 * Internal functions.
 */
VOIDFN	copyin();
VOIDFN	copyout();
int	dirchg();
int	dirmake();
int	dirneed();
void	fatal();
VOIDFN	in();
void	inalloc();
int	inascii();
int	inavail();
int	inbinary();
int	indata();
int	inentry();
int	infill();
int	inhead();
int	inread();
int	inskip();
int	inswab();
int	lineget();
void	linkalso();
Link	*linkfrom();
void	linkleft();
Link	*linkto();
char	*memget();
char	*memstr();
int	mkdir();
void	nameadd();
int	namecmp();
int	nameopt();
void	next();
void	nextask();
void	nextclos();
int	nextopen();
int	openi();
int	openo();
int	openq();
int	options();
off_t	optsize();
VOIDFN	out();
void	outalloc();
uint	outavail();
int	outdata();
void	outeof();
void	outflush();
void	outhead();
void	outpad();
void	outwait();
void	outwrite();
VOIDFN	pass();
void	passdata();
int	passitem();
int	pipechld();
int	pipeopen();
void	pipewait();
void	prsize();
int	rmdir();
int	swrite();
char	*syserr();
VOIDFN	toc();
void	tocentry();
void	tocmode();
void	usage();
int	warn();
int	warnarch();
int	xfork();
void	xpause();
int	xwait();

/*
 * External variables.
 */
extern int	errno;		/* System error code */

/*
 * Static variables.
 */
#ifdef	CTC3B2
STATIC short	Cflag;		/* Enable 3B2 CTC streaming (kludge) */
#endif	/* CTC3B2 */
STATIC short	dflag;		/* Don't create missing directories */
STATIC short	fflag;		/* Fork before writing to archive */
STATIC short	gflag;		/* Change to input file directories */
STATIC short	hflag;		/* Follow symbolic links */
STATIC short	jflag;		/* Don't generate sparse filesystem blocks */
STATIC short	kflag;		/* Skip initial junk to find a header */
STATIC short	lflag;		/* Link rather than copying (when possible) */
STATIC short	mflag;		/* Ignore archived timestamps */
STATIC short	nflag;		/* Keep newer existing files */
STATIC short	uflag;		/* Report files with unseen links */
STATIC short	vflag;		/* Verbose */
STATIC short	xflag;		/* Retain file ownership */
STATIC short	zflag;		/* Print final statistics */
STATIC uint	arbsize = BLOCK;/* Archive block size */
STATIC short	areof;		/* End of input volume reached */
STATIC int	arfd;		/* Archive file descriptor */
STATIC off_t	arleft;		/* Space remaining within current volume */
STATIC char	*arname;	/* Expanded archive name */
STATIC uint	arpad;		/* Final archive block padding boundary */
STATIC char	arspec[PATHSIZE];/* Specified archive name */
STATIC off_t	aruntil;	/* Volume size limit */
STATIC uint	arvolume;	/* Volume number */
STATIC uint	buflen;		/* Archive buffer length */
STATIC char	*buffer;	/* Archive buffer */
STATIC char	*bufidx;	/* Archive buffer index */
STATIC char	*bufend;	/* End of data within archive buffer */
STATIC Child	*children;	/* Child processes */
STATIC ushort	gid;		/* Group ID */
STATIC Link	*linkbase[256];	/* Unresolved link information */
STATIC ushort	mask;		/* File creation mask */
STATIC char	*myname;	/* Arg0 */
STATIC char	*optarg;	/* Option argument */
STATIC int	optind;		/* Command line index */
STATIC int	outpid;		/* Process ID of outstanding outflush() */
STATIC Pattern	*pattern;	/* Pathname matching patterns */
STATIC char	pwd[PATHSIZE];	/* Working directory (with "-g") */
STATIC int	pipepid;	/* Pipeline process ID */
STATIC time_t	timenow;	/* Current time */
STATIC time_t	timewait;	/* Time spent awaiting new media */
STATIC off_t	total;		/* Total number of bytes transferred */
STATIC int	ttyf;		/* For interactive queries (yuk) */
STATIC ushort	uid;		/* User ID */

main(ac, av)
int		ac;
reg char	**av;
{
	reg int		c;
	reg uint	group = 1;
	reg VOIDFN	(*fn)() = NULL;
	reg time_t	timedone;
	auto char	remote[PATHSIZE];

	if (myname = strrchr(*av, '/'))
		++myname;
	else
		myname = *av;
	mask = umask(0);
	ttyf = openq();
	uid = getuid();
	gid = getgid();
	if (uid == 0)
		++xflag;
	VOID signal(SIGPIPE, SIG_IGN);
	while (c = options(ac, av, "ioptIOVCb:c:de:fghjklmns:uvxXy:Y:z")) {
		switch (c) {
		case 'i':
			if (fn)
				usage();
			fn = in;
			break;
		case 'o':
			if (fn)
				usage();
			fn = out;
			break;
		case 'p':
			if (fn)
				usage();
			fn = pass;
			break;
		case 't':
			if (fn)
				usage();
			fn = toc;
			break;
		case 'I':
			if (fn)
				usage();
			fn = copyin;
			break;
		case 'O':
			if (fn)
				usage();
			fn = copyout;
			break;
		case 'V':
			VOID printf("%s\n", ident);
			exit(0);
#ifdef	CTC3B2
		case 'C':
			++Cflag;
			arbsize = 31 * 512;
			group = 10;
			aruntil = 1469 * 31 * 512;
			break;
#endif	/* CTC3B2 */
		case 'b':
			if ((arbsize = (uint) optsize(optarg)) == 0)
				fatal(optarg, "Bad block size");
			break;
		case 'c':
			if ((group = (uint) optsize(optarg)) == 0)
				fatal(optarg, "Bad buffer count");
			break;
		case 'd':
			++dflag;
			break;
		case 'e':
			arpad = (uint) optsize(optarg);
			break;
		case 'f':
			++fflag;
			break;
		case 'g':
			++gflag;
			break;
		case 'h':
			++hflag;
			break;
		case 'j':
			++jflag;
			break;
		case 'k':
			++kflag;
			break;
		case 'l':
			++lflag;
			break;
		case 'm':
			++mflag;
			break;
		case 'n':
			++nflag;
			break;
		case 's':
			aruntil = optsize(optarg);
			break;
		case 'u':
			++uflag;
			break;
		case 'v':
			++vflag;
			break;
		case 'x':
			++xflag;
			break;
		case 'X':
			xflag = 0;
			break;
		case 'y':
			nameadd(optarg, 0);
			break;
		case 'Y':
			nameadd(optarg, 1);
			break;
		case 'z':
			++zflag;
			break;
		default:
			usage();
		}
	}
	if (fn == NULL || av[optind] == NULL)
		usage();
	buflen = arbsize * group;
	if (arpad == 0)
		arpad = arbsize;
	if (fn != pass) {
		reg char	*colon;
		reg char	*equal;
		reg int		isoutput = (fn == out || fn == copyout);

		arname = strcpy(arspec, av[optind++]);
		if (colon = strchr(arspec, ':')) {
			*colon++ = '\0';
			if (equal = strchr(arspec, '='))
				*equal++ = '\0';
			VOID sprintf(arname = remote,
			    "!rsh %s %s -%c -b %u -c %u %s",
			    arspec, equal ? equal : myname,
			    isoutput ? 'O' : 'I', arbsize,
			    group, colon);
			if (equal)
				*--equal = '=';
			*--colon = ':';
		}
		if (gflag && *arname != '/' && *arname != '!')
			fatal(arspec, "Relative pathname");
		if ((buffer = bufidx = bufend = malloc(buflen)) == NULL)
			fatal(arspec, "Cannot allocate I/O buffer");
		if (nextopen(isoutput ? O_WRONLY : O_RDONLY) < 0)
			exit(1);
	}
	timenow = time((time_t *) NULL);
	(*fn)(av + optind);
	timedone = time((time_t *) NULL);
	if (uflag)
		linkleft();
	if (zflag) {
		reg FILE	*stream;

		stream = fn == toc || arfd == STDOUT ? stderr : stdout;
		VOID fprintf(stream, "%s: ", myname);
		prsize(stream, total);
		VOID fprintf(stream, " bytes %s in %lu seconds\n",
		  fn == pass
		    ? "transferred"
		    : fn == out || fn == copyout
		      ? "written"
		      : "read",
		  timedone - timenow - timewait);
	}
	nextclos();
	exit(0);
	/* NOTREACHED */
}

/*
 * copyin()
 *
 * Copy directly from the archive to the standard output.
 */
STATIC VOIDFN
copyin(av)
reg char	**av;
{
	reg int		got;
	reg uint	have;

	if (*av)
		fatal(*av, "Extraneous argument");
	while (!areof) {
		VOID infill();
		while (have = bufend - bufidx)
			if ((got = write(STDOUT, bufidx, have)) < 0)
				fatal("<stdout>", syserr());
			else if (got > 0)
				bufidx += got;
			else
				return;
	}
}

/*
 * copyout()
 *
 * Copy directly from the standard input to the archive.
 */
STATIC VOIDFN
copyout(av)
reg char	**av;
{
	reg int		got;
	reg uint	want;

	if (*av)
		fatal(*av, "Extraneous argument");
	for (;;) {
		while ((want = bufend - bufidx) == 0)
			outflush();
		if ((got = read(STDIN, bufidx, want)) < 0)
			fatal("<stdin>", syserr());
		else if (got == 0)
			break;
		else
			bufidx += got;
	}
	outflush();
	if (fflag)
		outwait();
}

/*
 * dirchg()
 *
 * Change to the directory containing a given file.
 */
STATIC int
dirchg(name, local)
reg char	*name;
reg char	*local;
{
	reg char	*last;
	reg int		len;
	auto char	dir[PATHSIZE];

	if (*name != '/')
		return (warn(name, "Relative pathname"));
	for (last = name + strlen(name); last[-1] != '/'; --last)
		;
	len = last - name;
	strncpy(dir, name, len)[len] = '\0';
	VOID strcpy(local, *last ? last : ".");
	if (strcmp(dir, pwd) == 0)
		return (0);
	if (chdir(dir) < 0)
		return (warn(name, syserr()));
	VOID strcpy(pwd, dir);
	return (0);
}

/*
 * dirmake()
 *
 * Make a directory. Returns zero if successful, -1 otherwise.
 */
STATIC int
dirmake(name, asb)
reg char	*name;
reg Stat	*asb;
{
	if (mkdir(name, asb->sb_mode & S_IPOPN) < 0)
		return (-1);
	if (asb->sb_mode & S_IPEXE)
		VOID chmod(name, asb->sb_mode & S_IPERM);
	if (xflag)
		VOID chown(name,
		    uid == 0 ? ush(asb->sb_uid) : uid,
		    ush(asb->sb_gid));
	return (0);
}

/*
 * dirneed()
 *
 * Recursively create missing directories (with the same permissions
 * as their first existing parent). Temporarily modifies the 'name'
 * argument string. Returns zero if successful, -1 otherwise.
 */
STATIC int
dirneed(name)
char		*name;
{
	reg char	*cp;
	reg char	*last;
	reg int		ok;
	static Stat	sb;

	last = NULL;
	for (cp = name; *cp; )
		if (*cp++ == '/')
			last = cp;
	if (last == NULL)
		return (STAT(".", &sb));
	*--last = '\0';
	ok = STAT(*name ? name : "/", &sb) == 0
	    ? ((sb.sb_mode & S_IFMT) == S_IFDIR)
	    : (!dflag && dirneed(name) == 0 && dirmake(name, &sb) == 0);
	*last = '/';
	return (ok ? 0 : -1);
}

/*
 * fatal()
 *
 * Print fatal message and exit.
 */
STATIC void
fatal(what, why)
char		*what;
char		*why;
{
	VOID fprintf(stderr,
	    "%s: \"%s\": %s\n",
	    myname, what, why);
	exit(1);
}

/*
 * in()
 *
 * Read an archive.
 */
STATIC VOIDFN
in(av)
reg char	**av;
{
	auto Stat	sb;
	auto char	name[PATHSIZE];

	if (*av)
		fatal(*av, "Extraneous argument");
	name[0] = '\0';
	while (inhead(name, &sb) == 0) {
		if (namecmp(name) < 0 || inentry(name, &sb) < 0)
			if (inskip(sb.sb_size) < 0)
				VOID warn(name, "Skipped file data is corrupt");
		if (vflag)
			VOID fprintf(stderr, "%s\n", name);
	}
}

/*
 * inalloc()
 *
 * Allocate input buffer space (which was previously indexed
 * by inavail()).
 */
STATIC void
inalloc(len)
reg uint	len;
{
	bufidx += len;
	total += len;
}

/*
 * inascii()
 *
 * Read an ASCII header. Returns zero if successful;
 * -1 otherwise. Assumes that the entire magic number
 * has been read.
 */
STATIC int
inascii(magic, name, asb)
reg char	*magic;
reg char	*name;
reg Stat	*asb;
{
	auto uint	namelen;
	auto char	header[H_STRLEN + 1];

	if (strncmp(magic, M_ASCII, M_STRLEN) != 0)
		return (-1);
	if (inread(header, H_STRLEN) < 0)
		return (warnarch("Corrupt ASCII header", (off_t) H_STRLEN));
	header[H_STRLEN] = '\0';
	if (sscanf(header, H_SCAN, &asb->sb_dev,
	    &asb->sb_ino, &asb->sb_mode, &asb->sb_uid,
	    &asb->sb_gid, &asb->sb_nlink, &asb->sb_rdev,
	    &asb->sb_mtime, &namelen, &asb->sb_size) != H_COUNT)
		return (warnarch("Bad ASCII header", (off_t) H_STRLEN));
	if (namelen == 0 || namelen >= PATHSIZE)
		return (warnarch("Bad ASCII pathname length", (off_t) H_STRLEN));
	if (inread(name, namelen) < 0)
		return (warnarch("Corrupt ASCII pathname", (off_t) namelen));
	if (name[namelen - 1] != '\0')
		return (warnarch("Bad ASCII pathname", (off_t) namelen));
	return (0);
}

/*
 * inavail()
 *
 * Index availible input data within the buffer. Stores a pointer
 * to the data and its length in given locations. Returns zero with
 * valid data, -1 if unreadable portions were replaced with nulls.
 */
STATIC int
inavail(bufp, lenp)
reg char	**bufp;
uint		*lenp;
{
	reg uint	have;
	reg int		corrupt = 0;

	while ((have = bufend - bufidx) == 0)
		corrupt |= infill();
	*bufp = bufidx;
	*lenp = have;
	return (corrupt);
}

/*
 * inbinary()
 *
 * Read a binary header. Returns the number of trailing alignment
 * bytes to skip; -1 if unsuccessful.
 */
STATIC int
inbinary(magic, name, asb)
reg char	*magic;
reg char	*name;
reg Stat	*asb;
{
	reg uint	namefull;
	auto Binary	binary;

	if (*((ushort *) magic) != M_BINARY)
		return (-1);
	memcpy((char *) &binary,
	    magic + sizeof(ushort),
	    M_STRLEN - sizeof(ushort));
	if (inread((char *) &binary + M_STRLEN - sizeof(ushort),
	    sizeof(binary) - (M_STRLEN - sizeof(ushort))) < 0)
		return (warnarch("Corrupt binary header",
		    (off_t) sizeof(binary) - (M_STRLEN - sizeof(ushort))));
	asb->sb_dev = binary.b_dev;
	asb->sb_ino = binary.b_ino;
	asb->sb_mode = binary.b_mode;
	asb->sb_uid = binary.b_uid;
	asb->sb_gid = binary.b_gid;
	asb->sb_nlink = binary.b_nlink;
	asb->sb_rdev = binary.b_rdev;
	asb->sb_mtime = binary.b_mtime[0] << 16 | binary.b_mtime[1];
	asb->sb_size = binary.b_size[0] << 16 | binary.b_size[1];
	if (binary.b_name == 0 || binary.b_name >= PATHSIZE)
		return (warnarch("Bad binary pathname length",
		    (off_t) sizeof(binary) - (M_STRLEN - sizeof(ushort))));
	if (inread(name, namefull = binary.b_name + binary.b_name % 2) < 0)
		return (warnarch("Corrupt binary pathname", (off_t) namefull));
	if (name[binary.b_name - 1] != '\0')
		return (warnarch("Bad binary pathname", (off_t) namefull));
	return (asb->sb_size % 2);
}

/*
 * indata()
 *
 * Install data from an archive. Returns given file descriptor.
 */
STATIC int
indata(fd, size, name)
int		fd;
reg off_t	size;
char		*name;
{
	reg uint	chunk;
	reg char	*oops;
	reg int		sparse;
	reg int		corrupt;
	auto char	*buf;
	auto uint	avail;

	corrupt = sparse = 0;
	oops = NULL;
	while (size) {
		corrupt |= inavail(&buf, &avail);
		size -= (chunk = size < avail ? (uint) size : avail);
		if (oops == NULL && (sparse = swrite(fd, buf, chunk)) < 0)
			oops = syserr();
		inalloc(chunk);
	}
	if (corrupt)
		VOID warn(name, "Corrupt archive data");
	if (oops)
		VOID warn(name, oops);
	else if (sparse > 0
	  && (lseek(fd, (off_t) -1, 1) < 0
	    || write(fd, "", 1) != 1))
		VOID warn(name, syserr());
	return (fd);
}

/*
 * inentry()
 *
 * Install a single archive entry. Returns zero if successful, -1 otherwise.
 */
STATIC int
inentry(name, asb)
char		*name;
reg Stat	*asb;
{
	reg Link	*linkp;
	reg int		ifd;
	reg int		ofd;
	auto time_t	tstamp[2];

	if ((ofd = openo(name, asb, linkp = linkfrom(asb), 0)) > 0)
		if (asb->sb_size || linkp == NULL || linkp->l_size == 0)
			VOID close(indata(ofd, asb->sb_size, name));
		else if ((ifd = open(linkp->l_path->p_name, O_RDONLY)) < 0)
			VOID warn(linkp->l_path->p_name, syserr());
		else {
			passdata(linkp->l_path->p_name, ifd, name, ofd);
			VOID close(ifd);
			VOID close(ofd);
		}
	else if (ofd < 0)
		return (-1);
	else if (inskip(asb->sb_size) < 0)
		VOID warn(name, "Redundant file data is corrupt");
	tstamp[0] = tstamp[1] = mflag ? timenow : asb->sb_mtime;
	VOID utime(name, tstamp);
	return (0);
}

/*
 * infill()
 *
 * Fill the archive buffer. Remembers mid-buffer read failures and
 * reports them the next time through. Replaces unreadable data with
 * null characters. Returns zero with valid data, -1 otherwise.
 */
STATIC int
infill()
{
	reg int		got;
	static int	failed;

	bufend = bufidx = buffer;
	if (!failed) {
		if (areof)
			if (total == 0)
				fatal(arspec, "No input");
			else
				next(O_RDONLY, "Input EOF");
		if (aruntil && arleft < arbsize)
			next(O_RDONLY, "Input limit reached");
		while (!failed
		    && !areof
		    && (aruntil == 0 || arleft >= arbsize)
		    && buffer + buflen - bufend >= arbsize) {
			if ((got = read(arfd, bufend, arbsize)) > 0) {
				bufend += got;
				arleft -= got;
			} else if (got < 0)
				failed = warnarch(syserr(),
				    (off_t) 0 - (bufend - bufidx));
			else
				++areof;
		}
	}
	if (failed && bufend == buffer) {
		failed = 0;
		for (got = 0; got < arbsize; ++got)
			*bufend++ = '\0';
		return (-1);
	}
	return (0);
}

/*
 * inhead()
 *
 * Read a header. Quietly translates old-fashioned binary cpio headers
 * (and arranges to skip the possible alignment byte). Returns zero if
 * successful, -1 upon archive trailer.
 */
STATIC int
inhead(name, asb)
reg char	*name;
reg Stat	*asb;
{
	reg off_t	skipped;
	auto char	magic[M_STRLEN];
	static int	align;

	if (align > 0)
		VOID inskip((off_t) align);
	align = 0;
	for (;;) {
		VOID inread(magic, M_STRLEN);
		skipped = 0;
		while ((align = inascii(magic, name, asb)) < 0
		    && (align = inbinary(magic, name, asb)) < 0
		    && (align = inswab(magic, name, asb)) < 0) {
			if (++skipped == 1) {
				if (!kflag && total - sizeof(magic) == 0)
					fatal(arspec, "Unrecognizable archive");
				VOID warnarch("Bad magic number",
				    (off_t) sizeof(magic));
				if (name[0])
					VOID warn(name, "May be corrupt");
			}
			memcpy(magic, magic + 1, sizeof(magic) - 1);
			VOID inread(magic + sizeof(magic) - 1, 1);
		}
		if (skipped) {
			VOID warnarch("Apparently resynchronized",
			    (off_t) sizeof(magic));
			VOID warn(name, "Continuing");
		}
		if (strcmp(name, TRAILER) == 0)
			return (-1);
		if (nameopt(name) >= 0)
			break;
		VOID inskip(asb->sb_size + align);
	}
#ifdef	S_IFLNK
	if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
		if (inread(asb->sb_link, (uint) asb->sb_size) < 0) {
			VOID warn(name, "Corrupt symbolic link");
			return (inhead(name, asb));
		}
		asb->sb_link[asb->sb_size] = '\0';
		asb->sb_size = 0;
	}
#endif	/* S_IFLNK */
	if (name[0] == '/')
		if (name[1])
			while (name[0] = name[1])
				++name;
		else
			name[0] = '.';
	asb->sb_atime = asb->sb_ctime = asb->sb_mtime;
	return (0);
}

/*
 * inread()
 *
 * Read a given number of characters from the input archive. Returns
 * zero with valid data, -1 if unreadable portions were replaced by
 * null characters.
 */
STATIC int
inread(dst, len)
reg char	*dst;
uint		len;
{
	reg uint	have;
	reg uint	want;
	reg int		corrupt = 0;
	char		*endx = dst + len;

	while (want = endx - dst) {
		while ((have = bufend - bufidx) == 0)
			corrupt |= infill();
		if (have > want)
			have = want;
		memcpy(dst, bufidx, have);
		bufidx += have;
		dst += have;
		total += have;
	}
	return (corrupt);
}

/*
 * inskip()
 *
 * Skip input archive data. Returns zero under normal circumstances,
 * -1 if unreadable data is encountered.
 */
STATIC int
inskip(len)
reg off_t	len;
{
	reg uint	chunk;
	reg int		corrupt = 0;

	while (len) {
		while ((chunk = bufend - bufidx) == 0)
			corrupt |= infill();
		if (chunk > len)
			chunk = len;
		bufidx += chunk;
		len -= chunk;
		total += chunk;
	}
	return (corrupt);
}

/*
 * inswab()
 *
 * Read a reversed byte order binary header. Returns the number
 * of trailing alignment bytes to skip; -1 if unsuccessful.
 */
STATIC int
inswab(magic, name, asb)
reg char	*magic;
reg char	*name;
reg Stat	*asb;
{
	reg ushort	namesize;
	reg uint	namefull;
	auto Binary	binary;

	if (*((ushort *) magic) != swab(M_BINARY))
		return (-1);
	memcpy((char *) &binary,
	    magic + sizeof(ushort),
	    M_STRLEN - sizeof(ushort));
	if (inread((char *) &binary + M_STRLEN - sizeof(ushort),
	    sizeof(binary) - (M_STRLEN - sizeof(ushort))) < 0)
		return (warnarch("Corrupt swapped header",
		    (off_t) sizeof(binary) - (M_STRLEN - sizeof(ushort))));
	asb->sb_dev = (dev_t) swab(binary.b_dev);
	asb->sb_ino = (ino_t) swab(binary.b_ino);
	asb->sb_mode = swab(binary.b_mode);
	asb->sb_uid = swab(binary.b_uid);
	asb->sb_gid = swab(binary.b_gid);
	asb->sb_nlink = swab(binary.b_nlink);
	asb->sb_rdev = (dev_t) swab(binary.b_rdev);
	asb->sb_mtime = swab(binary.b_mtime[0]) << 16 | swab(binary.b_mtime[1]);
	asb->sb_size = swab(binary.b_size[0]) << 16 | swab(binary.b_size[1]);
	if ((namesize = swab(binary.b_name)) == 0 || namesize >= PATHSIZE)
		return (warnarch("Bad swapped pathname length",
		    (off_t) sizeof(binary) - (M_STRLEN - sizeof(ushort))));
	if (inread(name, namefull = namesize + namesize % 2) < 0)
		return (warnarch("Corrupt swapped pathname", (off_t) namefull));
	if (name[namesize - 1] != '\0')
		return (warnarch("Bad swapped pathname", (off_t) namefull));
	return (asb->sb_size % 2);
}

/*
 * lineget()
 *
 * Get a line from a given stream. Returns 0 if successful, -1 at EOF.
 */
STATIC int
lineget(stream, buf)
reg FILE	*stream;
reg char	*buf;
{
	reg int		c;

	for (;;) {
		if ((c = getc(stream)) == EOF)
			return (-1);
		if (c == '\n')
			break;
		*buf++ = c;
	}
	*buf = '\0';
	return (0);
}

/*
 * linkalso()
 *
 * Add a destination pathname to an existing chain. Assumes that
 * at least one element is present.
 */
STATIC void
linkalso(linkp, name)
reg Link	*linkp;
char		*name;
{
	reg Path	*path;

	if ((path = (Path *) memget(sizeof(Path))) == NULL
	    || (path->p_name = memstr(name)) == NULL)
		return;
	path->p_forw = NULL;
	path->p_back = linkp->l_path->p_back;
	path->p_back->p_forw = path;
	linkp->l_path->p_back = path;
}

/*
 * linkfrom()
 *
 * Find a file to link from. Returns a pointer to a link
 * structure, or NULL if unsuccessful.
 */
STATIC Link *
linkfrom(asb)
reg Stat	*asb;
{
	reg Link	*linkp;
	reg Link	*linknext;
	reg Path	*path;
	reg Path	*pathnext;
	reg Link	**abase;

	for (linkp = *(abase = linkhash(asb->sb_ino)); linkp; linkp = linknext)
		if (linkp->l_nlink == 0) {
			for (path = linkp->l_path; path; path = pathnext) {
				pathnext = path->p_forw;
				free(path->p_name);
			}
			free((char *) linkp->l_path);
			if (linknext = linkp->l_forw)
				linknext->l_back = linkp->l_back;
			if (linkp->l_back)
				linkp->l_back->l_forw = linkp->l_forw;
			else
				*abase = linkp->l_forw;
			free((char *) linkp);
		} else if (linkp->l_ino == asb->sb_ino
		    && linkp->l_dev == asb->sb_dev) {
			--linkp->l_nlink;
			return (linkp);
		} else
			linknext = linkp->l_forw;
	return (NULL);
}

/*
 * linkleft()
 *
 * Complain about files with unseen links.
 */
STATIC void
linkleft()
{
	reg Link	*lp;
	reg Link	**base;

	for (base = linkbase; base < linkbase + nel(linkbase); ++base)
		for (lp = *base; lp; lp = lp->l_forw)
			if (lp->l_nlink)
				VOID warn(lp->l_path->p_name, "Unseen link(s)");
}

/*
 * linkto()
 *
 * Remember a file with outstanding links. Returns a
 * pointer to the associated link structure, or NULL
 * when linking is not possible.
 */
STATIC Link *
linkto(name, asb)
char		*name;
reg Stat	*asb;
{
	reg Link	*linkp;
	reg Path	*path;
	reg Link	**abase;

	if ((asb->sb_mode & S_IFMT) == S_IFDIR
	    || (linkp = (Link *) memget(sizeof(Link))) == NULL
	    || (path = (Path *) memget(sizeof(Path))) == NULL
	    || (path->p_name = memstr(name)) == NULL)
		return (NULL);
	linkp->l_dev = asb->sb_dev;
	linkp->l_ino = asb->sb_ino;
	linkp->l_nlink = asb->sb_nlink - 1;
	linkp->l_size = asb->sb_size;
	linkp->l_path = path;
	path->p_forw = NULL;
	path->p_back = path;
	if (linkp->l_forw = *(abase = linkhash(asb->sb_ino)))
		linkp->l_forw->l_back = linkp;
	linkp->l_back = NULL;
	return (*abase = linkp);
}

#ifndef	MEMCPY

/*
 * memcpy()
 *
 * A simple block move.
 */
STATIC void
memcpy(to, from, len)
reg char	*to;
reg char	*from;
uint		len;
{
	reg char	*toend;

	for (toend = to + len; to < toend; *to++ = *from++)
		;
}

#endif	/* MEMCPY */

/*
 * memget()
 *
 * Allocate memory.
 */
STATIC char *
memget(len)
uint		len;
{
	reg char	*mem;
	static short	outofmem;

	if ((mem = malloc(len)) == NULL && !outofmem)
		outofmem = warn("memget()", "Out of memory");
	return (mem);
}

/*
 * memstr()
 *
 * Duplicate a string into dynamic memory.
 */
STATIC char *
memstr(str)
reg char	*str;
{
	reg char	*mem;

	if (mem = memget((uint) strlen(str) + 1))
		VOID strcpy(mem, str);
	return (mem);
}

#ifndef	MKDIR

/*
 * mkdir()
 *
 * Make a directory via "/bin/mkdir". Sets errno to a
 * questionably sane value upon failure.
 */
STATIC int
mkdir(name, mode)
reg char	*name;
reg ushort	mode;
{
	reg int		pid;

	if ((pid = xfork("mkdir()")) == 0) {
		VOID close(fileno(stdin));
		VOID close(fileno(stdout));
		VOID close(fileno(stderr));
		VOID open("/dev/null", O_RDWR);
		VOID dup(fileno(stdin));
		VOID dup(fileno(stdin));
		VOID umask(~mode);
		VOID execl("/bin/mkdir", "mkdir", name, (char *) NULL);
		exit(1);
	}
	if (xwait(pid, "mkdir()") == 0)
		return (0);
	errno = EACCES;
	return (-1);
}

#endif	/* MKDIR */

/*
 * nameadd()
 *
 * Add a name to the pattern list.
 */
STATIC void
nameadd(name, not)
reg char	*name;
int		not;
{
	reg Pattern	*px;

	px = (Pattern *) memget(sizeof(Pattern));
	px->p_forw = pattern;
	px->p_str = name;
	px->p_len = strlen(name);
	px->p_not = not;
	pattern = px;
}

/*
 * namecmp()
 *
 * Compare a pathname with the pattern list. Returns 0 for
 * a match, -1 otherwise.
 */
STATIC int
namecmp(name)
reg char	*name;
{
	reg Pattern	*px;
	reg int		positive;
	reg int		match;

	positive = match = 0;
	for (px = pattern; px; px = px->p_forw) {
		if (!px->p_not)
			++positive;
		if (strncmp(name, px->p_str, px->p_len) == 0
		  && (name[px->p_len] == '/' || name[px->p_len] == '\0')) {
			if (px->p_not)
				return (-1);
			++match;
		}
	}
	return (match || !positive ? 0 : -1);
}

/*
 * nameopt()
 *
 * Optimize a pathname. Confused by "<symlink>/.." twistiness.
 * Returns the number of final pathname elements (zero for "/"
 * or ".") or -1 if unsuccessful.
 */
STATIC int
nameopt(begin)
char	*begin;
{
	reg char	*name;
	reg char	*item;
	reg int		idx;
	int		absolute;
	auto char	*element[PATHELEM];

	absolute = (*(name = begin) == '/');
	idx = 0;
	for (;;) {
		if (idx == PATHELEM)
			return (warn(begin, "Too many elements"));
		while (*name == '/')
			++name;
		if (*name == '\0')
			break;
		element[idx] = item = name;
		while (*name && *name != '/')
			++name;
		if (*name)
			*name++ = '\0';
		if (strcmp(item, "..") == 0)
			if (idx == 0)
				if (absolute)
					;
				else
					++idx;
			else if (strcmp(element[idx - 1], "..") == 0)
				++idx;
			else
				--idx;
		else if (strcmp(item, ".") != 0)
			++idx;
	}
	if (idx == 0)
		element[idx++] = absolute ? "" : ".";
	element[idx] = NULL;
	name = begin;
	if (absolute)
		*name++ = '/';
	for (idx = 0; item = element[idx]; ++idx, *name++ = '/')
		while (*item)
			*name++ = *item++;
	*--name = '\0';
	return (idx);
}

/*
 * next()
 *
 * Advance to the next archive volume.
 */
STATIC void
next(mode, why)
reg int		mode;
reg char	*why;
{
	reg time_t	began;
	auto char	msg[200];
	auto char	answer[20];

	began = time((time_t *) NULL);
	nextclos();
	VOID warnarch(why, (off_t) 0);
	if (arfd == STDIN || arfd == STDOUT)
		exit(1);
	VOID sprintf(msg, "\
%s: Ready for volume %u on \"%s\"\n\
%s: Type \"go\" when ready to proceed (or \"quit\" to abort): \07",
	    myname, arvolume + 1, arspec, myname);
	for (;;) {
		nextask(msg, answer, sizeof(answer));
		if (strcmp(answer, "quit") == 0)
			fatal(arspec, "Aborted");
		if (strcmp(answer, "go") == 0 && nextopen(mode) == 0)
			break;
	}
	VOID warnarch("Continuing", (off_t) 0);
	timewait += time((time_t *) NULL) - began;
}

/*
 * nextask()
 *
 * Ask a question and get a response. Ignores spaces and tabs.
 */
STATIC void
nextask(msg, answer, limit)
reg char	*msg;
reg char	*answer;
reg int		limit;
{
	reg int		idx;
	reg int		got;
	auto char	c;

	if (ttyf < 0)
		fatal(TTY, "Unavailable");
	VOID write(ttyf, msg, (uint) strlen(msg));
	idx = 0;
	while ((got = read(ttyf, &c, 1)) == 1)
		if (c == '\04' || c == '\n')
			break;
		else if (c == ' ' || c == '\t')
			continue;
		else if (idx < limit - 1)
			answer[idx++] = c;
	if (got < 0)
		fatal(TTY, syserr());
	answer[idx] = '\0';
}

/*
 * nextclos()
 *
 * Close an archive.
 */
STATIC void
nextclos()
{
	if (arfd != STDIN && arfd != STDOUT)
		VOID close(arfd);
	areof = 0;
	if (arname && *arname == '!')
		pipewait();
}

/*
 * nextopen()
 *
 * Open an archive. Returns 0 if successful, -1 otherwise.
 */
STATIC int
nextopen(mode)
int		mode;
{
	if (*arname == '!')
		arfd = pipeopen(mode);
	else if (strcmp(arname, "-") == 0)
		arfd = mode ? STDOUT : STDIN;
	else {
#ifdef	CTC3B2
		if (Cflag) {
			reg int		oops;
			reg int		fd;

			oops = ((fd = open(arname, O_RDWR | O_CTSPECIAL)) < 0
			    || ioctl(fd, STREAMON) < 0);
			VOID close(fd);
			if (oops)
				return (warnarch(syserr(), (off_t) 0));
		}
#endif	/* CTC3B2 */
		arfd = mode ? creat(arname, 0666 & ~mask) : open(arname, mode);
	}
	if (arfd < 0)
		return (warnarch(syserr(), (off_t) 0));
	arleft = aruntil;
	++arvolume;
	return (0);
}

/*
 * openi()
 *
 * Open the next input file. Returns a file descriptor, 0 if no data
 * exists, or -1 at EOF. This kludge works because standard input is
 * in use, preventing open() from returning zero.
 */
STATIC int
openi(name, asb)
char		*name;
reg Stat	*asb;
{
	reg int		fd;
	auto char	local[PATHSIZE];

	for (;;) {
		if (lineget(stdin, name) < 0)
			return (-1);
		if (nameopt(name) < 0)
			continue;
		if (!gflag)
			VOID strcpy(local, name);
		else if (dirchg(name, local) < 0)
			continue;
		if ((hflag ? STAT(local, asb) : LSTAT(local, asb)) < 0) {
			VOID warn(name, syserr());
			continue;
		}
		switch (asb->sb_mode & S_IFMT) {
		case S_IFDIR:
			asb->sb_nlink = 1;
			asb->sb_size = 0;
			return (0);
#ifdef	S_IFLNK
		case S_IFLNK:
			if ((asb->sb_size = readlink(local,
			    asb->sb_link, sizeof(asb->sb_link) - 1)) < 0) {
				VOID warn(name, syserr());
				continue;
			}
			asb->sb_link[asb->sb_size] = '\0';
			return (0);
#endif	/* S_IFLNK */
		case S_IFREG:
			if (asb->sb_size == 0)
				return (0);
			if ((fd = open(local, O_RDONLY)) >= 0)
				return (fd);
			VOID warn(name, syserr());
			break;
		default:
			asb->sb_size = 0;
			return (0);
		}
	}
}

/*
 * openo()
 *
 * Open an output file. Returns the output file descriptor,
 * 0 if no data is required or -1 if unsuccessful. Note that
 * UNIX open() will never return 0 because the standard input
 * is in use.
 */
STATIC int
openo(name, asb, linkp, ispass)
char		*name;
reg Stat	*asb;
Link		*linkp;
reg int		ispass;
{
	reg int		exists;
	reg int		fd;
	reg ushort	perm;
	ushort		operm;
	Path		*path;
	auto Stat	osb;
#ifdef	S_IFLNK
	reg int		ssize;
	auto char	sname[PATHSIZE];
#endif	/* S_IFLNK */

	if (exists = (LSTAT(name, &osb) == 0))
		if (ispass
		    && osb.sb_ino == asb->sb_ino
		    && osb.sb_dev == asb->sb_dev)
			return (warn(name, "Same file"));
		else if ((osb.sb_mode & S_IFMT) == (asb->sb_mode & S_IFMT))
			operm = osb.sb_mode & (xflag ? S_IPERM : S_IPOPN);
		else if (remove(name, &osb) < 0)
			return (warn(name, syserr()));
		else
			exists = 0;
	if (linkp) {
		if (exists)
			if (asb->sb_ino == osb.sb_ino
			    && asb->sb_dev == osb.sb_dev)
				return (0);
			else if (unlink(name) < 0)
				return (warn(name, syserr()));
			else
				exists = 0;
		for (path = linkp->l_path; path; path = path->p_forw)
			if (link(path->p_name, name) == 0
			  || (errno == ENOENT
			    && dirneed(name) == 0
			    && link(path->p_name, name) == 0))
				return (0);
			else if (errno != EXDEV)
				return (warn(name, syserr()));
		VOID warn(name, "Link broken");
		linkalso(linkp, name);
	}
	perm = asb->sb_mode & (xflag ? S_IPERM : S_IPOPN);
	switch (asb->sb_mode & S_IFMT) {
	case S_IFBLK:
	case S_IFCHR:
		fd = 0;
		if (exists)
			if (asb->sb_rdev == osb.sb_rdev)
				if (perm != operm && chmod(name, perm) < 0)
					return (warn(name, syserr()));
				else
					break;
			else if (remove(name, &osb) < 0)
				return (warn(name, syserr()));
			else
				exists = 0;
		if (mknod(name, asb->sb_mode, asb->sb_rdev) < 0
		  && (errno != ENOENT
		    || dirneed(name) < 0
		    || mknod(name, asb->sb_mode, asb->sb_rdev) < 0))
			return (warn(name, syserr()));
		break;
	case S_IFDIR:
		if (exists)
			if (perm != operm && chmod(name, perm) < 0)
				return (warn(name, syserr()));
			else
				;
		else if (dirneed(name) < 0 || dirmake(name, asb) < 0)
			return (warn(name, syserr()));
		return (0);
#ifdef	S_IFIFO
	case S_IFIFO:
		fd = 0;
		if (exists)
			if (perm != operm && chmod(name, perm) < 0)
				return (warn(name, syserr()));
			else
				;
		else if (mknod(name, asb->sb_mode, (dev_t) 0) < 0
		  && (errno != ENOENT
		    || dirneed(name) < 0
		    || mknod(name, asb->sb_mode, (dev_t) 0) < 0))
			return (warn(name, syserr()));
		break;
#endif	/* S_IFIFO */
#ifdef	S_IFLNK
	case S_IFLNK:
		if (exists)
			if ((ssize = readlink(name, sname, sizeof(sname))) < 0)
				return (warn(name, syserr()));
			else if (strncmp(sname, asb->sb_link, ssize) == 0)
				return (0);
			else if (remove(name, &osb) < 0)
				return (warn(name, syserr()));
			else
				exists = 0;
		if (symlink(asb->sb_link, name) < 0
		  && (errno != ENOENT
		    || dirneed(name) < 0
		    || symlink(asb->sb_link, name) < 0))
			return (warn(name, syserr()));
		return (0);	/* Can't chown()/chmod() a symbolic link */
#endif	/* S_IFLNK */
	case S_IFREG:
		if (exists)
			if (nflag && osb.sb_mtime > asb->sb_mtime)
				return (warn(name, "Newer file exists"));
			else if (unlink(name) < 0)
				return (warn(name, syserr()));
			else
				exists = 0;
		if ((fd = creat(name, perm)) < 0
		  && (errno != ENOENT
		    || dirneed(name) < 0
		    || (fd = creat(name, perm)) < 0))
			return (warn(name, syserr()));
		break;
	default:
		return (warn(name, "Unknown filetype"));
	}
	if (xflag
	  && (!exists
	    || asb->sb_uid != osb.sb_uid
	    || asb->sb_gid != osb.sb_gid))
		VOID chown(name,
		    uid == 0 ? ush(asb->sb_uid) : uid,
		    ush(asb->sb_gid));
	if (linkp == NULL && asb->sb_nlink > 1)
		VOID linkto(name, asb);
	return (fd);
}

/*
 * openq()
 *
 * Open the terminal for interactive queries (sigh). Assumes that
 * background processes ignore interrupts and that the open() or
 * the isatty() will fail for processes which are not attached to
 * terminals. Returns a file descriptor (-1 if unsuccessful).
 */
STATIC int
openq()
{
	reg int		fd;
	reg VOIDFN	(*intr)();

	if ((intr = signal(SIGINT, SIG_IGN)) == SIG_IGN)
		return (-1);
	VOID signal(SIGINT, intr);
	if ((fd = open(TTY, O_RDWR)) < 0)
		return (-1);
	if (isatty(fd))
		return (fd);
	VOID close(fd);
	return (-1);
}

/*
 * options()
 *
 * Decode most reasonable forms of UNIX option syntax. Takes main()-
 * style argument indices (argc/argv) and a string of valid option
 * letters. Letters denoting options with arguments must be followed
 * by colons. With valid options, returns the option letter and points
 * "optarg" at the associated argument (if any). Returns '?' for bad
 * options and missing arguments. Returns zero when no options remain,
 * leaving "optind" indexing the first remaining argument.
 */
STATIC int
options(ac, av, proto)
int		ac;
register char	**av;
char		*proto;
{
	register int	c;
	register char	*idx;
	static int	optsub;

	if (optind == 0) {
		optind = 1;
		optsub = 0;
	}
	optarg = NULL;
	if (optind >= ac)
		return (0);
	if (optsub == 0 && (av[optind][0] != '-' || av[optind][1] == '\0'))
		return (0);
	switch (c = av[optind][++optsub]) {
	case '\0':
		++optind;
		optsub = 0;
		return (options(ac, av, proto));
	case '-':
		++optind;
		optsub = 0;
		return (0);
	case ':':
		return ('?');
	}
	if ((idx = strchr(proto, c)) == NULL)
		return ('?');
	if (idx[1] != ':')
		return (c);
	optarg = &av[optind][++optsub];
	++optind;
	optsub = 0;
	if (*optarg)
		return (c);
	if (optind >= ac)
		return ('?');
	optarg = av[optind++];
	return (c);
}

/*
 * optsize()
 *
 * Interpret a "size" argument. Recognizes suffices for blocks
 * (512-byte), kilobytes and megabytes and blocksize. Returns
 * the size in bytes.
 */
STATIC off_t
optsize(str)
char		*str;
{
	reg char	*idx;
	reg off_t	number;
	reg off_t	result;

	result = 0;
	idx = str;
	for (;;) {
		number = 0;
		while (*idx >= '0' && *idx <= '9')
			number = number * 10 + *idx++ - '0';
		switch (*idx++) {
		case 'b':
			result += number * 512;
			continue;
		case 'k':
			result += number * 1024;
			continue;
		case 'm':
			result += number * 1024 * 1024;
			continue;
		case 'x':
			result += number * arbsize;
			continue;
		case '+':
			result += number;
			continue;
		case '\0':
			result += number;
			break;
		default:
			break;
		}
		break;
	}
	if (*--idx)
		fatal(str, "Unrecognizable value");
	return (result);
}

/*
 * out()
 *
 * Write an archive.
 */
STATIC VOIDFN
out(av)
char		**av;
{
	reg int		fd;
	auto Stat	sb;
	auto char	name[PATHSIZE];

	if (*av)
		fatal(*av, "Extraneous argument");
	while ((fd = openi(name, &sb)) >= 0) {
		if (!lflag && sb.sb_nlink > 1)
			if (linkfrom(&sb))
				sb.sb_size = 0;
			else
				VOID linkto(name, &sb);
		outhead(name, &sb);
		if (fd)
			VOID close(outdata(fd, name, sb.sb_size));
		if (vflag)
			VOID fprintf(stderr, "%s\n", name);
	}
	outeof(TRAILER, TRAILZ);
}

/*
 * outalloc()
 *
 * Allocate buffer space previously referenced by outavail().
 */
STATIC void
outalloc(len)
reg uint	len;
{
	bufidx += len;
	total += len;
}

/*
 * outavail()
 *
 * Index buffer space for archive output. Stores a buffer pointer
 * at a given location. Returns the number of bytes available.
 */
STATIC uint
outavail(bufp)
reg char	**bufp;
{
	reg uint	have;

	while ((have = bufend - bufidx) == 0)
		outflush();
	*bufp = bufidx;
	return (have);
}

/*
 * outdata()
 *
 * Write archive data. Continues after file read errors, padding with
 * null characters if neccessary. Always returns the given input file
 * descriptor.
 */
STATIC int
outdata(fd, name, size)
int		fd;
char		*name;
reg off_t	size;
{
	reg uint	chunk;
	reg int		got;
	reg int		oops;
	reg uint	avail;
	auto char	*buf;

	oops = got = 0;
	while (size) {
		avail = outavail(&buf);
		size -= (chunk = size < avail ? (uint) size : avail);
		if (oops == 0 && (got = read(fd, buf, chunk)) < 0) {
			oops = warn(name, syserr());
			got = 0;
		}
		if (got < chunk) {
			if (oops == NULL)
				oops = warn(name, "Early EOF");
			while (got < chunk)
				buf[got++] = '\0';
		}
		outalloc(chunk);
	}
	return (fd);
}

/*
 * outeof()
 *
 * Write an archive trailer.
 */
STATIC void
outeof(name, namelen)
char		*name;
reg uint	namelen;
{
	reg off_t	pad;
	auto char	header[M_STRLEN + H_STRLEN + 1];

	if (pad = (total + M_STRLEN + H_STRLEN + namelen) % arpad)
		pad = arpad - pad;
	VOID strcpy(header, M_ASCII);
	VOID sprintf(header + M_STRLEN, H_PRINT, 0, 0,
	    0, 0, 0, 1, 0, (time_t) 0, namelen, pad);
	outwrite(header, M_STRLEN + H_STRLEN);
	outwrite(name, namelen);
	outpad(pad);
	outflush();
	if (fflag)
		outwait();
}

/*
 * outflush()
 *
 * Flush the output buffer. Optionally fork()s to allow the
 * parent to refill the buffer while the child waits for the
 * write() to complete.
 */
STATIC void
outflush()
{
	reg char	*buf;
	reg int		got;
	reg uint	len;

	if (aruntil && arleft == 0)
		next(O_WRONLY, "Output limit reached");
	if (fflag) {
		outwait();
		if ((outpid = xfork("outflush()")) == 0)
			VOID nice(-40);
	}
	if (!fflag || outpid == 0) {
		for (buf = buffer; len = bufidx - buf; ) {
			if ((got = write(arfd, buf,
			    *arname == '!' ? len : min(len, arbsize))) > 0) {
				buf += got;
				arleft -= got;
			} else if (fflag) {
				VOID warn(arspec, got < 0
				    ? syserr()
				    : "Apparently full");
				_exit(1);
			} else if (got < 0)
				fatal(arspec, syserr());
			else
				next(O_WRONLY, "Apparently full");
		}
	}
	if (fflag) {
		if (outpid == 0)
			_exit(0);
		else
			arleft -= bufidx - buffer;
	}
	bufend = (bufidx = buffer) + (aruntil ? min(buflen, arleft) : buflen);
}

/*
 * outhead()
 *
 * Write an archive header.
 */
STATIC void
outhead(name, asb)
reg char	*name;
reg Stat	*asb;
{
	reg uint	namelen;
	auto char	header[M_STRLEN + H_STRLEN + 1];

	if (name[0] == '/')
		if (name[1])
			++name;
		else
			name = ".";
	namelen = (uint) strlen(name) + 1;
	VOID strcpy(header, M_ASCII);
	VOID sprintf(header + M_STRLEN, H_PRINT, ush(asb->sb_dev),
	    ush(asb->sb_ino), ush(asb->sb_mode), ush(asb->sb_uid),
	    ush(asb->sb_gid), ush(asb->sb_nlink), ush(asb->sb_rdev),
	    mflag ? timenow : asb->sb_mtime, namelen, asb->sb_size);
	outwrite(header, M_STRLEN + H_STRLEN);
	outwrite(name, namelen);
#ifdef	S_IFLNK
	if ((asb->sb_mode & S_IFMT) == S_IFLNK)
		outwrite(asb->sb_link, (uint) asb->sb_size);
#endif	/* S_IFLNK */
}

/*
 * outpad()
 *
 * Pad the archive.
 */
STATIC void
outpad(pad)
reg off_t	pad;
{
	reg int		idx;
	reg int		len;

	while (pad) {
		if ((len = bufend - bufidx) > pad)
			len = pad;
		for (idx = 0; idx < len; ++idx)
			*bufidx++ = '\0';
		total += len;
		outflush();
		pad -= len;
	}
}

/*
 * outwait()
 *
 * Wait for the last background outflush() process (if any). The child
 * exit value is zero if successful, 255 if a write() returned zero or
 * the value of errno if a write() was unsuccessful.
 */
STATIC void
outwait()
{
	auto int	status;

	if (outpid == 0)
		return;
	status = xwait(outpid, "outwait()");
	outpid = 0;
	if (status)
		fatal(arspec, "Child error");
}

/*
 * outwrite()
 *
 * Write archive data.
 */
STATIC void
outwrite(idx, len)
reg char	*idx;
uint		len;
{
	reg uint	have;
	reg uint	want;
	reg char	*endx = idx + len;

	while (want = endx - idx) {
		while ((have = bufend - bufidx) == 0)
			outflush();
		if (have > want)
			have = want;
		memcpy(bufidx, idx, have);
		bufidx += have;
		idx += have;
		total += have;
	}
}

/*
 * pass()
 *
 * Copy within the filesystem.
 */
STATIC VOIDFN
pass(av)
reg char	**av;
{
	reg int		fd;
	reg char	**avx;
	auto Stat	sb;
	auto char	name[PATHSIZE];

	for (avx = av; *avx; ++avx) {
		if (gflag && **avx != '/')
			fatal(*avx, "Relative pathname");
		if (STAT(*avx, &sb) < 0)
			fatal(*avx, syserr());
		if ((sb.sb_mode & S_IFMT) != S_IFDIR)
			fatal(*avx, "Not a directory");
	}
	while ((fd = openi(name, &sb)) >= 0) {
		if (passitem(name, &sb, fd, av))
			VOID close(fd);
		if (vflag)
			VOID fprintf(stderr, "%s\n", name);
	}
}

/*
 * passdata()
 *
 * Copy data to one file. Doesn't believe in input file
 * descriptor zero (see description of kludge in openi()
 * comments). Closes the provided output file descriptor.
 */
STATIC void
passdata(from, ifd, to, ofd)
char		*from;
reg int		ifd;
char		*to;
reg int		ofd;
{
	reg int		got;
	reg int		sparse;
	auto char	block[FSBUF];

	if (ifd) {
		VOID lseek(ifd, (off_t) 0, 0);
		sparse = 0;
		while ((got = read(ifd, block, sizeof(block))) > 0
		    && (sparse = swrite(ofd, block, (uint) got)) >= 0)
			total += got;
		if (got)
			VOID warn(got < 0 ? from : to, syserr());
		else if (sparse > 0
		  && (lseek(ofd, (off_t) -sparse, 1) < 0
		    || write(ofd, block, (uint) sparse) != sparse))
			VOID warn(to, syserr());
	}
	VOID close(ofd);
}

/*
 * passitem()
 *
 * Copy one file. Returns given input file descriptor.
 */
STATIC int
passitem(from, asb, ifd, dir)
char		*from;
Stat		*asb;
reg int		ifd;
reg char	**dir;
{
	reg int		ofd;
	auto time_t	tstamp[2];
	auto char	to[PATHSIZE];

	while (*dir) {
		if (nameopt(strcat(strcat(strcpy(to, *dir++), "/"), from)) < 0)
			continue;
		if ((ofd = openo(to, asb,
		    lflag ? linkto(from, asb) : linkfrom(asb), 1)) < 0)
			continue;
		if (ofd > 0)
			passdata(from, ifd, to, ofd);
		tstamp[0] = tstamp[1] = mflag ? timenow : asb->sb_mtime;
		VOID utime(to, tstamp);
	}
	return (ifd);
}

/*
 * pipechld()
 *
 * Child portion of pipeline fork.
 */
STATIC int
pipechld(mode, pfd)
int		mode;
reg int		*pfd;
{
	reg char	**av;
	auto char	*arg[32];

	av = arg;
	if ((*av = getenv("SHELL")) && **av)
		++av;
	else
		*av++ = "/bin/sh";
	*av++ = "-c";
	*av++ = arname + 1;
	*av = NULL;
	if (mode) {
		VOID close(pfd[1]);
		VOID close(STDIN);
		VOID dup(pfd[0]);
		VOID close(pfd[0]);
		VOID close(STDOUT);
		VOID open("/dev/null", O_WRONLY);
	} else {
		VOID close(STDIN);
		VOID open("/dev/null", O_RDONLY);
		VOID close(pfd[0]);
		VOID close(STDOUT);
		VOID dup(pfd[1]);
		VOID close(pfd[1]);
	}
	if (ttyf >= 0)
		VOID close(ttyf);
	VOID execvp(arg[0], arg);
	VOID warn(arg[0], syserr());
	_exit(1);
}

/*
 * pipeopen()
 *
 * Open an archive via a pipeline. Returns a file
 * descriptor, or -1 if unsuccessful.
 */
STATIC int
pipeopen(mode)
reg int		mode;
{
	auto int	pfd[2];

	if (pipe(pfd) < 0)
		return (-1);
	if ((pipepid = xfork("pipeopen()")) == 0)
		pipechld(mode, pfd);
	if (mode) {
		VOID close(pfd[0]);
		return (pfd[1]);
	} else {
		VOID close(pfd[1]);
		return (pfd[0]);
	}
}

/*
 * pipewait()
 *
 * Await a pipeline.
 */
STATIC void
pipewait()
{
	reg int		status;

	if (pipepid == 0)
		return;
	status = xwait(pipepid, "pipewait()");
	pipepid = 0;
	if (status)
		fatal(arspec, "Pipeline error");
}

/*
 * prsize()
 *
 * Print a file offset.
 */
STATIC void
prsize(stream, size)
FILE		*stream;
reg off_t	size;
{
	reg off_t	n;

	if (n = (size / (1024 * 1024))) {
		VOID fprintf(stream, "%ldm+", n);
		size -= n * 1024 * 1024;
	}
	if (n = (size / 1024)) {
		VOID fprintf(stream, "%ldk+", n);
		size -= n * 1024;
	}
	VOID fprintf(stream, "%ld", size);
}

#ifndef	MKDIR

/*
 * rmdir()
 *
 * Remove a directory via "/bin/rmdir". Sets errno to a
 * questionably sane value upon failure.
 */
STATIC int
rmdir(name)
reg char	*name;
{
	reg int		pid;

	if ((pid = xfork("rmdir()")) == 0) {
		VOID close(fileno(stdin));
		VOID close(fileno(stdout));
		VOID close(fileno(stderr));
		VOID open("/dev/null", O_RDWR);
		VOID dup(fileno(stdin));
		VOID dup(fileno(stdin));
		VOID execl("/bin/rmdir", "rmdir", name, (char *) NULL);
		exit(1);
	}
	if (xwait(pid, "rmdir()") == 0)
		return (0);
	errno = EACCES;
	return (-1);
}

#endif	/* MKDIR */

/*
 * swrite()
 *
 * Write a filesystem block. Seeks past sparse blocks. Returns
 * 0 if the block was written, the given length for a sparse
 * block or -1 if unsuccessful.
 */
STATIC int
swrite(fd, buf, len)
int		fd;
char		*buf;
uint		len;
{
	reg char	*bidx;
	reg char	*bend;

	if (jflag)
		return (write(fd, buf, len) == len ? 0 : -1);
	bend = (bidx = buf) + len;
	while (bidx < bend)
		if (*bidx++)
			return (write(fd, buf, len) == len ? 0 : -1);
	return (lseek(fd, (off_t) len, 1) < 0 ? -1 : len);
}

/*
 * syserr()
 *
 * Return pointer to appropriate system error message.
 */
char *
syserr()
{

	return (strerror(errno));
}

/*
 * toc()
 *
 * Print archive table of contents.
 */
STATIC VOIDFN
toc(av)
reg char	**av;
{
	auto Stat	sb;
	auto char	name[PATHSIZE];

	if (*av)
		fatal(*av, "Extraneous argument");
	name[0] = '\0';
	while (inhead(name, &sb) == 0) {
		if (namecmp(name) == 0)
			tocentry(name, &sb);
		if (inskip(sb.sb_size) < 0)
			VOID warn(name, "File data is corrupt");
	}
}

/*
 * tocentry()
 *
 * Print a single table-of-contents entry.
 */
STATIC void
tocentry(name, asb)
char		*name;
reg Stat	*asb;
{
	reg Time	*atm;
	reg Link	*from;
	reg Passwd	*pwp;
	reg Group	*grp;
	static char	*month[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	if (vflag) {
		tocmode(asb->sb_mode);
		VOID printf(" %2d", asb->sb_nlink);
		atm = localtime(&asb->sb_mtime);
		if (pwp = getpwuid(ush(asb->sb_uid)))
			VOID printf(" %-8s", pwp->pw_name);
		else
			VOID printf(" %-8u", ush(asb->sb_uid));
		if (grp = getgrgid(ush(asb->sb_gid)))
			VOID printf(" %-8s", grp->gr_name);
		else
			VOID printf(" %-8u", ush(asb->sb_gid));
		switch (asb->sb_mode & S_IFMT) {
		case S_IFBLK:
		case S_IFCHR:
			VOID printf(" %3d, %3d",
			    major(asb->sb_rdev), minor(asb->sb_rdev));
			break;
		case S_IFREG:
			VOID printf(" %8ld", asb->sb_size);
			break;
		default:
			VOID printf("         ");
		}
		VOID printf(" %3s %2d %02d:%02d:%02d %4d ",
		    month[atm->tm_mon], atm->tm_mday, atm->tm_hour,
		    atm->tm_min, atm->tm_sec, atm->tm_year + 1900);
	}
	VOID printf("%s", name);
	if (vflag || lflag) {
		if (asb->sb_nlink > 1)
			if (from = linkfrom(asb))
				VOID printf(" -> %s",
				    from->l_path->p_name);
			else
				VOID linkto(name, asb);
#ifdef	S_IFLNK
		if ((asb->sb_mode & S_IFMT) == S_IFLNK)
			VOID printf(" S-> %s", asb->sb_link);
#endif	/* S_IFLNK */
	}
	putchar('\n');
}

/*
 * tocmode()
 *
 * Fancy file mode display.
 */
STATIC void
tocmode(mode)
reg ushort	mode;
{
	switch (mode & S_IFMT) {
		case S_IFREG: putchar('-'); break;
		case S_IFDIR: putchar('d'); break;
#ifdef	S_IFLNK
		case S_IFLNK: putchar('l'); break;
#endif	/* S_IFLNK */
		case S_IFBLK: putchar('b'); break;
		case S_IFCHR: putchar('c'); break;
#ifdef	S_IFIFO
		case S_IFIFO: putchar('p'); break;
#endif	/* S_IFIFO */
		default:
			VOID printf("[%o]", mode >> S_IFSHF);
	}
	putchar(mode & 0400 ? 'r' : '-');
	putchar(mode & 0200 ? 'w' : '-');
	putchar(mode & 0100
	    ? mode & 04000 ? 's' : 'x'
	    : mode & 04000 ? 'S' : '-');
	putchar(mode & 0040 ? 'r' : '-');
	putchar(mode & 0020 ? 'w' : '-');
	putchar(mode & 0010
	    ? mode & 02000 ? 's' : 'x'
	    : mode & 02000 ? 'S' : '-');
	putchar(mode & 0004 ? 'r' : '-');
	putchar(mode & 0002 ? 'w' : '-');
	putchar(mode & 0001
	    ? mode & 01000 ? 't' : 'x'
	    : mode & 01000 ? 'T' : '-');
}

/*
 * usage()
 *
 * Print a helpful message and exit.
 */
STATIC void
usage()
{
	VOID fprintf(stderr, "\
Usage:	%s -o [ -fghlmuvz ] [ -(bces) n ] archive\n\
	%s -i [ -djkmnuvxz ] [ -(bcs) n ] [ -y prefix ] archive\n\
	%s -t [ -kuvz ] [ -(bcs) n ] [ -y prefix ] archive\n\
	%s -p [ -dghjlmnuvxz ] dir [ ... ]\n",
	    myname, myname, myname, myname);
	exit(1);
}

/*
 * warn()
 *
 * Print a warning message. Always returns -1.
 */
STATIC int
warn(what, why)
char	*what;
char	*why;
{
	VOID fprintf(stderr,
	    "%s: \"%s\": %s\n",
	    myname, what, why);
	return (-1);
}

/*
 * warnarch()
 *
 * Print an archive-related warning message, including
 * an adjusted file offset. Always returns -1.
 */
STATIC int
warnarch(msg, adjust)
char	*msg;
off_t	adjust;
{
	VOID fprintf(stderr, "%s: \"%s\" [offset ", myname, arspec);
	prsize(stderr, total - adjust);
	VOID fprintf(stderr, "]: %s\n", msg);
	return (-1);
}

/*
 * xfork()
 *
 * Create a child.
 */
STATIC int
xfork(what)
reg char	*what;
{
	reg int		pid;
	reg Child	*cp;
	reg int		idx;
	static uint	delay[] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

	for (idx = 0; (pid = fork()) < 0; ++idx) {
		if (idx == sizeof(delay))
			fatal(arspec, syserr());
		VOID warn(what, "Trouble forking...");
		sleep(delay[idx]);
	}
	if (idx)
		VOID warn(what, "...successful fork");
	cp = (Child *) memget(sizeof(*cp));
	cp->c_pid = pid;
	cp->c_flags = 0;
	cp->c_status = 0;
	cp->c_forw = children;
	children = cp;
	return (pid);
}

/*
 * xpause()
 *
 * Await a child.
 */
STATIC void
xpause()
{
	reg Child	*cp;
	reg int		pid;
	auto int	status;

	do {
		while ((pid = wait(&status)) < 0)
			;
		for (cp = children; cp && cp->c_pid != pid; cp = cp->c_forw)
			;
	} while (cp == NULL);
	cp->c_flags |= CF_EXIT;
	cp->c_status = status;
}

/*
 * xwait()
 *
 * Find the status of a child.
 */
STATIC int
xwait(pid, what)
reg int		pid;
char		*what;
{
	reg int		status;
	reg Child	*cp;
	reg Child	**acp;
	auto char	why[100];

	for (acp = &children; cp = *acp; acp = &cp->c_forw)
		if (cp->c_pid == pid)
			break;
	if (cp == NULL)
		fatal(what, "Lost child");
	while ((cp->c_flags & CF_EXIT) == 0)
		xpause();
	status = cp->c_status;
	*acp = cp->c_forw;
	free((char *) cp);
	if (status == 0)
		return (0);
	if (status & 0377)
		VOID sprintf(why, "Killed by signal %d%s",
		    status & 0177, status & 0200 ? " -- core dumped" : "");
	else
		VOID sprintf(why, "Exit %d", (status >> 8) & 0377);
	return (warn(what, why));
}
