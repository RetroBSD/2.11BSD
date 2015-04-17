/*
 * 1995/06/09 - standalone restor must be loaded split I/D because the
 *		disklabel handling increased the size of standalone programs.
 *		This is not too much of a problem since the Kernel has been
 *		required to be split I/D for several years.
 *
 *		Since split I/D is now required the NCACHE parameter is no
 *		longer ifdef'd on STANDALONE (and is always 3 instead of 1).
 */

#include <sys/param.h>
#ifdef	NONSEPARATE
#define MAXINO	1000
#else
#define MAXINO	3000
#endif
#define BITS	8
#define MAXXTR	60
#define NCACHE	3
#define	flsht()	(bct = NTREC + 1)

#ifndef STANDALONE
#include <stdio.h>
#include <signal.h>
#endif
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <protocols/dumprestor.h>

#define	MWORD(m,i) (m[(unsigned)(i-1)/MLEN])
#define	MBIT(i)	(1<<((unsigned)(i-1)%MLEN))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

#define	ODIRSIZ 14
struct odirect {
	ino_t d_ino;
	char  d_name[ODIRSIZ];
	};

struct	fs	sblock;

int	fi;
ino_t	ino, maxi, curino;

int	mt;
char	tapename[] = "/dev/rmt1";
char	*magtape = tapename;
#ifdef STANDALONE
char	mbuf[50];
char	module[] = "Restor";
#endif

#ifndef STANDALONE
daddr_t	seekpt;
u_int	prev;
int	df, ofile;
char	dirfile[] = "rstXXXXXX";
struct 	direct *rddir();

struct {
	ino_t	t_ino;
	daddr_t	t_seekpt;
} inotab[MAXINO];
int	ipos;

#define ONTAPE	1
#define XTRACTD	2
#define XINUSE	4
struct xtrlist {
	ino_t	x_ino;
	char	x_flags;
} xtrlist[MAXXTR];

char	name[12];

DIR	drblock;
#endif

u_char	eflag, cvtflag, isdir, volno = 1;

struct dinode tino, dino;
daddr_t	taddr[NADDR];

daddr_t	curbno;

#ifndef	STANDALONE
short	dumpmap[MSIZ];
#endif
short	clrimap[MSIZ];


int bct = NTREC+1;
char tbf[NTREC*DEV_BSIZE];

struct	cache {
	daddr_t	c_bno;
	int	c_time;
	char	c_block[DEV_BSIZE];
} cache[NCACHE];
int	curcache;

extern  long lseek();

main(argc, argv)
register char *argv[];
{
	register char *cp;
	char command;
	int done();

#ifndef STANDALONE
	mktemp(dirfile);
	if (argc < 2) {
usage:
		printf("Usage: restor x file file..., restor r filesys, or restor t\n");
		exit(1);
	}
	argv++;
	argc -= 2;
	for (cp = *argv++; *cp; cp++) {
		switch (*cp) {
		case '-':
			break;
		case 'c':
			cvtflag = 1;
			break;
		case 'f':
			magtape = *argv++;
			argc--;
			break;
		case 'r':
		case 'R':
		case 't':
		case 'x':
			command = *cp;
			break;
		default:
			printf("Bad key character %c\n", *cp);
			goto usage;
		}
	}
	if (command == 'x') {
		if (signal(SIGINT, done) == SIG_IGN)
			signal(SIGINT, SIG_IGN);
		if (signal(SIGTERM, done) == SIG_IGN)
			signal(SIGTERM, SIG_IGN);

		df = open(dirfile, O_CREAT | O_TRUNC| O_RDWR, 0666);
		if (df < 0) {
			printf("restor: %s - cannot create directory temporary\n", dirfile);
			exit(1);
		}
	}
	doit(command, argc, argv);
	if (command == 'x')
		unlink(dirfile);
	exit(0);
#else
	printf("%s\n",module);
	magtape = "tape";
	doit('r', 1, 0);
#endif
}

doit(command, argc, argv)
char	command;
int	argc;
char	*argv[];
{
	extern char *ctime();
	register i, k;
	ino_t	d;
#ifndef STANDALONE
	int	xtrfile(), skip(), null();
#endif
	int	rstrfile(), rstrskip();
	register struct dinode *ip, *ip1;

#ifndef STANDALONE
	if ((mt = open(magtape, 0)) < 0) {
		printf("%s: cannot open tape\n", magtape);
		exit(1);
	}
#else
	do {
		printf("Tape? ");
		gets(mbuf);
		mt = open(mbuf, 0);
	} while (mt == -1);
	magtape = mbuf;
#endif
	switch(command) {
#ifndef STANDALONE
	case 't':
		if (readhdr(&spcl) == 0) {
			printf("Tape is not a dump tape\n");
			exit(1);
		}
		printf("Dump   date: %s", ctime(&spcl.c_date));
		printf("Dumped from: %s", ctime(&spcl.c_ddate));
		return;
	case 'x':
		if (readhdr(&spcl) == 0) {
			printf("Tape is not a dump tape\n");
			unlink(dirfile);
			exit(1);
		}
		if (checkvol(&spcl, 1) == 0) {
			printf("Tape is not volume 1 of the dump\n");
			unlink(dirfile);
			exit(1);
		}
		pass1();  /* This sets the various maps on the way by */
		i = 0;
		while (i < MAXXTR-1 && argc--) {
			if ((d = psearch(*argv)) == 0 || BIT(d, dumpmap) == 0) {
				printf("%s: not on the tape\n", *argv++);
				continue;
			}
			xtrlist[i].x_ino = d;
			xtrlist[i].x_flags |= XINUSE;
			printf("%s: inode %u\n", *argv, d);
			argv++;
			i++;
		}
newvol:
		flsht();
		close(mt);
getvol:
		printf("Mount desired tape volume: Specify volume #: ");
		if (gets(tbf) == NULL)
			return;
		volno = atoi(tbf);
		if (volno <= 0) {
			printf("Volume numbers are positive numerics\n");
			goto getvol;
		}
		mt = open(magtape, 0);
		if (readhdr(&spcl) == 0) {
			printf("tape is not dump tape\n");
			goto newvol;
		}
		if (checkvol(&spcl, volno) == 0) {
			printf("Wrong volume (%d)\n", spcl.c_volume);
			goto newvol;
		}
rbits:
		while (gethead(&spcl) == 0)
			;
		if (checktype(&spcl, TS_INODE) == 1) {
			printf("Can't find inode mask!\n");
			goto newvol;
		}
		if (checktype(&spcl, TS_BITS) == 0)
			goto rbits;
		readbits(dumpmap);
		i = 0;
		for (k = 0; xtrlist[k].x_flags; k++) {
			if (BIT(xtrlist[k].x_ino, dumpmap)) {
				xtrlist[k].x_flags |= ONTAPE;
				i++;
			}
		}
		while (i > 0) {
again:
			if (ishead(&spcl) == 0)
				while(gethead(&spcl) == 0)
					;
			if (checktype(&spcl, TS_END) == 1) {
				printf("end of tape\n");
checkdone:
				for (k = 0; xtrlist[k].x_flags; k++)
					if ((xtrlist[k].x_flags&XTRACTD) == 0)
						goto newvol;
					return;
			}
			if (checktype(&spcl, TS_INODE) == 0) {
				gethead(&spcl);
				goto again;
			}
			d = spcl.c_inumber;
			for (k = 0; xtrlist[k].x_flags; k++) {
				if (d == xtrlist[k].x_ino) {
					printf("extract file %u\n", xtrlist[k].x_ino);
					sprintf(name, "%u", xtrlist[k].x_ino);
					if ((ofile = creat(name, 0666)) < 0) {
						printf("%s: cannot create file\n", name);
						i--;
						continue;
					}
					fchown(ofile, spcl.c_dinode.di_uid, spcl.c_dinode.di_gid);
					getfile(d, xtrfile, skip, spcl.c_dinode.di_size);
					i--;
					xtrlist[k].x_flags |= XTRACTD;
					close(ofile);
					goto done;
				}
			}
			getfile(d, null, null, spcl.c_dinode.di_size);
done:
			;
		}
		goto checkdone;
#endif
	case 'r':
	case 'R':
#ifndef STANDALONE
		if ((fi = open(*argv, 2)) < 0) {
			printf("%s: cannot open\n", *argv);
			exit(1);
		}
#else
		do {
			char charbuf[50];

			printf("Disk? ");
			gets(charbuf);
			fi = open(charbuf, 2);
		} while (fi == -1);
#endif
#ifndef STANDALONE
		if (command == 'R') {
			printf("Enter starting volume number: ");
			if (gets(tbf) == EOF) {
				volno = 1;
				printf("\n");
			}
			else
				volno = atoi(tbf);
		}
		else
#endif
			volno = 1;
		printf("Last chance before scribbling on %s. ",
#ifdef STANDALONE
								"disk");
#else
								*argv);
#endif
		while (getchar() != '\n');
		dread((daddr_t)SBLOCK, (char *)&sblock, sizeof(sblock));
		maxi = (sblock.fs_isize-2)*INOPB;
		if (readhdr(&spcl) == 0) {
			printf("Missing volume record\n");
			exit(1);
		}
		if (checkvol(&spcl, volno) == 0) {
			printf("Tape is not volume %d\n", volno);
			exit(1);
		}
		gethead(&spcl);
		for (;;) {
ragain:
			if (ishead(&spcl) == 0) {
				printf("Missing header block\n");
				while (gethead(&spcl) == 0)
					;
				eflag++;
			}
			if (checktype(&spcl, TS_END) == 1) {
				printf("End of tape\n");
				close(mt);
				dwrite((daddr_t) SBLOCK, (char *) &sblock);
				return;
			}
			if (checktype(&spcl, TS_CLRI) == 1) {
				readbits(clrimap);
				for (ino = 1; ino <= maxi; ino++)
					if (BIT(ino, clrimap) == 0) {
						getdino(ino, &tino);
						if (tino.di_mode == 0)
							continue;
						itrunc(&tino);
						clri(&tino);
						putdino(ino, &tino);
					}
				dwrite((daddr_t) SBLOCK, (char *) &sblock);
				goto ragain;
			}
			if (checktype(&spcl, TS_BITS) == 1) {
#ifndef STANDALONE
				readbits(dumpmap);
#else
				readbits(0);
#endif
				goto ragain;
			}
			if (checktype(&spcl, TS_INODE) == 0) {
				printf("Unknown header type\n");
				eflag++;
				gethead(&spcl);
				goto ragain;
			}
			ino = spcl.c_inumber;
			if (eflag)
				printf("Resynced at inode %u\n", ino);
			eflag = 0;
			if (ino > maxi) {
				printf("%u: ilist too small\n", ino);
				gethead(&spcl);
				goto ragain;
			}
			dino = spcl.c_dinode;
			getdino(ino, &tino);
			curbno = 0;
			itrunc(&tino);
			clri(&tino);
			bzero(taddr, NADDR * sizeof (daddr_t));
			if (cvtflag)
				futz(dino.di_addr);
			taddr[0] = dino.di_addr[0];
			isdir = ((dino.di_mode & IFMT) == IFDIR);
			getfile(ino, rstrfile, rstrskip, dino.di_size);
			ip = &tino;
			ip1 = &dino;
			ip->di_mode = ip1->di_mode;
			ip->di_nlink = ip1->di_nlink;
			ip->di_uid = ip1->di_uid;
			ip->di_gid = ip1->di_gid;
			if (cvtflag && isdir)
				ip->di_size = curbno * DEV_BSIZE; /* XXX */
			else
				ip->di_size = ip1->di_size;
			ip->di_atime = ip1->di_atime;
			ip->di_mtime = ip1->di_mtime;
			ip->di_ctime = ip1->di_ctime;
			bcopy(taddr, ip->di_addr, NADDR * sizeof(daddr_t));
			putdino(ino, &tino);
			isdir = 0;
		}
	}
}

/*
 * Read the tape, bulding up a directory structure for extraction
 * by name
 */
#ifndef STANDALONE
pass1()
{
	register i;
	struct dinode *ip;
	struct direct nulldir;
	int	putdir(), null();

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
#endif

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
	curino = n;
	goto start;
	for (;;) {
		if (gethead(&addrblock) == 0) {
			printf("Missing address (header) block\n");
			goto eloop;
		}
		if (checktype(&addrblock, TS_ADDR) == 0) {
			spcl = addrblock;
			curino = 0;
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
				curino = 0;
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
			printf("Tape read error: inode %u\n", curino);
			eflag++;
			for (i = 0; i < NTREC; i++)
				bzero(&tbf[i*DEV_BSIZE], DEV_BSIZE);
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
				goto loop;
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
#ifdef	STANDALONE
	if(b)
#endif
	bcopy(&tbf[(bct++*DEV_BSIZE)], b, DEV_BSIZE);
}

#ifndef	STANDALONE
/*
 * search the directory inode ino
 * looking for entry cp
 */
ino_t
search(inum, cp)
ino_t	inum;
char	*cp;
{
	register i, len;
	register struct direct *dp;

	for (i = 0; i < MAXINO; i++)
		if (inotab[i].t_ino == inum) {
			goto found;
		}
	return(0);
found:
	lseek(df, inotab[i].t_seekpt, 0);
	drblock.dd_loc = 0;
	len = strlen(cp);
	do {
		dp = rddir();
		if (dp == NULL || dp->d_ino == 0)
			return(0);
	} while (dp->d_namlen != len || strncmp(dp->d_name, cp, len) != 0);
	return((ino_t)dp->d_ino);
}

/*
 * Search the directory tree rooted at inode 2
 * for the path pointed at by n
 */
psearch(n)
char	*n;
{
	register char *cp, *cp1;
	register char c;

	ino = ROOTINO;
	if (*(cp = n) == '/')
		cp++;
next:
	cp1 = cp + 1;
	while (*cp1 != '/' && *cp1)
		cp1++;
	c = *cp1;
	*cp1 = 0;
	ino = search(ino, cp);
	if (ino == 0) {
		*cp1 = c;
		return(0);
	}
	*cp1 = c;
	if (c == '/') {
		cp = cp1+1;
		goto next;
	}
	return(ino);
}
#endif	STANDALONE

/*
 * read/write a disk block, be sure to update the buffer
 * cache if needed.
 */
dwrite(bno, b)
daddr_t	bno;
char	*b;
{
	register i;

	for (i = 0; i < NCACHE; i++) {
		if (cache[i].c_bno == bno) {
			bcopy(b, cache[i].c_block, DEV_BSIZE);
			cache[i].c_time = 0;
			break;
		}
		else
			cache[i].c_time++;
	}
	lseek(fi, bno*DEV_BSIZE, 0);
	if(write(fi, b, DEV_BSIZE) != DEV_BSIZE) {
#ifdef STANDALONE
		printf("disk write error:  block %D\n", bno);
#else
		fprintf(stderr, "disk write error:  block %ld\n", bno);
#endif
		exit(1);
	}
}

dread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
	register i, j;

	j = 0;
	for (i = 0; i < NCACHE; i++) {
		if (++curcache >= NCACHE)
			curcache = 0;
		if (cache[curcache].c_bno == bno) {
			bcopy(cache[curcache].c_block, buf, cnt);
			cache[curcache].c_time = 0;
			return;
		}
		else {
			cache[curcache].c_time++;
			if (cache[j].c_time < cache[curcache].c_time)
				j = curcache;
		}
	}

	lseek(fi, bno*DEV_BSIZE, 0);
	if (read(fi, cache[j].c_block, DEV_BSIZE) != DEV_BSIZE) {
#ifdef STANDALONE
		printf("read error:  block %D\n", bno);
#else
		printf("read error:  block %ld\n", bno);
#endif
		exit(1);
	}
	bcopy(cache[j].c_block, buf, cnt);
	cache[j].c_time = 0;
	cache[j].c_bno = bno;
}

/*
 * the inode manpulation routines. Like the system.
 *
 * clri zeros the inode
 */
clri(ip)
register struct dinode *ip;
{
	if (ip->di_mode&IFMT)
		sblock.fs_tinode++;
	bzero(ip, sizeof (*ip));
}

/*
 * itrunc/tloop/bfree free all of the blocks pointed at by the inode
 */
itrunc(ip)
register struct dinode *ip;
{
	register i;
	daddr_t bn;

	if (ip->di_mode == 0)
		return;
	i = ip->di_mode & IFMT;
	if (i != IFDIR && i != IFREG)
		return;
	for(i=NADDR-1;i>=0;i--) {
		bn = ip->di_addr[i];
		if(bn == 0) continue;
		switch(i) {

		default:
			bfree(bn);
			break;

		case NADDR-3:
			tloop(bn, 0, 0);
			break;

		case NADDR-2:
			tloop(bn, 1, 0);
			break;

		case NADDR-1:
			tloop(bn, 1, 1);
		}
	}
	ip->di_size = 0;
}

tloop(bn, f1, f2)
daddr_t	bn;
int	f1, f2;
{
	register i;
	daddr_t nb;
	union {
		char	data[DEV_BSIZE];
		daddr_t	indir[NINDIR];
	} ibuf;

	dread(bn, ibuf.data, DEV_BSIZE);
	for(i=NINDIR-1;i>=0;i--) {
		nb = ibuf.indir[i];
		if(nb) {
			if(f1)
				tloop(nb, f2, 0);
			else
				bfree(nb);
		}
	}
	bfree(bn);
}

bfree(bn)
daddr_t	bn;
{
	register i;
	union {
		char	data[DEV_BSIZE];
		struct	fblk frees;
	} fbuf;

	if(sblock.fs_nfree >= NICFREE) {
		fbuf.frees.df_nfree = sblock.fs_nfree;
		for(i=0;i<NICFREE;i++)
			fbuf.frees.df_free[i] = sblock.fs_free[i];
		sblock.fs_nfree = 0;
		dwrite(bn, fbuf.data);
	}
	sblock.fs_free[sblock.fs_nfree++] = bn;
	sblock.fs_tfree++;
}

/*
 * allocate a block off the free list.
 */
daddr_t
balloc()
{
	daddr_t	bno;
	register i;
	union {
		char	data[DEV_BSIZE];
		struct	fblk frees;
	} fbuf;

	if(sblock.fs_nfree == 0 || (bno=sblock.fs_free[--sblock.fs_nfree]) == 0) {
#ifdef STANDALONE
		printf("Out of space\n");
#else
		fprintf(stderr, "Out of space.\n");
#endif
		exit(1);
	}
	if(sblock.fs_nfree == 0) {
		dread(bno, fbuf.data, DEV_BSIZE);
		sblock.fs_nfree = fbuf.frees.df_nfree;
		for(i=0;i<NICFREE;i++)
			sblock.fs_free[i] = fbuf.frees.df_free[i];
	}
	bzero(fbuf.data, DEV_BSIZE);
	dwrite(bno, fbuf.data);
	sblock.fs_tfree--;
	return(bno);
}

/*
 * map a block number into a block address, ensuring
 * all of the correct indirect blocks are around. Allocate
 * the block requested.
 */
daddr_t
bmap(iaddr, bn)
daddr_t	iaddr[NADDR];
daddr_t	bn;
{
	register i;
	register int j, sh;
	daddr_t nb, nnb;
	daddr_t indir[NINDIR];

	/*
	 * blocks 0...NADDR-3 are direct blocks
	 */
	if(bn < NADDR-3) {
		iaddr[bn] = nb = balloc();
		return(nb);
	}

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for(j=3; j>0; j--) {
		sh += NSHIFT;
		nb <<= NSHIFT;
		if(bn < nb)
			break;
		bn -= nb;
	}
	if(j == 0) {
		return((daddr_t)0);
	}

	/*
	 * fetch the address from the inode
	 */
	if((nb = iaddr[NADDR-j]) == 0) {
		iaddr[NADDR-j] = nb = balloc();
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(; j<=3; j++) {
		dread(nb, (char *)indir, DEV_BSIZE);
		sh -= NSHIFT;
		i = (bn>>sh) & NMASK;
		nnb = indir[i];
		if(nnb == 0) {
			nnb = balloc();
			indir[i] = nnb;
			dwrite(nb, (char *)indir);
		}
		nb = nnb;
	}
	return(nb);
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

/*
 * return whether or not the buffer contains a header block
 */
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

checktype(b, t)
struct	spcl *b;
int	t;
{
	return(b->c_type == t);
}


checksum(b)
register int *b;
{
	register int i, j;

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
 * The next routines are called during file extraction to
 * put the data into the right form and place.
 */
#ifndef STANDALONE
xtrfile(b, size)
char	*b;
long	size;
{
	write(ofile, b, (int) size);
}

null() {;}

skip()
{
	lseek(ofile, (long) DEV_BSIZE, 1);
}
#endif


rstrfile(b, s)
char *b;
long s;
{
	daddr_t d;

	if (isdir && cvtflag)
		return(olddirect(b, s));
	d = bmap(taddr, curbno);
	dwrite(d, b);
	curbno += 1;
}

rstrskip(b, s)
char *b;
long s;
{
	curbno += 1;
}

#ifndef STANDALONE
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
		write(df, drblock.dd_buf, DIRBLKSIZ);
		drblock.dd_loc = 0;
	}
	bcopy(dp, drblock.dd_buf + drblock.dd_loc, dp->d_reclen);
	prev = drblock.dd_loc;
	drblock.dd_loc += dp->d_reclen;
}

flushent()
{

	((struct direct *)(drblock.dd_buf + prev))->d_reclen =DIRBLKSIZ - prev;
	write(df, drblock.dd_buf, DIRBLKSIZ);
	prev = drblock.dd_loc = 0;
	seekpt = lseek(df, 0L, 1);
}

struct direct *
rddir()
{
register struct direct *dp;

	for (;;) {
		if (drblock.dd_loc == 0) {
			drblock.dd_size = read(df, drblock.dd_buf, DIRBLKSIZ);
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

#endif

olddirect(buf, s)
char  *buf;
long	s;
{
	char ndirbuf[DEV_BSIZE];
	register char *dp;
	struct direct cvtbuf;
	register struct odirect *odp, *eodp;
	daddr_t d;
	u_int dirloc, prev;

	inidbuf(ndirbuf);
	dp = ndirbuf;
	odp = (struct odirect *)buf;
	eodp = (struct odirect *)&buf[(int)s];
	for (prev = 0, dirloc = 0; odp < eodp; odp++ ) {
		if (odp->d_ino == 0)
			continue;
		dcvt(odp, &cvtbuf);
		if (dirloc + cvtbuf.d_reclen > DIRBLKSIZ) {
			((struct direct *)(dp + prev))->d_reclen =
					DIRBLKSIZ - prev;
			if (dp != ndirbuf) {
				d = bmap(taddr, curbno);
				dwrite(d, ndirbuf);
				curbno++;
				inidbuf(ndirbuf);
				dp = ndirbuf;
			}
			else
				dp += DIRBLKSIZ;
			dirloc = 0;
		}
		bcopy(&cvtbuf, dp + dirloc, cvtbuf.d_reclen);
		prev = dirloc;
		dirloc += cvtbuf.d_reclen;
	}
	if (dirloc) {
		((struct direct *)(dp + prev))->d_reclen = 
			DIRBLKSIZ - prev;
		d = bmap(taddr, curbno);
		dwrite(d, ndirbuf);
		curbno++;
	}
	return(0);
}

struct direct *
inidbuf(buf)
register char *buf;
{
	register int i;
	register struct direct *dp;

	bzero(buf, DEV_BSIZE);
	for (i = 0; i < DEV_BSIZE; i += DIRBLKSIZ) {
		dp = (struct direct *)&buf[i];
		dp->d_reclen = DIRBLKSIZ;
	}
	return((struct direct *)buf);
}

/*
 * read/write an inode from the disk
 */
getdino(inum, b)
ino_t	inum;
struct	dinode *b;
{
	daddr_t	bno;
	char buf[DEV_BSIZE];

	bno = (inum - 1)/INOPB;
	bno += 2;
	dread(bno, buf, DEV_BSIZE);
	bcopy(&buf[((inum-1)%INOPB)*sizeof(struct dinode)], (char *) b, sizeof(struct dinode));
}

putdino(inum, b)
ino_t	inum;
struct	dinode *b;
{
	daddr_t bno;
	char buf[DEV_BSIZE];

	if (b->di_mode&IFMT)
		sblock.fs_tinode--;
	bno = ((inum - 1)/INOPB) + 2;
	dread(bno, buf, DEV_BSIZE);
	bcopy((char *) b, &buf[((inum-1)%INOPB)*sizeof(struct dinode)], sizeof(struct dinode));
	dwrite(bno, buf);
}

/*
 * read a bit mask from the tape into m.
 */
readbits(m)
register short	*m;
{
	register i;

	i = spcl.c_count;

	while (i--) {
		readtape((char *) m);
#ifdef	STANDALONE
		if(m)
#endif
		m += (DEV_BSIZE/(MLEN/BITS));
	}
	while (gethead(&spcl) == 0)
		;
}

done()
{
#ifndef STANDALONE
	unlink(dirfile);
#endif
	exit(0);
}

futz(addr)
	char	*addr;
	{
	daddr_t	l;
	register char *a, *b;

	a = (char *)&l;
	b = addr;
	*a++ = *b++;
	*a++ = 0;
	*a++ = *b++;
	*a++ = *b++;
	*(daddr_t *)addr = l;
	}
