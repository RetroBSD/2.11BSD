/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fs.h	1.3 (2.11BSD GTE) 1995/12/24
 */

#ifndef	_SYS_FS_H_
#define	_SYS_FS_H_

/*
 * The root inode is the root of the file system.
 * Inode 0 can't be used for normal purposes and
 * historically bad blocks were linked to inode 1,
 * thus the root inode is 2. (inode 1 is no longer used for
 * this purpose, however numerous dump tapes make this
 * assumption, so we are stuck with it)
 * The lost+found directory is given the next available
 * inode when it is created by ``mkfs''.
 */
#define BBSIZE		DEV_BSIZE
#define SBSIZE		DEV_BSIZE
#define	BBLOCK		((daddr_t)(0))
#define	SBLOCK		((daddr_t)(BBLOCK + BBSIZE / DEV_BSIZE))

#define	SUPERB		((daddr_t)1)	/* block number of the super block */
#define	ROOTINO		((ino_t)2)	/* i number of all roots */
#define	LOSTFOUNDINO	(ROOTINO + 1)

#define	NICINOD		100		/* number of superblock inodes */
#define	NICFREE		50		/* number of superblock free blocks */

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in 
 * the super block for this name.
 */
#define MAXMNTLEN 12

/*
 * Super block for a file system.  NOTE:  The 'fs_flock' and 'fs_ilock'
 * fields MUST be on an even byte boundary because they are used as sleep()
 * channels and odd values specify a network sleep().
 */
struct	fs
{
	u_short	fs_isize;		/* first block after i-list */
	daddr_t	fs_fsize;		/* size in blocks of entire volume */
	short	fs_nfree;		/* number of addresses in fs_free */
	daddr_t	fs_free[NICFREE];	/* free block list */
	short	fs_ninode;		/* number of inodes in fs_inode */
	ino_t	fs_inode[NICINOD];	/* free inode list */
	char	fs_flock;		/* lock during free list manipulation */
	char	fs_fmod;		/* super block modified flag */
	char	fs_ilock;		/* lock during i-list manipulation */
	char	fs_ronly;		/* mounted read-only flag */
	time_t	fs_time;		/* last super block update */
	daddr_t	fs_tfree;		/* total free blocks */
	ino_t	fs_tinode;		/* total free inodes */
	short	fs_step;		/* optimal step in free list pattern */
	short	fs_cyl;			/* number of blocks per pattern */
	char	fs_fsmnt[MAXMNTLEN];	/* ordinary file mounted on */
	ino_t	fs_lasti;		/* start place for circular search */
	ino_t	fs_nbehind;		/* est # free inodes before s_lasti */
	u_short	fs_flags;		/* mount time flags */
/* actually longer */
};

struct	fblk {
	short	df_nfree;		/* number of addresses in df_free */
	daddr_t	df_free[NICFREE];	/* free block list */
};

/*
 * Turn file system block numbers into disk block addresses.
 * This maps file system blocks to device size blocks.
 */
#define	fsbtodb(b)	((daddr_t)((daddr_t)(b)<<1))
#define	dbtofsb(b)	((daddr_t)((daddr_t)(b)>>1))

/*
 * Macros for handling inode numbers:
 *     inode number to file system block offset.
 *     inode number to file system block address.
 */
#define	itoo(x)		((int)(((x) + 2 * INOPB - 1) % INOPB))
#define	itod(x)		((daddr_t)((((u_int)(x) + 2 * INOPB - 1) / INOPB)))

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#define blkoff(loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & DEV_BMASK)
#define lblkno(loc)		/* calculates (loc / fs->fs_bsize) */ \
	((loc) >> DEV_BSHIFT)

/*
 * Determine the number of available blocks given a
 * percentage to hold in reserve
 */
#define freespace(fs, percentreserved) \
	((fs)->fs_tfree - ((fs)->fs_fsize - \
	(fs)->fs_isize) * (percentreserved) / 100)

/*
 * INOPB is the number of inodes in a secondary storage block.
 */
#define INOPB	16			/* MAXBSIZE / sizeof(dinode) */

/*
 * NINDIR is the number of indirects in a file system block.
 */
#define	NINDIR		(DEV_BSIZE / sizeof(daddr_t))
#define	NSHIFT		8		/* log2(NINDIR) */
#define	NMASK		0377L		/* NINDIR - 1 */

/*
 * We continue to implement pipes within the file system because it would
 * be pretty tough for us to handle 10 4K blocked pipes on a 1M machine.
 *
 * 4K is the allowable buffering per write on a pipe.  This is also roughly
 * the max size of the file created to implement the pipe.  If this size is
 * bigger than 4096, pipes will be implemented with large files, which is
 * probably not good.
 */
#define	MAXPIPSIZ	(NDADDR * MAXBSIZE)

#if defined(KERNEL) && !defined(SUPERVISOR)
struct	fs *getfs();
struct	fs *mountfs();
#endif
#endif /* _SYS_FS_H_ */
