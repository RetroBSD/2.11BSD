/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
static char sccsid[] = "@(#)ncheck.c	5.4 (Berkeley) 1/9/86";
#endif 

/*
 * ncheck -- obtain file names from reading filesystem
 */

#define	NI		8
#define	NB		100
#define	HSIZE		1503

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <stdio.h>

struct	fs	sblock;
struct	dinode	itab[NI * INOPB];
struct 	dinode	*gip;
struct ilist {
	ino_t	ino;
	u_short	mode;
	uid_t	uid;
	gid_t	gid;
} ilist[NB];
struct	htab
{
	ino_t	h_ino;
	ino_t	h_pino;
	char	*h_name;
} htab[HSIZE];

struct dirstuff {
	off_t loc;
	struct dinode *ip;
	char dbuf[MAXBSIZE];
};

int	aflg;
int	sflg;
int	iflg; /* number of inodes being searched for */
int	mflg;
int	fi;
ino_t	ino;
int	nhent;
int	nxfile;

int	nerror;
daddr_t	bmap();
long	atol();
struct htab *lookup();

main(argc, argv)
	int argc;
	char *argv[];
{
	register i;
	long n;

	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {

		case 'a':
			aflg++;
			continue;

		case 'i':
			for(iflg=0; iflg<NB; iflg++) {
				n = atol(argv[1]);
				if(n == 0)
					break;
				ilist[iflg].ino = (ino_t)n;
				nxfile = iflg;
				argv++;
				argc--;
			}
			continue;

		case 'm':
			mflg++;
			continue;

		case 's':
			sflg++;
			continue;

		default:
			fprintf(stderr, "ncheck: bad flag %c\n", (*argv)[1]);
			nerror++;
		}
		check(*argv);
	}
	return(nerror);
}

check(file)
	char *file;
{
	register int i, j;
	int mino;

	fi = open(file, 0);
	if(fi < 0) {
		fprintf(stderr, "ncheck: cannot open %s\n", file);
		nerror++;
		return;
	}
	nhent = 0;
	printf("%s:\n", file);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	ino = 0;
	mino = (sblock.fs_isize-2) * INOPB;
	for (i = 2; ; i+=NI) {
		if (ino >= mino)
			break;
		bread((daddr_t) i, (char *)itab, sizeof (itab));
		for (j = 0; j < INOPB * NI; j++) {
			if (ino >= mino)
				break;
			ino++;
			pass1(&itab[j]);
		}
	}
	ilist[nxfile+1].ino = 0;
	ino = 0;
	for (i = 2; ; i+=NI) {
		if (ino >= mino)
			break;
		bread((daddr_t) i, (char *)itab, sizeof (itab));
		for (j = 0; j < INOPB * NI; j++) {
			if (ino >= mino)
				break;
			ino++;
			pass2(&itab[j]);
		}
	}
	ino = 0;
	for (i = 2; ; i+=NI) {
		if (ino >= mino)
			break;
		bread((daddr_t) i, (char *)itab, sizeof (itab));
		for (j = 0; j < INOPB * NI; j++) {
			if (ino >= mino)
				break;
			ino++;
			pass3(&itab[j]);
		}
	}
	close(fi);
	for (i = 0; i < HSIZE; i++) {
		if (htab[i].h_name)
			free(htab[i].h_name);
		htab[i].h_ino = 0;
		htab[i].h_name = 0;
	}
	for (i = iflg; i < NB; i++)
		ilist[i].ino = 0;
	nxfile = iflg;
}

pass1(ip)
	register struct dinode *ip;
{
	register int i;

	if (mflg)
		for (i = 0; i < iflg; i++)
			if (ino == ilist[i].ino) {
				ilist[i].mode = ip->di_mode;
				ilist[i].uid = ip->di_uid;
				ilist[i].gid = ip->di_gid;
			}
	if ((ip->di_mode & IFMT) != IFDIR) {
		if (sflg==0 || nxfile>=NB)
			return;
		if ((ip->di_mode&IFMT)==IFBLK || (ip->di_mode&IFMT)==IFCHR
		  || ip->di_mode&(ISUID|ISGID)) {
			ilist[nxfile].ino = ino;
			ilist[nxfile].mode = ip->di_mode;
			ilist[nxfile].uid = ip->di_uid;
			ilist[nxfile++].gid = ip->di_gid;
			return;
		}
	}
	lookup(ino, 1);
}

pass2(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	struct htab *hp;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		hp = lookup((ino_t)dp->d_ino, 0);
		if(hp == 0)
			continue;
		if(dotname(dp))
			continue;
		hp->h_pino = ino;
		hp->h_name = (char *)malloc(strlen(dp->d_name) + 1);
		if (hp->h_name == (char *)NULL) {
			fprintf(stderr, "no memory for name %s\n",dp->d_name);
			return;
		}
		strcpy(hp->h_name, dp->d_name);
	}
}

pass3(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	int k;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for(dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(aflg==0 && dotname(dp))
			continue;
		if(sflg == 0 && iflg == 0)
			goto pr;
		for(k = 0; ilist[k].ino != 0; k++)
			if(ilist[k].ino == dp->d_ino)
				break;
		if (ilist[k].ino == 0)
			continue;
		if (mflg)
			printf("mode %-6o uid %-5d gid %-5d ino ",
			    ilist[k].mode, ilist[k].uid, ilist[k].gid);
	pr:
		printf("%-5u\t", (ino_t)dp->d_ino);
		pname(ino, 0);
		printf("/%s", dp->d_name);
		if (lookup((ino_t)dp->d_ino, 0))
			printf("/.");
		printf("\n");
	}
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
		if (blkoff(dirp->loc) == 0) {
			lbn = lblkno(dirp->loc);
			d = bmap(lbn);
			if(d == 0)
				return NULL;
			bread(d, dirp->dbuf, DEV_BSIZE);
		}
		dp = (struct direct *) (dirp->dbuf + blkoff(dirp->loc));
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

dotname(dp)
	register struct direct *dp;
{

	if (dp->d_name[0]=='.')
		if (dp->d_name[1]==0 ||
		   (dp->d_name[1]=='.' && dp->d_name[2]==0))
			return(1);
	return(0);
}

pname(i, lev)
	ino_t i;
	int lev;
{
	register struct htab *hp;

	if (i==ROOTINO)
		return;
	if ((hp = lookup(i, 0)) == 0) {
		printf("???");
		return;
	}
	if (lev > 10) {
		printf("...");
		return;
	}
	pname(hp->h_pino, ++lev);
	printf("/%s", hp->h_name);
}

struct htab *
lookup(i, ef)
	ino_t i;
	int ef;
{
	register struct htab *hp;

	for (hp = &htab[i%HSIZE]; hp->h_ino;) {
		if (hp->h_ino==i)
			return(hp);
		if (++hp >= &htab[HSIZE])
			hp = htab;
	}
	if (ef==0)
		return(0);
	if (++nhent >= HSIZE) {
		fprintf(stderr, "ncheck: HSIZE of %d is too small\n", HSIZE);
		exit(1);
	}
	hp->h_ino = i;
	return(hp);
}

bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	register int cnt;
{

	lseek(fi, bno * DEV_BSIZE, 0);
	if (read(fi, buf, cnt) != cnt) {
		fprintf(stderr, "ncheck: read error %d\n", bno);
		bzero(buf, cnt);
	}
}

daddr_t
bmap(i)
	daddr_t i;
{
	daddr_t ibuf[NINDIR];

	if(i < NDADDR)
		return(gip->di_addr[i]);
	i -= NDADDR;
	if(i > NINDIR) {
		fprintf(stderr, "ncheck: %u - huge directory\n", ino);
		return((daddr_t)0);
	}
	bread(gip->di_addr[NDADDR], (char *)ibuf, sizeof(ibuf));
	return(ibuf[i]);
}
