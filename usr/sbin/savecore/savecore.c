/*
 * savecore, 1.1 (2.11BSD) 1995/07/15
 */

#include	<sys/param.h>
#include	<stdio.h>
#include	<nlist.h>
#include	<sys/stat.h>
#include	<sys/fs.h>
#include	<sys/time.h>

#ifdef	pdp11
#define	CLICK	ctob(1)		/* size of core units */
#define	REGLOC	0300		/* offset of saved registers in disk dump */
#define	NREGS	8		/* r0-6, KA6 */
#endif

#define	DAY		(60L*60L*24L)
#define	LEEWAY		(3*DAY)

#define	eq(a,b)		(!strcmp(a,b))
#define	ok(number)	((off_t) ((number)&0177777))

#define	SHUTDOWNLOG	"/usr/adm/shutdownlog"

struct nlist nl[] = {
#define	X_DUMPDEV	0
	{ "_dumpdev" },
#define	X_DUMPLO	1
	{ "_dumplo" },
#define	X_TIME		2
	{ "_time" },
#define	X_PANICSTR	3
	{ "_panicstr" },
#define	X_PHYSMEM	4
	{ "_physmem" },
#define	X_BOOTIME	5
	{ "_boottime" },
#define	X_VERSION	6
	{ "_version" },
	{ 0 },
};

char	*system;
char	*dirname;			/* directory to save dumps in */
char	*ddname;			/* name of dump device */
char	*find_dev();
dev_t	dumpdev;			/* dump device */
time_t	dumptime;			/* time the dump was taken */
time_t	boottime;			/* time we were rebooted */
daddr_t	dumplo;				/* where dump starts on dumpdev */
size_t	physmem;			/* amount of memory in machine */
time_t	now;				/* current date */
char	*path();
extern	char	*malloc();
extern	char	*ctime();
char	vers[80];
char	core_vers[80];
char	panic_mesg[80];
int	panicstr;
extern	off_t	lseek();
extern	char	*devname();
off_t	Lseek();
int	debug;

main(argc, argv)
	char **argv;
	int argc;
{
	int setup;

	if ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == 'd')) {
		debug = 1;
		argc--;
		argv++;
	}
	if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage:  savecore [ -d ] dirname [ system ]\n");
		exit(1);
	}
	dirname = argv[1];
	if (argc == 3)
		system = argv[2];
	if (access(dirname, 2) < 0) {
		perror(dirname);
		exit(1);
	}
	(void) time(&now);
	setup = read_kmem();
	log_entry();
	if (setup && get_crashtime() && check_space())
		save_core();
	else
		exit(1);
}

char *
find_dev(dev, type)
	register dev_t dev;
	int type;
	{
	register char *dp, *cp;

	cp = devname(dev, type);
	if	(!cp)
		{
		if	(debug)
			fprintf(stderr, "Can't find device %d,%d\n",
				major(dev), minor(dev));
		return(NULL);
		}
	dp = (char *)malloc(strlen(cp) + 1 + sizeof ("/dev/"));
	(void)sprintf(dp, "/dev/%s", cp);
	return(dp);
	}

read_kmem()
{
	int kmem;
	FILE *fp;
	register char *cp;

	nlist("/unix", nl);
	if (nl[X_DUMPDEV].n_value == 0) {
		if (debug)
			fprintf(stderr, "/unix:  dumpdev not in namelist\n");
		exit(1);
	}
	if (nl[X_DUMPLO].n_value == 0) {
		fprintf(stderr, "/unix:  dumplo not in namelist\n");
		exit(1);
	}
	if (nl[X_TIME].n_value == 0) {
		fprintf(stderr, "/unix:  time not in namelist\n");
		exit(1);
	}
	if (nl[X_PHYSMEM].n_value == 0) {
		fprintf(stderr, "/unix:  physmem not in namelist\n");
		exit(1);
	}
	if (nl[X_VERSION].n_value == 0) {
		fprintf(stderr, "/unix:  version not in namelist\n");
		exit(1);
	}
	if (nl[X_PANICSTR].n_value == 0) {
		fprintf(stderr, "/unix:  panicstr not in namelist\n");
		exit(1);
	}
	kmem = Open("/dev/kmem", 0);
	Lseek(kmem, (off_t)nl[X_DUMPDEV].n_value, 0);
	Read(kmem, (char *)&dumpdev, sizeof dumpdev);
	if (dumpdev == NODEV) {
		if (debug)
			fprintf(stderr, "Dumpdev is NODEV\n");
		return(0);
	}
	Lseek(kmem, (off_t)nl[X_DUMPLO].n_value, 0);
	Read(kmem, (char *)&dumplo, sizeof dumplo);
	Lseek(kmem, (off_t)nl[X_PHYSMEM].n_value, 0);
	Read(kmem, (char *)&physmem, sizeof physmem);
	if (nl[X_BOOTIME].n_value != 0) {
		Lseek(kmem, (off_t)nl[X_BOOTIME].n_value, 0);
		Read(kmem, (char *)&boottime, sizeof boottime);
	}
	dumplo *= (long)NBPG;
	ddname = find_dev(dumpdev, S_IFBLK);
	if (ddname == NULL)
		return(0);
	if (system)
		return(1);
	if ((fp = fdopen(kmem, "r")) == NULL) {
		fprintf(stderr, "Couldn't fdopen kmem\n");
		exit(1);
	}
	fseek(fp, (long)nl[X_VERSION].n_value, 0);
	fgets(vers, sizeof vers, fp);
	fclose(fp);
	if ((fp = fopen(ddname, "r")) == NULL) {
		perror(ddname);
		exit(1);
	}
	fseek(fp, (off_t)(dumplo + ok(nl[X_VERSION].n_value)), 0);
	fgets(core_vers, sizeof core_vers, fp);
	fclose(fp);
	if (!eq(vers, core_vers)) {
		if (debug)
			fprintf(stderr, "unix version mismatch:\n\t%sand\n\t%s",
			    vers,core_vers);
		return(0);
	}
	fp = fopen(ddname, "r");
	fseek(fp, (off_t)(dumplo + ok(nl[X_PANICSTR].n_value)), 0);
	fread((char *)&panicstr, sizeof panicstr, 1, fp);
	if (panicstr) {
		fseek(fp, dumplo + ok(panicstr), 0);
		cp = panic_mesg;
		do
			*cp = getc(fp);
		while (*cp && (++cp < panic_mesg+sizeof(panic_mesg)));
	}
	fclose(fp);
	return(1);
}

get_crashtime()
{
	int dumpfd;
	time_t clobber = (time_t)0;

	if (dumpdev == NODEV)
		return (0);
	if (system)
		return (1);
	dumpfd = Open(ddname, 2);
	Lseek(dumpfd, (off_t)(dumplo + ok(nl[X_TIME].n_value)), 0);
	Read(dumpfd, (char *)&dumptime, sizeof dumptime);
	if (!debug) {
		Lseek(dumpfd, (off_t)(dumplo + ok(nl[X_TIME].n_value)), 0);
		Write(dumpfd, (char *)&clobber, sizeof clobber);
	}
	close(dumpfd);
	if (dumptime == 0) {
		if (debug)
			printf("dump time is 0\n");
		return (0);
	}
	printf("System went down at %s", ctime(&dumptime));
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
		printf("Dump time is unreasonable\n");
		return (0);
	}
	if (dumptime > now) {
		printf("Time was lost: was %s\n", ctime(&now));
		if (boottime != 0) {
			struct timeval	tp;
			now = now - boottime + dumptime;
			tp.tv_sec = now;
			tp.tv_usec = 0;
			if (settimeofday(&tp,(struct timezone *)NULL))
			    printf("\t-- resetting clock to %s\n", ctime(&now));
		}
	}
	return (1);
}

char *
path(file)
	char *file;
{
	register char *cp = malloc(strlen(file) + strlen(dirname) + 2);

	(void) strcpy(cp, dirname);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

check_space()
{
	struct stat dsb;
	register char *ddev;
	register int dfd;
	struct fs sblk;

	if (stat(dirname, &dsb) < 0) {
		perror(dirname);
		exit(1);
	}
	ddev = find_dev(dsb.st_dev, S_IFBLK);
	dfd = Open(ddev, 0);
	Lseek(dfd, (off_t)SUPERB*DEV_BSIZE, 0);
	Read(dfd, (char *)&sblk, sizeof sblk);
	close(dfd);
	if (read_number("minfree") > sblk.fs_tfree) {
		fprintf(stderr, "Dump omitted, not enough space on device\n");
		return (0);
	}
	return (1);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(path(fn), "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}

#define	BLOCK	32		/* buffer size, in clicks */

save_core()
{
	register int n;
	char buffer[BLOCK*CLICK];
	char *cp = buffer;
	register int ifd, ofd, bounds;
	FILE *fp;

	bounds = read_number("bounds");
	ifd = Open(system ?  system : "/unix", 0);
	(void)sprintf(cp, "unix.%d", bounds);
	ofd = Create(path(cp), 0666);
	while((n = Read(ifd, buffer, sizeof buffer)) > 0)
		Write(ofd, cp, n);
	close(ifd);
	close(ofd);
	ifd = Open(ddname, 0);
	(void)sprintf(cp, "core.%d", bounds);
	ofd = Create(path(cp), 0666);
	Lseek(ifd, (off_t)dumplo, 0);
	printf("Saving %D bytes of image in core.%d\n",
		(long)CLICK*physmem, bounds);
	while (physmem) {
		n = Read(ifd, cp, (physmem>BLOCK? BLOCK: physmem) * CLICK);
		if (n<0) {
			perror("Read");
			break;
		}
		Write(ofd, cp, n);
		physmem -= n/CLICK;
	}
	close(ifd);
	close(ofd);
	fp = fopen(path("bounds"), "w");
	fprintf(fp, "%d\n", bounds+1);
	fclose(fp);
}

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

log_entry()
{
	FILE *fp;
	struct tm *tm, *localtime();

	tm = localtime(&now);
	fp = fopen(SHUTDOWNLOG, "a");
	if (fp == 0)
		return;
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  Reboot", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	if (panicstr)
		fprintf(fp, " after panic:  %s\n", panic_mesg);
	else
		putc('\n', fp);
	fclose(fp);
}

/*
 * Versions of std routines that exit on error.
 */

Open(name, rw)
	char *name;
	int rw;
{
	int fd;

	if ((fd = open(name, rw)) < 0) {
		perror(name);
		exit(1);
	}
	return fd;
}

Read(fd, buff, size)
	int fd, size;
	char *buff;
{
	int ret;

	if ((ret = read(fd, buff, size)) < 0) {
		perror("read");
		exit(1);
	}
	return ret;
}

off_t
Lseek(fd, off, flag)
	int fd, flag;
	off_t off;
{
	off_t ret;

	if ((ret = lseek(fd, off, flag)) == -1L) {
		perror("lseek");
		exit(1);
	}
	return ret;
}

Create(file, mode)
	char *file;
	int mode;
{
	register int fd;

	if ((fd = creat(file, mode)) < 0) {
		perror(file);
		exit(1);
	}
	return fd;
}

Write(fd, buf, size)
	int fd, size;
	char *buf;

{

	if (write(fd, buf, size) < size) {
		perror("write");
		exit(1);
	}
}
