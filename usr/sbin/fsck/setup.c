/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)setup.c	5.3.1 (2.11BSD) 1996/2/3";
#endif not lint

#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include "fsck.h"

setup(dev)
	char *dev;
{
	dev_t rootdev;
	off_t smapsz, totsz, lncntsz;
	daddr_t bcnt, nscrblk;
	struct stat statb;
	u_int msize;
	char *mbase;
	int i, j, n;
	long size;
	BUFAREA *bp;
	char junk[80 + sizeof (".XXXXX") + 1];

	if (stat("/", &statb) < 0)
		errexit("Can't stat root\n");
	rootdev = statb.st_dev;
	if (stat(dev, &statb) < 0) {
		printf("Can't stat %s\n", dev);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK)
		;
	else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else {
		if (reply("file is not a block or character device; OK") == 0)
			return (0);
	}
	if (rootdev == statb.st_rdev)
		hotroot++;
	if ((dfile.rfdes = open(dev, 0)) < 0) {
		printf("Can't open %s\n", dev);
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (dfile.wfdes = open(dev, 1)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	dfile.mod = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&fileblk);
	initbarea(&inoblk);
	/*
	 * Read in the super block and its summary info.
	 */
	if (bread(&dfile, (char *)&sblock, SBLOCK, SBSIZE) != 0)
		return (0);
	sblk.b_bno = SBLOCK;
	sblk.b_size = SBSIZE;

	imax = ((ino_t)sblock.fs_isize - (SUPERB+1)) * INOPB;
	fmin = (daddr_t)sblock.fs_isize;	/* first data blk num */
	fmax = sblock.fs_fsize;		/* first invalid blk num */
	startib = fmax;
	if(fmin >= fmax || 
		(imax/INOPB) != ((ino_t)sblock.fs_isize-(SUPERB+1))) {
		pfatal("Size check: fsize %ld isize %d",
			sblock.fs_fsize,sblock.fs_isize);
		printf("\n");
		ckfini();
		return(0);
	}
	if (preen == 0)
		printf("File System: %.12s\n\n", sblock.fs_fsmnt);
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsz = roundup(howmany(fmax,BITSPB),sizeof(*lncntp));
	smapsz = roundup(howmany((long)(imax+1),STATEPB),sizeof(*lncntp));
	lncntsz = (long)(imax+1) * sizeof(*lncntp);
	if(bmapsz > smapsz+lncntsz)
		smapsz = bmapsz-lncntsz;
	totsz = bmapsz+smapsz+lncntsz;
	msize = memsize;
	mbase = membase;
	bzero(mbase,msize);
	muldup = enddup = duplist;
	zlnp = zlnlist;

	if((off_t)msize < totsz) {
		bmapsz = roundup(bmapsz,DEV_BSIZE);
		smapsz = roundup(smapsz,DEV_BSIZE);
		lncntsz = roundup(lncntsz,DEV_BSIZE);
		nscrblk = (bmapsz+smapsz+lncntsz)>>DEV_BSHIFT;
		if(scrfile[0] == 0) {
			pfatal("\nNEED SCRATCH FILE (%ld BLKS)\n",nscrblk);
			do {
				printf("ENTER FILENAME:  ");
				if((n = getline(stdin, scrfile, 
						sizeof(scrfile) - 6)) == EOF)
					errexit("\n");
			} while(n == 0);
		}
		strcpy(junk, scrfile);
		strcat(junk, ".XXXXX");
		sfile.wfdes = mkstemp(junk);
		if ((sfile.wfdes < 0)
		    || ((sfile.rfdes = open(junk,0)) < 0)) {
			printf("Can't create %s\n", junk);
			ckfini();
			return(0);
		}
		unlink(junk);	/* make it invisible incase we exit */
		if (hotroot && (fstat(sfile.wfdes,&statb)==0)
		    && ((statb.st_mode & S_IFMT) == S_IFREG)
		    && (statb.st_dev==rootdev))
		     pfatal("TMP FILE (%s) ON ROOT WHEN CHECKING ROOT", junk);
		bp = &((BUFAREA *)mbase)[(msize/sizeof(BUFAREA))];
		poolhead = NULL;
		while(--bp >= (BUFAREA *)mbase) {
			initbarea(bp);
			bp->b_next = poolhead;
			poolhead = bp;
		}
		bp = poolhead;
		for(bcnt = 0; bcnt < nscrblk; bcnt++) {
			bp->b_bno = bcnt;
			dirty(bp);
			flush(&sfile,bp);
		}
		blockmap = freemap = statemap = (char *) NULL;
		lncntp = (short *) NULL;
		bmapblk = 0;
		smapblk = bmapblk + bmapsz / DEV_BSIZE;
		lncntblk = smapblk + smapsz / DEV_BSIZE;
		fmapblk = smapblk;
	}
	else {
		poolhead = NULL;
		blockmap = mbase;
		statemap = &mbase[bmapsz];
		freemap = statemap;
		lncntp = (short *)&statemap[smapsz];
	}
	return(1);
}
