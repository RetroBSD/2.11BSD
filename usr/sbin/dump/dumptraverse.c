#if	!defined(lint) && defined(DOSCCS)
static	char *sccsid = "@(#)dumptraverse.c	1.2 (2.11BSD GTE) 12/6/94";
#endif

#include "dump.h"

struct	fs	sblock;		/* disk block */
struct	dinode itab[INOPB * NI];

pass(fn, map)
int (*fn)();
short *map;
{
	register i, j;
	int bits;
	ino_t mino;
	daddr_t d;

	sync();
	bread((daddr_t)1, (char *)&sblock, sizeof(sblock));
	mino = (sblock.fs_isize-2) * INOPB;
	ino = 0;
	for(i=2;; i+=NI) {
		if(ino >= mino)
			break;
		d = (unsigned)i;
		for(j=0; j<INOPB*NI; j++) {
			if(ino >= mino)
				break;
			if((ino % MLEN) == 0) {
				bits = ~0;
				if(map != NULL)
					bits = *map++;
			}
			ino++;
			if(bits & 1) {
				if(d != 0) {
					bread(d, (char *)itab, sizeof(itab));
					d = 0;
				}
				(*fn)(&itab[j]);
			}
			bits >>= 1;
		}
	}
}

icat(ip, fn1, fn2)
struct	dinode	*ip;
int (*fn1)(), (*fn2)();
{
	register i;

	(*fn2)(ip->di_addr, NADDR-3);
	for(i=0; i<NADDR; i++) {
		if(ip->di_addr[i] != 0) {
			if(i < NADDR-3)
				(*fn1)(ip->di_addr[i]); else
				indir(ip->di_addr[i], fn1, fn2, i-(NADDR-3));
		}
	}
}

indir(d, fn1, fn2, n)
daddr_t d;
int (*fn1)(), (*fn2)();
{
	register i;
	daddr_t	idblk[NINDIR];

	bread(d, (char *)idblk, sizeof(idblk));
	if(n <= 0) {
		spcl.c_type = TS_ADDR;
		(*fn2)(idblk, NINDIR);
		for(i=0; i<NINDIR; i++) {
			d = idblk[i];
			if(d != 0)
				(*fn1)(d);
		}
	} else {
		n--;
		for(i=0; i<NINDIR; i++) {
			d = idblk[i];
			if(d != 0)
				indir(d, fn1, fn2, n);
		}
	}
}

#define	CHANGEDSINCE(dp,t)  ((dp)->di_mtime >= (t) || (dp)->di_ctime >= (t))
#define	WANTTODUMP(dp)  (CHANGEDSINCE(dp,spcl.c_ddate) && \
			 (nonodump || (dp->di_flags & UF_NODUMP) != UF_NODUMP))

mark(ip)
register struct dinode *ip;
{
	register f;

	f = ip->di_mode & IFMT;
	if(f == 0)
		return;
	BIS(ino, clrmap);
	if(f == IFDIR)
		BIS(ino, dirmap);
	if (WANTTODUMP(ip)) {
		BIS(ino, nodmap);
		if (f != IFREG && f != IFDIR && f != IFLNK){
			esize++;
			return;
		}
		est(ip);
	}
}

add(ip)
struct dinode *ip;
{

	if(BIT(ino, nodmap))
		return;
	nsubdir = 0;
	dadded = 0;
	icat(ip, dsrch, nullf);
	if(dadded) {
		BIS(ino, nodmap);
		est(ip);
		nadded++;
	}
	if(nsubdir == 0)
		if(!BIT(ino, nodmap))
			BIC(ino, dirmap);
}

dump(ip)
struct dinode *ip;
{
	register i;

	if(newtape) {
		newtape = 0;
		bitmap(nodmap, TS_BITS);
	}
	BIC(ino, nodmap);
	spcl.c_dinode = *ip;
	spcl.c_type = TS_INODE;
	spcl.c_count = 0;
	i = ip->di_mode & IFMT;
	if(i != IFDIR && i != IFREG && i != IFLNK) {
		spclrec();
		return;
	}
	icat(ip, tapsrec, dmpspc);
}

dmpspc(dp, n)
daddr_t *dp;
{
	register i, t;

	spcl.c_count = n;
	for(i=0; i<n; i++) {
		t = 0;
		if(dp[i] != 0)
			t++;
		spcl.c_addr[i] = t;
	}
	spclrec();
}

bitmap(map, typ)
short *map;
{
	register i, n;
	char *cp;

	n = -1;
	for(i=0; i<MSIZ; i++)
		if(map[i])
			n = i;
	if(n < 0)
		return;
	spcl.c_type = typ;
	spcl.c_count = (n*sizeof(map[0]) + DEV_BSIZE)/DEV_BSIZE;
	spclrec();
	cp = (char *)map;
	for(i=0; i<spcl.c_count; i++) {
		taprec(cp);
		cp += DEV_BSIZE;
	}
}

spclrec()
{
	register i, *ip, s;

	spcl.c_inumber = ino;
	spcl.c_magic = NFS_MAGIC;
	spcl.c_checksum = 0;
	ip = (int *)&spcl;
	s = 0;
	for(i=0; i<DEV_BSIZE/sizeof(*ip); i++)
		s += *ip++;
	spcl.c_checksum = CHECKSUM - s;
	taprec((char *)&spcl);
}

dsrch(d)
daddr_t d;
{
	register struct direct *dp;
	register int i;
	char	dbuf[DEV_BSIZE];

	if(dadded)
		return;
	bread(d, dbuf, DEV_BSIZE);
	for (i = 0; i < DEV_BSIZE; ) {
		dp = (struct direct *)(dbuf + i);
		if (dp->d_reclen == 0) {
/*
 * following hack is for directories which, although occupying a full fs block
 * do not have the second DIRBLKSIZ section initialized to an empty dir blk.
*/
			if (i != DIRBLKSIZ)	/* XXX */
				fprintf(stderr,"corrupted directory, inumber %u\n",ino);
			break;
		}
		i += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		if (dp->d_name[0] == '.') {
			if (dp->d_name[1] == '\0')
				continue;
			if (dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		if(BIT(dp->d_ino, nodmap)) {
			dadded++;
			return;
		}
		if(BIT(dp->d_ino, dirmap))
			nsubdir++;
	}
}

nullf()
{
}

int	breaderrors = 0;
#define	BREADEMAX 32

bread(da, ba, c)
	daddr_t da;
	char *ba;
	unsigned c;	
{
	register n;
	register unsigned regc;

	if (lseek(fi, (off_t)da * DEV_BSIZE, 0) < 0)
		msg("bread: lseek fails\n");
	regc = c;	/* put c someplace safe; it gets clobbered */
	n = read(fi, ba, c);
	if (n == -1 || ((unsigned)n) != c || regc != c){
		msg("(This should not happen)bread from %s [block %ld]: c=0x%x, regc=0x%x, &c=0x%x, n=0x%x\n",
			disk, da, c, regc, &c, n);
		if (++breaderrors > BREADEMAX){
			msg("More than %d block read errors from %d\n",
				BREADEMAX, disk);
			broadcast("DUMP IS AILING!\n");
			msg("This is an unrecoverable error.\n");
			if (!query("Do you want to attempt to continue?")){
				dumpabort();
				/*NOTREACHED*/
			} else
				breaderrors = 0;
		}
	}
}
