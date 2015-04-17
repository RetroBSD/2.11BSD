#include <stdio.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/mtio.h>
#include <protocols/dumprestor.h>
#ifdef NONSEPARATE
#define MAXINO 1000
#else
#define MAXINO 3000
#endif
#define BITS	8
#define MAXXTR	60

#define	MWORD(m,i) (m[(unsigned)(i-1)/MLEN])
#define	MBIT(i)	(1<<((unsigned)(i-1)%MLEN))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

int	mt;
char	tapename[] = DEFTAPE;
char	*magtape = tapename;

daddr_t	seekpt;
int	ofile;
FILE	*df;
char	dirfile[] = "/tmp/rstXXXXXX";

#define ODIRSIZ 14
struct odirect
	{
	ino_t	d_ino;
	char	d_name[ODIRSIZ];
	};

struct {
	ino_t	t_ino;
	daddr_t	t_seekpt;
} inotab[MAXINO];
int	ipos;

#define ONTAPE	1
#define XTRACTD	2
#define XINUSE	4

short	dumpmap[MSIZ];
short	clrimap[MSIZ];


int bct = NTREC+1;
char tbf[NTREC*DEV_BSIZE];

char prebuf[MAXPATHLEN];

DIR drblock;
u_int prev;

int volno, cvtflag;

main(argc, argv)
char *argv[];
{
	extern char *ctime();

	mktemp(dirfile);
	argv++;
	if (argc>=3 && *argv[0] == 'f')
		magtape = *++argv;
	df = fopen(dirfile, "w");
	if (df == NULL) {
		printf("dumpdir: %s - cannot create directory temporary\n", dirfile);
		exit(1);
	}

	if ((mt = open(magtape, 0)) < 0) {
		printf("%s: cannot open tape\n", magtape);
		exit(1);
	}
	if (readhdr(&spcl) == 0) {
		printf("Tape is not a dump tape\n");
		exit(1);
	}
	printf("Dump   date: %s", ctime(&spcl.c_date));
	printf("Dumped from: %s", ctime(&spcl.c_ddate));
	if (checkvol(&spcl, 1) == 0) {
		printf("Tape is not volume 1 of the dump\n");
		exit(1);
	}
	pass1();  /* This sets the various maps on the way by */
	freopen(dirfile, "r", df);
	strcpy(prebuf, "/");
	printem(prebuf, (ino_t) ROOTINO);
	unlink(dirfile);
	exit(0);
}

/*
 * Read the tape, bulding up a directory structure for extraction
 * by name
 */
pass1()
{
	register i;
	struct dinode *ip;
	int	putdir(), null();
	struct direct nulldir;

	while (gethead(&spcl) == 0) {
		printf("Can't find directory header!\n");
	}
	nulldir.d_ino = 0;
	nulldir.d_namlen = 1;
	strcpy(nulldir.d_name, "/");
	nulldir.d_reclen = DIRSIZ(&nulldir);

	for (;;) {
		if (checktype(&spcl, TS_BITS) == 1) {
			readbits(dumpmap);
			continue;
		}
		if (checktype(&spcl, TS_CLRI) == 1) {
			readbits(clrimap);
			continue;
		}
		if (checktype(&spcl, TS_INODE) == 0) {
finish:
			fflush(df);
			close(mt);
			return;
		}
		ip = &spcl.c_dinode;
		i = ip->di_mode & IFMT;
		if (i != IFDIR) {
			goto finish;
		}
		inotab[ipos].t_ino = spcl.c_inumber;
		inotab[ipos++].t_seekpt = seekpt;
		getfile(spcl.c_inumber, putdir, null, spcl.c_dinode.di_size);
		putent(&nulldir);
		flushent();
	}
}

printem(prefix, inum)
char *prefix;
ino_t	inum;
{
	register struct direct *dp;
	register int i;
	struct direct *rddir();

	for (i = 0; i < MAXINO; i++)
		if (inotab[i].t_ino == inum) {
			goto found;
		}
	printf("PANIC - can't find directory %u\n", inum);
	return;
found:
	fseek(df, inotab[i].t_seekpt, 0);
	drblock.dd_loc = 0;
	for (;;) {
		dp = rddir();
		if (dp == NULL || dp->d_ino == 0)
			return;
		if (search((ino_t)dp->d_ino) && strcmp(dp->d_name, ".") && 
				strcmp(dp->d_name, "..")) {
			int len;
			off_t savpos;
			u_int savloc;

			savpos = ftell(df) - DIRBLKSIZ;
			savloc = drblock.dd_loc;
			len = strlen(prefix);
			strncat(prefix, dp->d_name, sizeof(dp->d_name));
			strcat(prefix, "/");
			printem(prefix, (ino_t)dp->d_ino);
			prefix[len] = '\0';
			fseek(df, savpos, 0);
			fread(drblock.dd_buf, DIRBLKSIZ, 1, df);
			drblock.dd_loc = savloc;
		}
		else
			if (BIT(dp->d_ino, dumpmap))
				printf("%5u	%s%-.14s\n", (ino_t)dp->d_ino, 
					prefix, dp->d_name);
	}
}

dcvt(odp, ndp)
register struct odirect *odp;
register struct direct *ndp;
{

	bzero(ndp, sizeof (struct direct));
	ndp->d_ino = odp->d_ino;
	strncpy(ndp->d_name, odp->d_name, ODIRSIZ);
	ndp->d_namlen = strlen(ndp->d_name);
	ndp->d_reclen = DIRSIZ(ndp);
}

/*
 * Do the file extraction, calling the supplied functions
 * with the blocks
 */
getfile(n, f1, f2, size)
ino_t	n;
int	(*f2)(), (*f1)();
long	size;
{
	register i;
	struct spcl addrblock;
	char buf[DEV_BSIZE];

	addrblock = spcl;
	goto start;
	for (;;) {
		if (gethead(&addrblock) == 0) {
			printf("Missing address (header) block\n");
			goto eloop;
		}
		if (checktype(&addrblock, TS_ADDR) == 0) {
			spcl = addrblock;
			return;
		}
start:
		for (i = 0; i < addrblock.c_count; i++) {
			if (addrblock.c_addr[i]) {
				readtape(buf);
				(*f1)(buf, size > DEV_BSIZE ? (long) DEV_BSIZE : size);
			}
			else {
				bzero(buf, DEV_BSIZE);
				(*f2)(buf, size > DEV_BSIZE ? (long) DEV_BSIZE : size);
			}
			if ((size -= DEV_BSIZE) <= 0) {
eloop:
				while (gethead(&spcl) == 0)
					;
				if (checktype(&spcl, TS_ADDR) == 1)
					goto eloop;
				return;
			}
		}
	}
}

/*
 * Do the tape i\/o, dealling with volume changes
 * etc..
 */
readtape(b)
char *b;
{
	register i;
	struct spcl tmpbuf;

	if (bct >= NTREC) {
		for (i = 0; i < NTREC; i++)
			((struct spcl *)&tbf[i*DEV_BSIZE])->c_magic = 0;
		bct = 0;
		if ((i = read(mt, tbf, NTREC*DEV_BSIZE)) < 0) {
			exit(1);
		}
		if (i == 0) {
			bct = NTREC + 1;
			volno++;
loop:
			bct = NTREC+1;
			close(mt);
			printf("Mount volume %d\n", volno);
			while (getchar() != '\n')
				;
			if ((mt = open(magtape, 0)) == -1) {
				printf("Cannot open tape!\n");
			}
			if (readhdr(&tmpbuf) == 0) {
				printf("Not a dump tape.Try again\n");
				goto loop;
			}
			if (checkvol(&tmpbuf, volno) == 0) {
				printf("Wrong tape. Try again\n");
				goto loop;
			}
			readtape(b);
			return;
		}
	}
	bcopy(&tbf[(bct++*DEV_BSIZE)], b, DEV_BSIZE);
}

putdir(b)
char *b;
{
	register struct direct *dp;
	struct direct cvtbuf;
	struct odirect *odp, *eodp;
	u_int	loc;
	register int i;

	if (cvtflag) {
		eodp = (struct odirect *)&b[DEV_BSIZE];
		for (odp = (struct odirect *)b; odp < eodp; odp++) {
			if (odp->d_ino) {
				dcvt(odp, &cvtbuf);
				putent(&cvtbuf);
			}
		}
	return;
	}

	for (loc = 0; loc < DEV_BSIZE; ) {
		dp = (struct direct *)(b + loc);
		i = DIRBLKSIZ - (loc & (DIRBLKSIZ - 1));
		if (dp->d_reclen == 0 || dp->d_reclen > i) {
			loc += i;
			continue;
		}
		loc += dp->d_reclen;
		if (dp->d_ino)
			putent(dp);
	}
}

putent(dp)
register struct direct *dp;
{

	dp->d_reclen = DIRSIZ(dp);
	if (drblock.dd_loc + dp->d_reclen > DIRBLKSIZ) {
		((struct direct *)(drblock.dd_buf + prev))->d_reclen = 
			DIRBLKSIZ - prev;
		fwrite(drblock.dd_buf, DIRBLKSIZ, 1, df);
		drblock.dd_loc = 0;
	}
	bcopy(dp, drblock.dd_buf + drblock.dd_loc, dp->d_reclen);
	prev = drblock.dd_loc;
	drblock.dd_loc += dp->d_reclen;
}

flushent()
{

	((struct direct *)(drblock.dd_buf + prev))->d_reclen =DIRBLKSIZ - prev;
	fwrite(drblock.dd_buf, DIRBLKSIZ, 1, df);
	prev = drblock.dd_loc = 0;
	seekpt = ftell(df);
}

struct direct *
rddir()
{
register struct direct *dp;

	for (;;) {
		if (drblock.dd_loc == 0) {
			drblock.dd_size = fread(drblock.dd_buf, 1,DIRBLKSIZ,df);
			if (drblock.dd_size <= 0) {
				printf("error reading directory\n");
				return(NULL);
			}
		}
		if (drblock.dd_loc >= drblock.dd_size) {
			drblock.dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(drblock.dd_buf + drblock.dd_loc);
		if (dp->d_reclen == 0 ||
			dp->d_reclen > DIRBLKSIZ + 1 - drblock.dd_loc) {
				printf("corrupted directory: bad reclen %d\n",
					dp->d_reclen);
				return(NULL);
		}
		drblock.dd_loc += dp->d_reclen;
		if (dp->d_ino == 0 && strcmp(dp->d_name, "/"))
			continue;
	return(dp);
	}
}

/*
 * search the directory inode ino
 * looking for entry cp
 */
search(inum)
ino_t	inum;
{
	register low, high, probe;

	low = 0;
	high = ipos-1;

	while (low != high) {
		probe = (high - low + 1)/2 + low;
/*
printf("low = %d, high = %d, probe = %d, ino = %d, inum = %d\n", low, high, probe, inum, inotab[probe].t_ino);
*/
		if (inum >= inotab[probe].t_ino)
			low = probe;
		else
			high = probe - 1;
	}
	return(inum == inotab[low].t_ino);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
struct spcl *buf;
{
	readtape((char *)buf);
	return(ishead(buf));
}

ishead(buf)
register struct spcl *buf;
{
register int ret = 0;

	if (buf->c_magic == OFS_MAGIC) {
		if (cvtflag == 0)
			printf("Convert old direct format to new\n");
		ret = cvtflag = 1;
	}
	else if (buf->c_magic == NFS_MAGIC) {
		if (cvtflag)
			printf("Was converting old direct format, not now\n");
		cvtflag = 0;
		ret = 1;
	}
	if (ret == 0)
		return(ret);
	return(checksum((int *) buf));
}

/*
 * return whether or not the buffer contains a header block
 */
checktype(b, t)
struct	spcl *b;
int	t;
{
	return(b->c_type == t);
}


checksum(b)
int *b;
{
	register i, j;

	j = DEV_BSIZE/sizeof(int);
	i = 0;
	do
		i += *b++;
	while (--j);
	if (i != CHECKSUM) {
		printf("Checksum error %o\n", i);
		return(0);
	}
	return(1);
}

checkvol(b, t)
struct spcl *b;
int t;
{
	if (b->c_volume == t)
		return(1);
	return(0);
}

readhdr(b)
struct	spcl *b;
{
	if (gethead(b) == 0)
		return(0);
	if (checktype(b, TS_TAPE) == 0)
		return(0);
	return(1);
}

/*
 * read a bit mask from the tape into m.
 */
readbits(m)
short	*m;
{
	register i;

	i = spcl.c_count;

	while (i--) {
		readtape((char *) m);
		m += (DEV_BSIZE/(MLEN/BITS));
	}
	while (gethead(&spcl) == 0)
		;
}

null() { ; }
