/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
static char sccsid[] = "@(#)mkproto.c	5.1 (Berkeley) 5/28/85";
#endif not lint

/*
 * Make a file system prototype.
 * usage: mkproto filsys proto
 *
 * Files written to the new filesystem may be up to 4kb less than the
 * max single indirect size:  256kb.
 */
#include <stdio.h>
#include <sys/param.h>
#undef	EXTERNALITIMES		/* Need full inode definiton */
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>

union {
	struct	fs fs;
	char	fsx[DEV_BSIZE];
} ufs;
#define sblock	ufs.fs
struct	fs *fs;
int	fso, fsi;
FILE	*proto;
char	token[BUFSIZ];
int	errs;
ino_t	ino = 10;
int	flshroot;
long	getnum();
char	*strcpy();
daddr_t alloc();
time_t	time();

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	if (argc != 3) {
		fprintf(stderr, "usage: mkproto filsys proto\n");
		exit(1);
	}
	fso = open(argv[1], 1);
	fsi = open(argv[1], 0);
	if (fso < 0 || fsi < 0) {
		perror(argv[1]);
		exit(1);
	}
	fs = &sblock;
	rdfs(SBLOCK, (char *)fs);
	proto = fopen(argv[2], "r");
	descend((struct inode *)0);
	wtfs(SBLOCK, (char *)fs);
	exit(errs);
}

descend(par)
	struct inode *par;
{
	struct inode in;
	int ibc = 0, dbc = 0;
	int i, f, c;
	struct dinode *dip, inos[MAXBSIZE / sizeof (struct dinode)];
	daddr_t ib[MAXBSIZE / sizeof (daddr_t)];
	char buf[MAXBSIZE];

	getstr();
	in.i_mode = gmode(token[0], "-bcd", IFREG, IFBLK, IFCHR, IFDIR);
	in.i_mode |= gmode(token[1], "-u", 0, ISUID, 0, 0);
	in.i_mode |= gmode(token[2], "-g", 0, ISGID, 0, 0);
	for (i = 3; i < 6; i++) {
		c = token[i];
		if (c < '0' || c > '7') {
			printf("%c/%s: bad octal mode digit\n", c, token);
			errs++;
			c = 0;
		}
		in.i_mode |= (c-'0')<<(15-3*i);
	}
	in.i_uid = getnum(); in.i_gid = getnum();
	in.i_nlink = 1;
	in.i_size = 0;
	bzero(in.i_addr, NADDR * sizeof (daddr_t));
	bzero(buf, sizeof (buf));
	bzero(ib, sizeof (ib));

	if (par != (struct inode *)0) {
		ialloc(&in);
	} else {
		par = &in;
		i = itod(ROOTINO);
		rdfs((daddr_t) i, (char *)inos);
		dip = &inos[itoo(ROOTINO)];
		in.i_number = ROOTINO;
		in.i_nlink = dip->di_nlink;
		in.i_size = dip->di_size;
		in.i_db[0] = dip->di_addr[0];
		ib[0] = in.i_db[0];	/* rootino has 1st block assigned */
		ibc = 1;		/* preserve it when iput is done */
		rdfs(in.i_db[0], buf);
		flshroot = 1;
	}

	switch (in.i_mode&IFMT) {

	case IFREG:
		getstr();
		f = open(token, 0);
		if (f < 0) {
			printf("%s: cannot open\n", token);
			errs++;
			break;
		}
		while ((i = read(f, buf, DEV_BSIZE)) > 0) {
			in.i_size += i;
			newblk(buf, &ibc, ib, &dbc);
		}
		close(f);
		break;

	case IFBLK:
	case IFCHR:
		/*
		 * special file
		 * content is maj/min types
		 */

		i = getnum() & 0377;
		f = getnum() & 0377;
		in.i_rdev = (i << 8) | f;
		break;

	case IFDIR:
		/*
		 * directory
		 * put in extra links
		 * call recursively until
		 * name of "$" found
		 */

		if (in.i_number != ROOTINO) {
			par->i_nlink++;
			in.i_nlink++;
			entry(&in, in.i_number, ".", buf, &ibc, ib, &dbc);
			entry(&in, par->i_number, "..", buf, &ibc, ib, &dbc);
		}
		for (;;) {
			getstr();
			if (token[0]=='$' && token[1]=='\0')
				break;
			entry(&in, ino+1, token, buf, &ibc, ib, &dbc);
			descend(&in);
		}
		if (dbc) {
			if (in.i_number == ROOTINO && flshroot)
				wtfs(in.i_db[0], buf);
			else
				newblk(buf, &ibc, ib, &dbc);
		}
		break;
	}
	iput(&in, &ibc, ib);
}

/*ARGSUSED*/
gmode(c, s, m0, m1, m2, m3)
	char c, *s;
{
	int i;

	for (i = 0; s[i]; i++)
		if (c == s[i])
			return((&m0)[i]);
	printf("%c/%s: bad mode\n", c, token);
	errs++;
	return(0);
}

long
getnum()
{
	int i, c;
	long n;

	getstr();
	n = 0;
	i = 0;
	for (i = 0; c=token[i]; i++) {
		if (c<'0' || c>'9') {
			printf("%s: bad number\n", token);
			errs++;
			return((long)0);
		}
		n = n*10 + (c-'0');
	}
	return(n);
}

getstr()
{
	int i, c;

loop:
	switch (c = getc(proto)) {

	case ' ':
	case '\t':
	case '\n':
		goto loop;

	case EOF:
		printf("Unexpected EOF\n");
		exit(1);

	case ':':
		while (getc(proto) != '\n')
			;
		goto loop;

	}
	i = 0;
	do {
		token[i++] = c;
		c = getc(proto);
	} while (c != ' ' && c != '\t' && c != '\n' && c != '\0');
	token[i] = 0;
}

entry(ip, inum, str, buf, aibc, ib, adbc)
	struct inode *ip;
	ino_t inum;
	char *str;
	char *buf;
	int *aibc;
	daddr_t *ib;
	int *adbc;
{
	register struct direct *dp, *odp;
	int oldsize, newsize, spacefree;
	u_short diroff = ip->i_size & DEV_BMASK;

	odp = dp = (struct direct *)buf;
	while ((int)dp - (int)buf < diroff) {
		odp = dp;
		dp = (struct direct *)((int)dp + dp->d_reclen);
	}
	if (odp != dp)
		oldsize = DIRSIZ(odp);
	else
		oldsize = 0;
	spacefree = odp->d_reclen - oldsize;
	dp = (struct direct *)((int)odp + oldsize);
	dp->d_ino = inum;
	dp->d_namlen = strlen(str);
	newsize = DIRSIZ(dp);
	if (spacefree >= newsize) {
		odp->d_reclen = oldsize;
		dp->d_reclen = spacefree;
	} else {
		dp = (struct direct *)((int)odp + odp->d_reclen);
		if ((int)dp - (int)buf >= DEV_BSIZE) {
			if (ip->i_number == ROOTINO && flshroot) {
				flshroot = 0;
				wtfs(ip->i_addr[0], buf);
				bzero(buf, DEV_BSIZE);
			}
			else
				newblk(buf, aibc, ib, adbc);
			dp = (struct direct *)(&buf[DIRBLKSIZ]);
			dp->d_reclen = DIRBLKSIZ;
			dp = (struct direct *)buf;
		}
		dp->d_ino = inum;
		dp->d_namlen = strlen(str);
		dp->d_reclen = DIRBLKSIZ;
	}
	strcpy(dp->d_name, str);
	ip->i_size = (int)dp - (int)buf + newsize;
	(*adbc)++;
}

newblk(buf, aibc, ib, adbc)
	int *aibc;
	char *buf;
	daddr_t *ib;
	int *adbc;
{
	daddr_t bno;

	bno = alloc();
	wtfs(bno, buf);
	bzero(buf, DEV_BSIZE);
	ib[(*aibc)++] = bno;
	if (*aibc >= NINDIR) {
		printf("indirect block full\n");
		errs++;
		*aibc = 0;
	*adbc = 0;
	}
}

iput(ip, aibc, ib)
	register struct inode *ip;
	int *aibc;
	daddr_t *ib;
{
	daddr_t d;
	register int i, off;
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];

	ip->i_atime = ip->i_mtime = ip->i_ctime = time((long *)0);
	switch (ip->i_mode&IFMT) {

	case IFDIR:
	case IFREG:
		for (i = 0; i < *aibc; i++) {
			if (i >= NDADDR)
				break;
			ip->i_db[i] = ib[i];
		}
		if (*aibc > NDADDR) {
			ip->i_ib[0] = alloc();
			for (i = 0; i < NINDIR - NDADDR; i++) {
				ib[i] = ib[i+NDADDR];
				ib[i+NDADDR] = (daddr_t)0;
			}
			wtfs(ip->i_ib[0], (char *)ib);
		}
		break;

	case IFBLK:
	case IFCHR:
		break;

	default:
		printf("bad mode %o\n", ip->i_mode);
		exit(1);
	}
	d = itod(ip->i_number);
	off = itoo(ip->i_number);
	rdfs(d, (char *)buf);
	buf[off].di_ic2 = ip->i_ic2;
	bcopy(ip->i_addr,buf[off].di_addr,NADDR*sizeof(daddr_t));
	buf[off].di_ic1 = ip->i_ic1;
	wtfs(d, (char *)buf);
}

daddr_t
alloc()
{
	daddr_t	bno;
	register i;
	union {
		char	data[DEV_BSIZE];
		struct	fblk frees;
	} fbuf;

	if (!sblock.fs_nfree || (bno=sblock.fs_free[--sblock.fs_nfree]) == 0) {
		fprintf(stderr, "Out of space.\n");
		exit(1);
		}
	sblock.fs_tfree--;
	if(sblock.fs_nfree == 0) {
		rdfs(bno, fbuf.data);
		sblock.fs_nfree = fbuf.frees.df_nfree;
		for(i=0;i<NICFREE;i++)
			sblock.fs_free[i] = fbuf.frees.df_free[i];
	}
	return(bno);
}

/*
 * Allocate an inode on the disk
 *
 * This assumes a newly created filesystem in which fs_inode
 * is zero filled and fs_ninode is zero.
 */

ino_t
ialloc(ip)
	register struct inode *ip;
{

	ip->i_number = ++ino;
	return(ip->i_number);
}

/*
 * read a block from the file system
 */
rdfs(bno, bf)
	daddr_t bno;
	register char *bf;
{
	register int n;

	if (lseek(fsi, bno * DEV_BSIZE, 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
	n = read(fsi, bf, DEV_BSIZE);
	if(n != DEV_BSIZE) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
}

/*
 * write a block to the file system
 */
wtfs(bno, bf)
	daddr_t bno;
	register char *bf;
{
	register int n;

	if (lseek(fso, bno * DEV_BSIZE, 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
	n = write(fso, bf, DEV_BSIZE);
	if(n != DEV_BSIZE) {
		printf("write error: %D\n", bno);
		perror("wtfs");
		exit(1);
	}
}
