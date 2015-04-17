#if	!defined(lint) && defined(DOSCCS)
char	*sccsid = "@(#)mkfs.c	2.9 (2.11BSD) 1996/5/8";
#endif

/*
 * Make a file system.  Run by 'newfs' and not directly by users.
 * usage: mkfs -s size -i byte/ino -n num -m num filsys
 *
 * NOTE:  the size is specified in filesystem (1k) blocks NOT sectors.
 *	  newfs does the conversion before running mkfs - if you run mkfs
 *	  manually beware that the '-s' option means sectors to newfs but
 *	  filesystem blocks to mkfs!
 */

#include <sys/param.h>
/*
 * Need to do the following to get the larger incore inode structure
 * (the kernel might be built with the inode times external/mapped-out).
 * See /sys/h/localtimes.h and /sys/conf.
*/
#undef	EXTERNALITIMES

#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/fs.h>

#ifndef STANDALONE
#include <stdlib.h>
#include <stdio.h>
#include <sys/inode.h>
#else
#include "saio.h"
#endif

#define	UMASK	0755
#define	MAXFN	750

time_t	utime;

#ifdef STANDALONE
int	fin;
char	module[] = "Mkfs";
extern	char	*ltoa();
extern	long	atol();
extern	struct	iob	iob[];
#endif

int	fsi;
int	fso;
char	buf[DEV_BSIZE];

union {
	struct fblk fb;
	char pad1[DEV_BSIZE];
} fbuf;

union {
	struct fs fs;
	char pad2[DEV_BSIZE];
} filsys;

u_int	f_i	= 4096;		/* bytes/inode default */
int	f_n	= 100;
int	f_m	= 2;

	daddr_t	alloc();

main(argc,argv)
	int	argc;
	char	**argv;
{
register int c;
	long n;
	char *size = 0;
	char	*special;
#ifdef	STANDALONE
	struct	disklabel *lp;
register struct	partition *pp;
	struct	iob	*io;
#endif

#ifndef STANDALONE
	time(&utime);
	while	((c = getopt(argc, argv, "i:m:n:s:")) != EOF)
		{
		switch	(c)
			{
			case	'i':
				f_i = atoi(optarg);
				break;
			case	'm':
				f_m = atoi(optarg);
				break;
			case	'n':
				f_n = atoi(optarg);
				break;
			case	's':
				size = optarg;
				break;
			default:
				usage();
				break;
			}
		}
	argc -= optind;
	argv += optind;
	if	(argc != 1 || !size || !f_i || !f_m || !f_n)
		usage();
	special = *argv;

/*
 * NOTE: this will fail if the device is currently mounted and the system
 * is at securelevel 1 or higher.
 *
 * We do not get the partition information because 'newfs' has already
 * done so and invoked us.  This program should not be run manually unless
 * you are absolutely sure you know what you are doing - use 'newfs' instead.
*/
	fso = creat(special, 0666);
	if	(fso < 0)
		err(1, "cannot create %s\n", special);
	fsi = open(special, 0);
	if	(fsi < 0)
		err(1, "cannot open %s\n", special);
#else
/*
 * Something more modern than January 1, 1970 - the date that the new 
 * standalone mkfs worked.  1995/06/08 2121.
*/
	utime = 802671684L;
	printf("%s\n",module);
	do {
		printf("file system: ");
		gets(buf);
		fso = open(buf, 1);
		fsi = open(buf, 0);
	} while (fso < 0 || fsi < 0);

/*
 * If the underlying driver supports disklabels then do not make a file
 * system unless: there is a valid label present, the specified partition
 * is of type FS_V71K, and the size is not zero.
 *
 * The 'open' above will have already fetched the label if the driver supports
 * labels - the check below will only fail if the driver doesn't do labels
 * or if the drive blew up in the millisecond since the last read.
*/
	io = &iob[fsi - 3];
	lp = &io->i_label;
	pp = &lp->d_partitions[io->i_part];

	if	(devlabel(io, READLABEL) < 0)
		{
/*
 * The driver does not implement labels.  The 'iob' structure still contains
 * a label structure so initialize the items that will be looked at later.
*/
		pp->p_size = 0;
		lp->d_secpercyl = 0;
		goto nolabels;
		}
	if	(lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
		 dkcksum(lp))
		{
		printf("'%s' is either unlabeled or the label is corrupt.\n",
			buf);
		printf("Since the driver for '%s' supports disklabels you\n",
			buf);
		printf("must use the standalone 'disklabel' before making\n");
		printf("a filesystem on '%s'\n", buf);
		return;
		}
	if	(pp->p_fstype != FS_V71K)
		{
		printf("%s is not a 2.11BSD (FS_V71K) partition.", buf);
		return;
		}
	if	(pp->p_size == 0)
		{
		printf("%s is a zero length partition.\n", buf);
		return;
		}
nolabels:
	printf("file sys size [%D]: ", dbtofsb(pp->p_size));
	size = buf+128;
	gets(size);
	if	(size[0] == '\0')
		strcpy(size, ltoa(dbtofsb(pp->p_size)));
	if	(pp->p_size && atol(size) > pp->p_size)
		{
		printf("specified size larger than disklabel says.\n");
		return;
		}
	printf("bytes per inode [%u]: ", f_i);
	gets(buf);
	if	(buf[0])
		f_i = atoi(buf);
	printf("interleaving factor (m; %d default): ", f_m);
	gets(buf);
	if	(buf[0])
		f_m = atoi(buf);
	if	(lp->d_secpercyl)
		f_n = dbtofsb(lp->d_secpercyl);
	printf("interleaving modulus (n; %d default): ", f_n);
	gets(buf);
	if	(buf[0])
		f_n = atoi(buf);
#endif

	if	(f_n <= 0 || f_n >= MAXFN)
		f_n = MAXFN;
	if	(f_m <= 0 || f_m > f_n)
		f_m = 3;

	n = atol(size);
	if	(!n)
		{
		printf("Can't make zero length filesystem\n");
		return;
		}
	filsys.fs.fs_fsize = n;

/*
 * Calculate number of blocks of inodes as follows:
 *
 *	dbtob(n) = # of bytes in the filesystem
 *	dbtob(n) / f_i = # of inodes to allocate
 *	(dbtob(n) / f_i) / INOPB = # of fs blocks of inodes
 *	
 * Pretty - isn't it?
*/
	n = (dbtob(n) / f_i) / INOPB;
	if	(n <= 0)
		n = 1;
	if	(n > 65500/INOPB)
		n = 65500/INOPB;
	filsys.fs.fs_isize = n + 2;
	printf("isize = %D\n", n*INOPB);

	filsys.fs.fs_step = f_m;
	filsys.fs.fs_cyl = f_n;
	printf("m/n = %d %d\n", f_m, f_n);
	if	(filsys.fs.fs_isize >= filsys.fs.fs_fsize)
		{
		printf("%D/%D: bad ratio\n", filsys.fs.fs_fsize, filsys.fs.fs_isize-2);
		exit(1);
		}
	filsys.fs.fs_tfree = 0;
	filsys.fs.fs_tinode = 0;
	bzero(buf, DEV_BSIZE);
	for	(n=2; n!=filsys.fs.fs_isize; n++) {
		wtfs(n, buf);
		filsys.fs.fs_tinode += INOPB;
	}

	bflist();

	fsinit();

	filsys.fs.fs_time = utime;
	wtfs((long)SBLOCK, (char *)&filsys.fs);
	exit(0);
}

/*
 * initialize the file system
 */
struct inode node;
#define PREDEFDIR 3
struct direct root_dir[] = {
	{ ROOTINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
};
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 },
};

fsinit()
{
	register int i;

	/*
	 * initialize the node
	 */
	node.i_atime = utime;
	node.i_mtime = utime;
	node.i_ctime = utime;
	/*
	 * create the lost+found directory
	 */
	(void)makedir(lost_found_dir, 2);
	for (i = DIRBLKSIZ; i < DEV_BSIZE; i += DIRBLKSIZ)
		bcopy(&lost_found_dir[2], &buf[i], DIRSIZ(&lost_found_dir[2]));
	node.i_number = LOSTFOUNDINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = 2;
	node.i_size = DEV_BSIZE;
	node.i_db[0] = alloc();
	wtfs(node.i_db[0], buf);
	iput(&node);
	/*
	 * create the root directory
	 */
	node.i_number = ROOTINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = PREDEFDIR;
	node.i_size = makedir(root_dir, PREDEFDIR);
	node.i_db[0] = alloc();
	wtfs(node.i_db[0], buf);
	iput(&node);
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
makedir(protodir, entries)
	register struct direct *protodir;
	int entries;
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	for (cp = buf, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(&protodir[i]);
		bcopy(&protodir[i], cp, protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	bcopy(&protodir[i], cp, DIRSIZ(&protodir[i]));
	return (DIRBLKSIZ);
}

daddr_t
alloc()
{
	register int i;
	daddr_t bno;

	filsys.fs.fs_tfree--;
	bno = filsys.fs.fs_free[--filsys.fs.fs_nfree];
	if(bno == 0) {
		printf("out of free space\n");
		exit(1);
	}
	if(filsys.fs.fs_nfree <= 0) {
		rdfs(bno, (char *)&fbuf);
		filsys.fs.fs_nfree = fbuf.fb.df_nfree;
		for(i=0; i<NICFREE; i++)
			filsys.fs.fs_free[i] = fbuf.fb.df_free[i];
	}
	return(bno);
}

rdfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fsi, bno*DEV_BSIZE, 0);
	n = read(fsi, bf, DEV_BSIZE);
	if(n != DEV_BSIZE) {
		printf("read error: %ld\n", bno);
		exit(1);
	}
}

wtfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fso, bno*DEV_BSIZE, 0);
	n = write(fso, bf, DEV_BSIZE);
	if(n != DEV_BSIZE) {
		printf("write error: %D\n", bno);
		exit(1);
	}
}

bfree(bno)
daddr_t bno;
{
	register int i;

	if (bno != 0)
		filsys.fs.fs_tfree++;
	if(filsys.fs.fs_nfree >= NICFREE) {
		fbuf.fb.df_nfree = filsys.fs.fs_nfree;
		for(i=0; i<NICFREE; i++)
			fbuf.fb.df_free[i] = filsys.fs.fs_free[i];
		wtfs(bno, (char *)&fbuf);
		filsys.fs.fs_nfree = 0;
	}
	filsys.fs.fs_free[filsys.fs.fs_nfree++] = bno;
}

bflist()
{
	struct inode in;
	char flg[MAXFN];
	int adr[MAXFN];
	register int i, j;
	daddr_t f, d;

	bzero(flg, sizeof flg);
	i = 0;
	for(j=0; j<f_n; j++) {
		while(flg[i])
			i = (i+1)%f_n;
		adr[j] = i+1;
		flg[i]++;
		i = (i+f_m)%f_n;
	}

	bzero(&in, sizeof (in));
	in.i_number = 1;		/* inode 1 is a historical hack */
	in.i_mode = IFREG;
	bfree((daddr_t)0);
	d = filsys.fs.fs_fsize-1;
	while(d%f_n)
		d++;
	for(; d > 0; d -= f_n)
	for(i=0; i<f_n; i++) {
		f = d - adr[i];
		if(f < filsys.fs.fs_fsize && f >= filsys.fs.fs_isize)
			bfree(f);
	}
	iput(&in);
}

iput(ip)
register struct inode *ip;
{
	struct	dinode	buf[INOPB];
	register struct dinode *dp;
	daddr_t d;

	filsys.fs.fs_tinode--;
	d = itod(ip->i_number);
	if(d >= filsys.fs.fs_isize) {
		printf("ilist too small\n");
		return;
	}
	rdfs(d, buf);
	dp = (struct dinode *)buf;
	dp += itoo(ip->i_number);

	dp->di_addr[0] = ip->i_addr[0];
	dp->di_ic1 = ip->i_ic1;
	dp->di_ic2 = ip->i_ic2;
	wtfs(d, buf);
}

#ifndef	STANDALONE
usage()
	{
	printf("usage: [-s size] [-i bytes/ino] [-n num] [-m num] special\n");
	exit(1);
	/* NOTREACHED */
	}
#endif
