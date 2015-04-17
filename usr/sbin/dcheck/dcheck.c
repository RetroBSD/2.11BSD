/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
static char sccsid[] = "@(#)dcheck.c	5.1 (Berkeley) 6/6/85";
#endif

/*
 * dcheck - check directory consistency
 */
#define	NI	8
#define	NF	2048
#define	NB	10
#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <stdio.h>

union {
	struct	fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	sblock	fsun.fs

struct dirstuff {
	off_t loc;
	struct dinode *ip;
	char dbuf[MAXBSIZE];
};

struct	dinode	itab[NI * INOPB];
struct	dinode	*gip;
ino_t	ilist[NB];

int	fi;
int	fo;
ino_t	ino;
ino_t	ecount[NF];
ino_t	starti, endi;
int	edirty;
char	*tfn = "/tmp/dchkXXXXXX";
int	headpr;
ino_t	nfiles;

int	nerror;
daddr_t	bmap();
extern long	atol(), lseek();
extern char	*malloc();
extern	int	errno;

main(argc, argv)
char *argv[];
{
	register i;
	long n;

	mktemp(tfn);
	fo = open(tfn, O_CREAT|O_RDWR, 0600);
	if (fo < 0) {
		fprintf(stderr, "Can't create tmp file: %s\n", tfn);
		exit(4);
	}
	unlink(tfn);

	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {

		case 'i':
			for(i=0; i<NB; i++) {
				n = atol(argv[1]);
				if(n == 0)
					break;
				ilist[i] = (ino_t)n;
				argv++;
				argc--;
			}
			ilist[i] = 0;
			continue;

		default:
			printf("Bad flag %c\n", (*argv)[1]);
			nerror++;
		}
		check(*argv);
	}
	return(nerror);
}

check(file)
char *file;
{
	register i, j;

	fi = open(file, 0);
	if(fi < 0) {
		printf("cannot open %s\n", file);
		nerror++;
		return;
	}

	bread(SBLOCK, (char *)&sblock, SBSIZE);
	nfiles = (sblock.fs_isize - 2) * INOPB;
	bzero(ecount, sizeof (ecount));

	lseek(fo, 0L, 0);
	ftruncate(fo, 0L);
	lseek(fo, (off_t)nfiles * sizeof (u_short), 0);
	write(fo, ecount, sizeof (ecount));

	headpr = 0;
	printf("%s:\n", file);
	sync();
	ino = 0;
	for (i = 2; ; i += NI) {
		if (ino >= nfiles)
			break;
		bread((daddr_t)i, (char *)itab, sizeof (itab));
		for (j = 0; j < INOPB * NI; j++) {
			if (ino >= nfiles)
				break;
			ino++;
			pass1(&itab[j]);
		}
	}
	ino = 0;
	for (i = 2; ; i += NI) {
		if (ino >= nfiles)
			break;
		bread((daddr_t)i, (char *)itab, sizeof (itab));
		for (j = 0; j < INOPB * NI; j++) {
			if (ino >= nfiles)
				break;
			ino++;
			pass1(&itab[j]);
		}
	}
}

pass1(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	register int k;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		if(dp->d_ino > nfiles || dp->d_ino < ROOTINO) {
			printf("%u bad; %u/%s\n",
			    dp->d_ino, ino, dp->d_name);
			nerror++;
			continue;
		}
		for (k = 0; ilist[k] != 0; k++)
			if (ilist[k] == dp->d_ino) {
				printf("%u arg; %u/%s\n",
				     dp->d_ino, ino, dp->d_name);
				nerror++;
			}
		dolncnt((ino_t)dp->d_ino, 1);
	}
}

pass2(ip)
register struct dinode *ip;
{
	register ino_t i;
	register u_short cnt;

	i = ino;
	cnt = dolncnt(i, 0);
	if ((ip->di_mode&IFMT)==0 && cnt==0)
		return;
	if (ip->di_nlink==cnt && ip->di_nlink!=0)
		return;
	if (headpr==0) {
		printf("     entries  link cnt\n");
		headpr++;
	}
	printf("%u\t%d\t%d\n", ino, cnt, ip->di_nlink);
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	daddr_t lbn, d;

	for(;;) {
		if (dirp->loc >= dirp->ip->di_size)
			return NULL;
		if ((lbn = lblkno(dirp->loc)) == 0) {
			d = bmap(lbn);
			if(d == 0)
				return NULL;
			bread(d, dirp->dbuf, DEV_BSIZE);
		}
		dp = (struct direct *)(dirp->dbuf + blkoff(dirp->loc));
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{

	lseek(fi, bno * DEV_BSIZE, 0);
	if (read(fi, buf, cnt) != cnt) {
		printf("read error %ld\n", bno);
		bzero(buf, cnt);
	}
}

daddr_t
bmap(i)
	daddr_t i;
{
	daddr_t ibuf[MAXNINDIR];

	if(i < NDADDR)
		return(gip->di_addr[i]);
	i -= NDADDR;
	if(i > NINDIR) {
		printf("%u - huge directory\n", ino);
		return((daddr_t)0);
	}
	bread(gip->di_addr[NDADDR], (char *)ibuf, sizeof(ibuf));
	return(ibuf[i]);
}

u_short
dolncnt(i, cnt)
	register ino_t i;
	register u_short cnt;
{
again:
	if (i >= starti && i <= endi) {
		if (cnt)
			edirty = 1;
		ecount[i - starti] += cnt;
		return(ecount[i - starti]);
	}
	fill(i);
	goto again;
}

flush()
{

	if (edirty) {
		edirty = 0;
		if (lseek(fo, (off_t)(starti / NF) * sizeof (ecount), 0) < 0) {
			fprintf(stderr, "lseek error %d in tmp file\n", errno);
			return;
		}
		if (write(fo, ecount, sizeof (ecount)) != sizeof (ecount)) {
			fprintf(stderr, "write error %d in tmp file\n", errno);
			return;
		}
	}
}

fill(i)
register ino_t i;
{
	flush();
	starti = (i / NF) * NF;
	endi = starti + NF - 1;
	if (lseek(fo, (off_t)(starti/NF) * sizeof (ecount), 0) < 0) {
		fprintf(stderr, "lseek error %d in tmp file\n", errno);
		return;
	}
	if (read(fo, ecount, sizeof (ecount)) != sizeof (ecount)) {
		fprintf(stderr, "read error %d in tmp file\n", errno);
		return;
	}
}
