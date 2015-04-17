#include <stdio.h>


/* old <sys/param.h> */
#include <sys/types.h>
#define	BSIZE 512

/* old <sys/inode.h> */
/* modes */
#define	IFMT	0170000		/* type of file */
#define		IFDIR	0040000	/* directory */
#define		IFCHR	0020000	/* character special */
#define		IFMPC	0030000	/* multiplexed char special */
#define		IFBLK	0060000	/* block special */
#define		IFMPB	0070000	/* multiplexed block special */
#define		IFREG	0100000	/* regular */
#define		IFLNK	0120000	/* symbolic link */
#define		IFQUOT	0140000	/* quota */
#define	ISUID	04000		/* set user id on execution */
#define	ISGID	02000		/* set group id on execution */
#define	ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100

/* old <sys/ino.h> */
/*
 * Inode structure as it appears on
 * a disk block.
 */
struct dinode
{
	u_short	di_mode;     	/* mode and type of file */
	short	di_nlink;    	/* number of links to file */
	short	di_uid;      	/* owner's user id */
	short	di_gid;      	/* owner's group id */
	off_t	di_size;     	/* number of bytes in file */
	char  	di_addr[40];	/* disk block addresses */
	time_t	di_atime;   	/* time last accessed */
	time_t	di_mtime;   	/* time last modified */
	time_t	di_ctime;   	/* time created */
};

#define	INOPB	8	/* 8 inodes per block */
/*
 *	39 of the address bytes are used;
 *	13 addresses of 3 bytes each.
 */

/* old <sys/dir.h> */
#define	DIRSIZ	14

struct	direct
{
	ino_t	d_ino;
	char	d_name[DIRSIZ];
};

/* old <dumprestor.h> */
#define NTREC   	20
#define MLEN    	16
#define MSIZ    	4096

#define TS_TAPE 	1
#define TS_INODE	2
#define TS_BITS 	3
#define TS_ADDR 	4
#define TS_END  	5
#define TS_CLRI 	6
#define MAGIC   	(int)60011
#define CHECKSUM	(int)84446
struct	spcl
{
	int	c_type;
	time_t	c_date;
	time_t	c_ddate;
	int	c_volume;
	daddr_t	c_tapea;
	ino_t	c_inumber;
	int	c_magic;
	int	c_checksum;
	struct	dinode	c_dinode;
	int	c_count;
	char	c_addr[BSIZE];
} spcl;

struct	idates
{
	char	id_name[16];
	char	id_incno;
	time_t	id_ddate;
};

/* end of old includes */

#ifdef NONSEPARATE
#define MAXINO 1000
#else
#define MAXINO 2500
#endif
#define BITS	8
#define MAXXTR	60
#define NCACHE	3


#define	MWORD(m,i) (m[(unsigned)(i-1)/MLEN])
#define	MBIT(i)	(1<<((unsigned)(i-1)%MLEN))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

int	mt;
char	tapename[] = "/dev/rmt8";
char	*magtape = tapename;

daddr_t	seekpt;
int	ofile;
FILE	*df;
char	dirfile[] = "/tmp/rstXXXXXX";

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
char tbf[NTREC*BSIZE];

char prebuf[512];

int volno;

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
	printem(prebuf, (ino_t) 2);
	exit(0);
}
	i = 0;
/*
 * Read the tape, bulding up a directory structure for extraction
 * by name
 */
pass1()
{
	register i;
	struct dinode *ip;
	int	putdir(), null();

	while (gethead(&spcl) == 0) {
		printf("Can't find directory header!\n");
	}
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
			flsh();
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
		putent("\000\000/");
	}
}

printem(prefix, inum)
char *prefix;
ino_t	inum;
{
	struct direct dir;
	register int i;

	for (i = 0; i < MAXINO; i++)
		if (inotab[i].t_ino == inum) {
			goto found;
		}
	printf("PANIC - can't find directory %d\n", inum);
	return;
found:
	mseek(inotab[i].t_seekpt);
	for (;;) {
		getent((char *) &dir);
		if (direq(dir.d_name, "/"))
			return;
		if (search(dir.d_ino) != 0 && direq(dir.d_name, ".") == 0 && direq(dir.d_name, "..") == 0) {
			int len;
			FILE *tdf;

			tdf = df;
			df = fopen(dirfile, "r");
			len = strlen(prefix);
			strncat(prefix, dir.d_name, sizeof(dir.d_name));
			strcat(prefix, "/");
			printem(prefix, dir.d_ino);
			prefix[len] = '\0';
			fclose(df);
			df = tdf;
		}
		else
			if (BIT(dir.d_ino, dumpmap))
				printf("%5d	%s%-.14s\n", dir.d_ino, prefix, dir.d_name);
	}
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
	char buf[BSIZE];

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
				(*f1)(buf, size > BSIZE ? (long) BSIZE : size);
			}
			else {
				clearbuf(buf);
				(*f2)(buf, size > BSIZE ? (long) BSIZE : size);
			}
			if ((size -= BSIZE) <= 0) {
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
			((struct spcl *)&tbf[i*BSIZE])->c_magic = 0;
		bct = 0;
		if ((i = read(mt, tbf, NTREC*BSIZE)) < 0) {
			exit(1);
		}
		if (i == 0) {
			bct = NTREC + 1;
			volno++;
loop:
			flsht();
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
	copy(&tbf[(bct++*BSIZE)], b, BSIZE);
}

flsht()
{
	bct = NTREC+1;
}

copy(f, t, s)
register char *f, *t;
{
	register i;

	i = s;
	do
		*t++ = *f++;
	while (--i);
}

clearbuf(cp)
register char *cp;
{
	register i;

	i = BSIZE;
	do
		*cp++ = 0;
	while (--i);
}

/*
 * Put and get the directory entries from the compressed
 * directory file
 */
putent(cp)
char	*cp;
{
	register i;

	for (i = 0; i < sizeof(ino_t); i++)
		writec(*cp++);
	for (i = 0; i < DIRSIZ; i++) {
		writec(*cp);
		if (*cp++ == 0)
			return;
	}
	return;
}

getent(bf)
register char *bf;
{
	register i;

	for (i = 0; i < sizeof(ino_t); i++)
		*bf++ = readc();
	for (i = 0; i < DIRSIZ; i++)
		if ((*bf++ = readc()) == 0)
			return;
	return;
}

/*
 * read/write te directory file
 */
writec(c)
char c;
{
	seekpt++;
	fwrite(&c, 1, 1, df);
}

readc()
{
	char c;

	fread(&c, 1, 1, df);
	return(c);
}

mseek(pt)
daddr_t pt;
{
	fseek(df, pt, 0);
}

flsh()
{
	fflush(df);
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

direq(s1, s2)
register char *s1, *s2;
{
	register i;

	for (i = 0; i < DIRSIZ; i++)
		if (*s1++ == *s2) {
			if (*s2++ == 0)
				return(1);
		} else
			return(0);
	return(1);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
struct spcl *buf;
{
	readtape((char *)buf);
	if (buf->c_magic != MAGIC || checksum((int *) buf) == 0)
		return(0);
	return(1);
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

	j = BSIZE/sizeof(int);
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

putdir(b)
char *b;
{
	register struct direct *dp;
	register i;

	for (dp = (struct direct *) b, i = 0; i < BSIZE; dp++, i += sizeof(*dp)) {
		if (dp->d_ino == 0)
			continue;
		putent((char *) dp);
	}
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
		m += (BSIZE/(MLEN/BITS));
	}
	while (gethead(&spcl) == 0)
		;
}

null() { ; }
