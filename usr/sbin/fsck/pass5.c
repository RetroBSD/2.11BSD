/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)pass5.c	5.2 (Berkeley) 3/5/86";
#endif not lint

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "fsck.h"

pass5()
{
struct	inodesc idesc;
	daddr_t	blkno;
	int	n;
	BUFAREA	*bp1, *bp2;
	extern int debug;

	bzero(&idesc, sizeof (idesc));
	idesc.id_type = ADDR;

	if(sflag)
		fixfree = 1;
	else {
		ifreechk();
		if(freemap)
			bcopy(blockmap, freemap, (int)bmapsz);
		else {
			for(blkno = 0; blkno < fmapblk-bmapblk; blkno++) {
				bp1 = getblk((BUFAREA *)NULL,blkno+bmapblk);
				bp2 = getblk((BUFAREA *)NULL,blkno+fmapblk);
				bcopy(bp1->b_un.b_buf,bp2->b_un.b_buf,DEV_BSIZE);
				dirty(bp2);
			}
		}
		badblk = dupblk = 0;
		freeblk.df_nfree = sblock.fs_nfree;
		for(n = 0; n < NICFREE; n++)
			freeblk.df_free[n] = sblock.fs_free[n];
		freechk();
		if(badblk)
			pfatal("%d BAD BLKS IN FREE LIST\n",badblk);
		if(dupblk)
			pwarn("%d DUP BLKS IN FREE LIST\n",dupblk);
		if (debug)
			printf("n_files %ld n_blks %ld n_free %ld fmax %ld fmin %ld ninode: %u\n",
				n_files, n_blks, n_free, fmax, fmin,sblock.fs_ninode);
		if(fixfree == 0) {
			if ((n_blks+n_free) != (fmax-fmin) && 
					dofix(&idesc, "BLK(S) MISSING"))
				fixfree = 1;
			else if (n_free != sblock.fs_tfree && 
			   dofix(&idesc,"FREE BLK COUNT WRONG IN SUPERBLOCK")) {
					sblock.fs_tfree = n_free;
					sbdirty();
				}
		}
		if(fixfree && !dofix(&idesc, "BAD FREE LIST"))
				fixfree = 0;
	}

	if(fixfree) {
		if (preen == 0)
			printf("** Phase 6 - Salvage Free List\n");
		makefree();
		n_free = sblock.fs_tfree;
		sblock.fs_ronly = 0;
		sblock.fs_fmod = 0;
		sbdirty();
	}
}

ifreechk() {
	register int i;
	ino_t	inum;

	for (i=0; i<sblock.fs_ninode; i++) {
		inum = sblock.fs_inode[i];
		switch (getstate(inum)) {

		case USTATE:
			continue;
		default:
			pwarn("ALLOCATED INODE(S) IN IFREE LIST");
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sblock.fs_ninode = i-1;
				sbdirty();
			}
			return;
		}
	}
}

freechk()
{
	register daddr_t *ap;

	if(freeblk.df_nfree == 0)
		return;
	do {
		if(freeblk.df_nfree <= 0 || freeblk.df_nfree > NICFREE) {
			pfatal("BAD FREEBLK COUNT");
			printf("\n");
			fixfree = 1;
			return;
		}
		ap = &freeblk.df_free[freeblk.df_nfree];
		while(--ap > &freeblk.df_free[0]) {
			if(pass5check(*ap) == STOP)
				return;
		}
		if(*ap == (daddr_t)0 || pass5check(*ap) != KEEPON)
			return;
	} while(getblk(&fileblk,*ap) != NULL);
}


makefree()
{
	register i, cyl, step;
	int j;
	char flg[MAXCYL];
	short addr[MAXCYL];
	daddr_t blk, baseblk;

	sblock.fs_nfree = 0;
	sblock.fs_flock = 0;
	sblock.fs_fmod = 0;
	sblock.fs_tfree = 0;
	sblock.fs_ninode = 0;
	sblock.fs_ilock = 0;
	sblock.fs_ronly = 0;
	if(cylsize == 0 || stepsize == 0) {
		step = sblock.fs_step;
		cyl = sblock.fs_cyl;
	}
	else {
		step = stepsize;
		cyl = cylsize;
	}
	if(step > cyl || step <= 0 || cyl <= 0 || cyl > MAXCYL) {
		printf("Default free list spacing assumed\n");
		step = STEPSIZE;
		cyl = CYLSIZE;
	}
	sblock.fs_step = step;
	sblock.fs_cyl = cyl;
	bzero(flg,sizeof(flg));
	i = 0;
	for(j = 0; j < cyl; j++) {
		while(flg[i])
			i = (i + 1) % cyl;
		addr[j] = i + 1;
		flg[i]++;
		i = (i + step) % cyl;
	}
	baseblk = (daddr_t)roundup(fmax,cyl);
	bzero((char *)&freeblk,DEV_BSIZE);
	freeblk.df_nfree++;
	for( ; baseblk > 0; baseblk -= cyl)
		for(i = 0; i < cyl; i++) {
			blk = baseblk - addr[i];
			if(!outrange(blk) && !getbmap(blk)) {
				sblock.fs_tfree++;
				if(freeblk.df_nfree >= NICFREE) {
					fbdirty();
					fileblk.b_bno = blk;
					flush(&dfile,&fileblk);
					bzero((char *)&freeblk,DEV_BSIZE);
				}
				freeblk.df_free[freeblk.df_nfree] = blk;
				freeblk.df_nfree++;
			}
		}
	sblock.fs_nfree = freeblk.df_nfree;
	for(i = 0; i < NICFREE; i++)
		sblock.fs_free[i] = freeblk.df_free[i];
	sbdirty();
}

pass5check(blk)
daddr_t blk;
{
	if(outrange(blk)) {
		fixfree = 1;
		if (preen)
			pfatal("BAD BLOCKS IN FREE LIST.");
		if(++badblk >= MAXBAD) {
			printf("EXCESSIVE BAD BLKS IN FREE LIST.");
			if(reply("CONTINUE") == 0)
				errexit("");
			return(STOP);
		}
		return(SKIP);
	}
	if(getfmap(blk)) {
		fixfree = 1;
		if(++dupblk >= DUPTBLSIZE) {
			printf("EXCESSIVE DUP BLKS IN FREE LIST.");
			if(reply("CONTINUE") == 0)
				errexit("");
			return(STOP);
		}
	}
	else {
		n_free++;
		setfmap(blk);
	}
	return(KEEPON);
}
