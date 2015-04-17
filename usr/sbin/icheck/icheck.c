#if	!defined(lint) && defined(DOSCCS)
char	*sccsid = "@(#)icheck.c	2.5";
#endif

#include <sys/param.h>

#ifdef	STANDALONE
#define	NI	4
#else
#define	NI	8
#endif

#define	NB	10
#define	BITS	8
#define	MAXFN	500

#ifndef STANDALONE
#include <stdio.h>
#endif !STANDALONE
#include <sys/inode.h>
#include <sys/fs.h>

struct	fs	sblock;
struct	dinode	itab[INOPB*NI];
daddr_t	blist[NB];
char	*bmap;

int	sflg;
int	mflg;
int	dflg;
int	fi;
ino_t	ino;

ino_t	nrfile;
ino_t	nsfile;
ino_t	ndfile;
ino_t	nbfile;
ino_t	ncfile;
ino_t	nlfile;

daddr_t	ndirect;
daddr_t	nindir;
daddr_t	niindir;
daddr_t	niiindir;
daddr_t	nfree;
daddr_t	ndup;

int	nerror;

long	atol();
daddr_t	alloc();

#ifdef STANDALONE
char	module[] = "Icheck";
#define	STDBUFSIZ	16000		/* Small enough for 407 */
char	stdbuf[STDBUFSIZ];
#else !STANDALONE
char	*malloc();
#endif STANDALONE

main(argc, argv)
char *argv[];
{
	register i;
	long n;

	blist[0] = -1;
#ifndef STANDALONE
	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {
		case 'd':
			dflg++;
			continue;


		case 'm':
			mflg++;
			continue;

		case 's':
			sflg++;
			continue;

		case 'b':
			for(i=0; i<NB; i++) {
				n = atol(argv[1]);
				if(n == 0)
					break;
				blist[i] = n;
				argv++;
				argc--;
			}
			blist[i] = -1;
			continue;

		default:
			printf("Bad flag\n");
		}
		check(*argv);
	}
#else
	{
		static char fname[100];
		printf("%s\n",module);
		printf("File: ");
		gets(fname);
		check(fname);
	}
#endif
	return(nerror);
}

check(file)
char *file;
{
	register i, j;
	ino_t mino;
	daddr_t d;
	long n;

	fi = open(file, sflg?2:0);
	if (fi < 0) {
		printf("cannot open %s\n", file);
		nerror |= 04;
		return;
	}
	printf("%s:\n", file);
	nrfile = 0;
	nsfile = 0;
	ndfile = 0;
	ncfile = 0;
	nbfile = 0;
	nlfile = 0;

	ndirect = 0;
	nindir = 0;
	niindir = 0;
	niiindir = 0;

	ndup = 0;
#ifndef STANDALONE
	sync();
#endif
	bread((daddr_t)SBLOCK, (char *)&sblock, sizeof(sblock));
	mino = (sblock.fs_isize-2) * INOPB;
	ino = 0;
	n = (sblock.fs_fsize - sblock.fs_isize + BITS-1) / BITS;
	if (n != (unsigned)n) {
		printf("Check fsize and isize: %D, %u\n",
		   sblock.fs_fsize, sblock.fs_isize);
	}
#ifdef STANDALONE
	if((unsigned)n > STDBUFSIZ)
		bmap = NULL;
	else
		bmap = stdbuf;
#else
	bmap = malloc((unsigned)n);
#endif
	if (bmap==NULL) {
		printf("Not enough core; duplicates unchecked\n");
		dflg++;
		sflg = 0;
	}
	if(!dflg)
		bzero(bmap, (u_short) n);
	for(i=2;; i+=NI) {
		if(ino >= mino)
			break;
		bread((daddr_t)i, (char *)itab, sizeof(itab));
		for(j=0; j<INOPB*NI; j++) {
			if(ino >= mino)
				break;
			ino++;
			pass1(&itab[j]);
		}
	}
	ino = 0;
#ifndef STANDALONE
	sync();
#endif
	bread((daddr_t)SBLOCK, (char *)&sblock, sizeof(sblock));
	if (sflg) {
		makefree();
		close(fi);
#ifndef STANDALONE
		if (bmap)
			free(bmap);
#endif
		return;
	}
	nfree = 0;
	while(n = alloc()) {
		if (chk(n, "free"))
			break;
		nfree++;
	}
	close(fi);
#ifndef STANDALONE
	if (bmap)
		free(bmap);
#endif

	i = nrfile + ndfile + ncfile + nbfile;
	i += nlfile;
	printf("files %u (r=%u,d=%u,b=%u,c=%u,l=%u,s=%u)\n",
		i, nrfile, ndfile, nbfile, ncfile, nlfile,nsfile);
	n = ndirect + nindir + niindir + niindir;
	printf("used %D (i=%D,ii=%D,iii=%D,d=%D)\n",
		n, nindir, niindir, niiindir, ndirect);
	printf("free %D\n", nfree);
	if(!dflg) {
		n = 0;
		for(d=sblock.fs_isize; d<sblock.fs_fsize; d++)
			if(!duped(d)) {
				if(mflg)
					printf("%D missing\n", d);
				n++;
			}
		printf("missing %D\n", n);
	}
}

pass1(ip)
register struct dinode *ip;
{
	daddr_t ind1[NINDIR];
	daddr_t ind2[NINDIR];
	daddr_t ind3[NINDIR];
	register i, j;
	int k, l;

	i = ip->di_mode & IFMT;
	if(i == 0) {
		sblock.fs_tinode++;
		return;
	}
	if(i == IFCHR) {
		ncfile++;
		return;
	}
	if(i == IFBLK) {
		nbfile++;
		return;
	}
	if(i == IFLNK)
		nlfile++;
	else if(i == IFDIR)
		ndfile++;
	else if(i == IFREG)
		nrfile++;
	else if(i == IFSOCK)
		nsfile++;
	else {
		printf("bad mode %u\n", ino);
		return;
	}
	for(i=0; i<NADDR; i++) {
		if(ip->di_addr[i] == 0)
			continue;
		if(i < NADDR-3) {
			ndirect++;
			chk(ip->di_addr[i], "data (small)");
			continue;
		}
		nindir++;
		if (chk(ip->di_addr[i], "1st indirect"))
				continue;
		bread(ip->di_addr[i], (char *)ind1, DEV_BSIZE);
		for(j=0; j<NINDIR; j++) {
			if(ind1[j] == 0)
				continue;
			if(i == NADDR-3) {
				ndirect++;
				chk(ind1[j], "data (large)");
				continue;
			}
			niindir++;
			if(chk(ind1[j], "2nd indirect"))
				continue;
			bread(ind1[j], (char *)ind2, DEV_BSIZE);
			for(k=0; k<NINDIR; k++) {
				if(ind2[k] == 0)
					continue;
				if(i == NADDR-2) {
					ndirect++;
					chk(ind2[k], "data (huge)");
					continue;
				}
				niiindir++;
				if(chk(ind2[k], "3rd indirect"))
					continue;
				bread(ind2[k], (char *)ind3, DEV_BSIZE);
				for(l=0; l<NINDIR; l++)
					if(ind3[l]) {
						ndirect++;
						chk(ind3[l], "data (garg)");
					}
			}
		}
	}
}

chk(bno, s)
daddr_t bno;
char *s;
{
	register n;

	if (bno<sblock.fs_isize || bno>=sblock.fs_fsize) {
		printf("%D bad; inode=%u, class=%s\n", bno, ino, s);
		return(1);
	}
	if(duped(bno)) {
		printf("%D dup; inode=%u, class=%s\n", bno, ino, s);
		ndup++;
	}
	for (n=0; blist[n] != -1; n++)
		if (bno == blist[n])
			printf("%D arg; inode=%u, class=%s\n", bno, ino, s);
	return(0);
}

duped(bno)
daddr_t bno;
{
	daddr_t d;
	register m, n;

	if(dflg)
		return(0);
	d = bno - sblock.fs_isize;
	m = 1 << (d%BITS);
	n = (d/BITS);
	if(bmap[n] & m)
		return(1);
	bmap[n] |= m;
	return(0);
}

daddr_t
alloc()
{
	int i;
	daddr_t bno;
	union {
		char	data[DEV_BSIZE];
		struct	fblk fb;
	} buf;

	sblock.fs_tfree--;
	if (sblock.fs_nfree<=0)
		return(0);
	if (sblock.fs_nfree>NICFREE) {
		printf("Bad free list, s.b. count = %d\n", sblock.fs_nfree);
		return(0);
	}
	bno = sblock.fs_free[--sblock.fs_nfree];
	sblock.fs_free[sblock.fs_nfree] = (daddr_t)0;
	if(bno == 0)
		return(bno);
	if(sblock.fs_nfree <= 0) {
		bread(bno, buf.data, DEV_BSIZE);
		sblock.fs_nfree = buf.fb.df_nfree;
		if (sblock.fs_nfree<0 || sblock.fs_nfree>NICFREE) {
			printf("Bad free list, entry count of block %D = %d\n",
				bno, sblock.fs_nfree);
			sblock.fs_nfree = 0;
			return(0);
		}
		for(i=0; i<NICFREE; i++)
			sblock.fs_free[i] = buf.fb.df_free[i];
	}
	return(bno);
}

bfree(bno)
daddr_t bno;
{
	union {
		char	data[DEV_BSIZE];
		struct	fblk fb;
	} buf;
register int i;

	sblock.fs_tfree++;
	if(sblock.fs_nfree >= NICFREE) {
		bzero(buf.data, DEV_BSIZE);
		buf.fb.df_nfree = sblock.fs_nfree;
		for(i=0; i<NICFREE; i++)
			buf.fb.df_free[i] = sblock.fs_free[i];
		bwrite(bno, buf.data);
		sblock.fs_nfree = 0;
	}
	sblock.fs_free[sblock.fs_nfree] = bno;
	sblock.fs_nfree++;
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{

	lseek(fi, bno*DEV_BSIZE, 0);
	if (read(fi, buf, cnt) != cnt) {
		printf("read error %D\n", bno);
		if (sflg) {
			printf("No update\n");
			sflg = 0;
		}
		bzero(buf, DEV_BSIZE);
	}
}

bwrite(bno, buf)
daddr_t bno;
char	*buf;
{

	lseek(fi, bno*DEV_BSIZE, 0);
	if (write(fi, buf, DEV_BSIZE) != DEV_BSIZE)
		printf("write error %D\n", bno);
}

makefree()
{
	char flg[MAXFN];
	int adr[MAXFN];
	register i, j;
	daddr_t f, d;
	int m, n;

	n = sblock.fs_cyl;
	if(n <= 0 || n > MAXFN)
		n = MAXFN;
	sblock.fs_cyl = n;
	m = sblock.fs_step;
	if(m <= 0 || m > sblock.fs_cyl)
		m = 3;
	sblock.fs_step = m;

	bzero(flg, n);
	i = 0;
	for(j=0; j<n; j++) {
		while(flg[i])
			i = (i+1)%n;
		adr[j] = i+1;
		flg[i]++;
		i = (i+m)%n;
	}

	sblock.fs_nfree = 0;
	sblock.fs_ninode = 0;
	sblock.fs_flock = 0;
	sblock.fs_ilock = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;
#ifndef STANDALONE
	time(&sblock.fs_time);
#endif
	sblock.fs_tfree = 0;
	sblock.fs_tinode = 0;

	bfree((daddr_t)0);
	d = sblock.fs_fsize-1;
	while(d%sblock.fs_cyl)
		d++;
	for(; d > 0; d -= sblock.fs_cyl)
	for(i=0; i<sblock.fs_cyl; i++) {
		f = d - adr[i];
		if(f < sblock.fs_fsize && f >= sblock.fs_isize)
			if(!duped(f))
				bfree(f);
	}
	bwrite((daddr_t)1, (char *)&sblock);
#ifndef STANDALONE
	sync();
#endif
	return;
}
