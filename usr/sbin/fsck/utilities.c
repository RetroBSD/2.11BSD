/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)utilities.c	5.2 (Berkeley) 9/10/85";
#endif not lint

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include "fsck.h"

long	lseek();

ftypeok(dp)
	DINODE *dp;
{
	switch (dp->di_mode & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", dp->di_mode);
		return (0);
	}
}

reply(s)
	char *s;
{
	char line[80];

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	printf("\n%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		printf(" no\n\n");
		return (0);
	}
	if (yflag) {
		printf(" yes\n\n");
		return (1);
	}
	if (getline(stdin, line, sizeof(line)) == EOF)
		errexit("\n");
	printf("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	else
		return (0);
}

getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (EOF);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}

BUFAREA *
getblk(bp, blk)
	register BUFAREA *bp;
	daddr_t blk;
{
	register struct filecntl *fcp;
	extern BUFAREA *search();

	if(bp == NULL) {
		bp = search(blk);
		fcp = &sfile;
	}
	else
		fcp = &dfile;
	if(bp->b_bno == blk)
		return(bp);
	flush(fcp,bp);
	bp->b_errs = bread(fcp,bp->b_un.b_buf,blk,DEV_BSIZE);
	bp->b_bno = blk;
	bp->b_size = DEV_BSIZE;
	return(bp);
}

flush(fcp, bp)
	struct filecntl *fcp;
	register BUFAREA *bp;
{

	if (!bp->b_dirty)
		return;
	if (bp->b_errs != 0)
		pfatal("WRITING ZERO'ED BLOCK %ld TO DISK\n", bp->b_bno);
	bp->b_dirty = 0;
	bp->b_errs = 0;
	if (bp == &inoblk)
		bwrite(fcp, inobuf, startib, NINOBLK * DEV_BSIZE);
	else
		bwrite(fcp, bp->b_un.b_buf, bp->b_bno, DEV_BSIZE);
}

rwerr(s, blk)
	char *s;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %ld", s, blk);
	if (reply("CONTINUE") == 0)
		errexit("Program terminated\n");
}

ckfini()
{

	flush(&dfile, &fileblk);
	flush(&dfile, &sblk);
	if (sblk.b_bno != SBLOCK) {
		sblk.b_bno = SBLOCK;
		sbdirty();
		flush(&dfile, &sblk);
	}
	flush(&dfile, &inoblk);
	if (dfile.rfdes)
		(void)close(dfile.rfdes);
	if (dfile.wfdes)
		(void)close(dfile.wfdes);
	if (sfile.rfdes)
		(void)close(sfile.rfdes);
	if (sfile.wfdes)
		(void)close(sfile.wfdes);
}

bread(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	int size;
{
	char *cp;
	register int i, errs;

	if (lseek(fcp->rfdes, blk << DEV_BSHIFT, 0) < 0)
		rwerr("SEEK", blk);
	else if (read(fcp->rfdes, buf, size) == size)
		return (0);
	rwerr("READ", blk);
	if (lseek(fcp->rfdes, blk << DEV_BSHIFT, 0) < 0)
		rwerr("SEEK", blk);
	errs = 0;
	pfatal("THE FOLLOWING SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE) {
		if (read(fcp->rfdes, cp, DEV_BSIZE) < 0) {
			printf(" %ld,", blk + i / DEV_BSIZE);
			bzero(cp, DEV_BSIZE);
			errs++;
		}
	}
	printf("\n");
	return (errs);
}

bwrite(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	int size;
{
	int i;
	char *cp;

	if (fcp->wfdes < 0)
		return;
	if (lseek(fcp->wfdes, blk << DEV_BSHIFT, 0) < 0)
		rwerr("SEEK", blk);
	else if (write(fcp->wfdes, buf, size) == size) {
		fcp->mod = 1;
		return;
	}
	rwerr("WRITE", blk);
	if (lseek(fcp->wfdes, blk << DEV_BSHIFT, 0) < 0)
		rwerr("SEEK", blk);
	pfatal("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE)
		if (write(fcp->wfdes, cp, DEV_BSIZE) < 0)
			printf(" %ld,", blk + i / DEV_BSIZE);
	printf("\n");
	return;
}

/*
 * allocate a data block
 */
daddr_t
allocblk()
{
	daddr_t i;

	for (i = 0; i < fmax; i++) {
		if (getbmap(i))
			continue;
		setbmap(i);
		n_blks++;
		return (i);
	}
	return (0);
}

/*
 * Free a previously allocated block
 */
fr_blk(blkno)
	daddr_t blkno;
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	pass4check(&idesc);
}

/*
 * Find a pathname
 */
getpathname(namebuf, curdir, ino)
	char *namebuf;
	ino_t curdir, ino;
{
	int len, st;
	register char *cp;
	struct inodesc idesc;
	extern int findname();

	st = getstate(ino);
	if (st != DSTATE && st != DFOUND) {
		strcpy(namebuf, "?");
		return;
	}
	bzero(&idesc, sizeof(struct inodesc));
	idesc.id_type = DATA;
	cp = &namebuf[MAXPATHLEN - 1];
	*cp-- = '\0';
	if (curdir != ino) {
		idesc.id_parent = curdir;
		goto namelookup;
	}
	while (ino != ROOTINO) {
		idesc.id_number = ino;
		idesc.id_func = findino;
		idesc.id_name = "..";
		if ((ckinode(ginode(ino), &idesc) & STOP) == 0)
			break;
	namelookup:
		idesc.id_number = idesc.id_parent;
		idesc.id_parent = ino;
		idesc.id_func = findname;
		idesc.id_name = namebuf;
		if ((ckinode(ginode(idesc.id_number), &idesc) & STOP) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		if (cp < &namebuf[MAXNAMLEN])
			break;
		bcopy(namebuf, cp, len);
		*--cp = '/';
		ino = idesc.id_number;
	}
	if (ino != ROOTINO) {
		strcpy(namebuf, "?");
		return;
	}
	bcopy(cp, namebuf, &namebuf[MAXPATHLEN] - cp);
}

catch()
{

	ckfini();
	exit(12);
}

/*
 * When preening, allow a single quit to signal
 * a special exit after filesystem checks complete
 * so that reboot sequence may be interrupted.
 */
catchquit()
{
	extern returntosingle;

	printf("returning to single-user after filesystem check\n");
	returntosingle = 1;
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
voidquit()
{

	sleep(1);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * determine whether an inode should be fixed.
 */
dofix(idesc, msg)
	register struct inodesc *idesc;
	char *msg;
{

	switch (idesc->id_fix) {

	case DONTKNOW:
		if (idesc->id_type == DATA)
			direrr(idesc->id_number, msg);
		else
			pwarn(msg);
		if (preen) {
			printf(" (SALVAGED)\n");
			idesc->id_fix = FIX;
			return (ALTERED);
		}
		if (reply("SALVAGE") == 0) {
			idesc->id_fix = NOFIX;
			return (0);
		}
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		return (ALTERED);

	case NOFIX:
		return (0);

	default:
		errexit("UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
	}
	/* NOTREACHED */
}

/* VARARGS1 */
errexit(s1, s2, s3, s4)
	char *s1;
{
	printf(s1, s2, s3, s4);
	exit(8);
}

/*
 * An inconsistency occured which shouldn't during normal operations.
 * Die if preening, otherwise just printf.
 */
/* VARARGS1 */
pfatal(s, a1, a2, a3, a4)
	char *s;
{

	if (preen) {
		printf("%s: ", devname);
		printf(s, a1, a2, a3, a4);
		printf("\n");
		printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n",
			devname);
		exit(8);
	}
	printf(s, a1, a2, a3, a4);
}

/*
 * Pwarn is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS1 */
pwarn(s, a1, a2, a3, a4, a5, a6)
	char *s;
{

	if (preen)
		printf("%s: ", devname);
	printf(s, a1, a2, a3, a4, a5, a6);
}

stype(p)
register char *p;
{
	if(*p == 0)
		return;
	if (*(p+1) == 0) {
		if (*p == '3') {
			cylsize = 200;
			stepsize = 5;
			return;
		}
		if (*p == '4') {
			cylsize = 418;
			stepsize = 9;
			return;
		}
	}
	cylsize = atoi(p);
	while(*p && *p != ':')
		p++;
	if(*p)
		p++;
	stepsize = atoi(p);
	if(stepsize <= 0 || stepsize > cylsize ||
	cylsize <= 0 || cylsize > MAXCYL) {
		printf("Invalid -s argument, defaults assumed\n");
		cylsize = stepsize = 0;
	}
}

dostate(ino, s,flg)
	ino_t ino;
	int s, flg;
{
	register char *p;
	register unsigned byte, shift;
	BUFAREA *bp;

	byte = ino / STATEPB;
	shift = LSTATE * (ino % STATEPB);
	if(statemap != NULL) {
		bp = NULL;
		p = &statemap[byte];
	}
	else {
		bp = getblk(NULL,(daddr_t)(smapblk+(byte/DEV_BSIZE)));
		if (bp->b_errs)
			errexit("Fatal I/O error\n");
		else
			p = &bp->b_un.b_buf[byte%DEV_BSIZE];
	}
	switch(flg) {
		case 0:
			*p &= ~(SMASK<<(shift));
			*p |= s << shift;
			if(bp != NULL)
				dirty(bp);
			return(s);
		case 1:
			return((*p >> shift) & SMASK);
	}
	return(USTATE);
}

domap(blk,flg)
daddr_t blk;
int flg;
{
	register char *p;
	register unsigned n;
	register BUFAREA *bp;
	off_t byte;

	byte = blk >> BITSHIFT;
	n = 1<<((unsigned)(blk & BITMASK));
	if(flg & 04) {
		p = freemap;
		blk = fmapblk;
	}
	else {
		p = blockmap;
		blk = bmapblk;
	}
	if(p != NULL) {
		bp = NULL;
		p += (unsigned)byte;
	}
	else {
		bp = getblk((BUFAREA *)NULL,blk+(byte>>DEV_BSHIFT));
		if (bp->b_errs)
			errexit("Fatal I/O error\n");
		else
			p = &bp->b_un.b_buf[(unsigned)(byte&DEV_BMASK)];
	}
	switch(flg&03) {
		case 0:
			*p |= n;
			break;
		case 1:
			n &= *p;
			bp = NULL;
			break;
		case 2:
			*p &= ~n;
	}
	if(bp != NULL)
		dirty(bp);
	return(n);
}

dolncnt(ino, flg, val)
ino_t ino;
short val, flg;
{
	register short *sp;
	register BUFAREA *bp;

	if(lncntp != NULL) {
		bp = NULL;
		sp = &lncntp[ino];
	}
	else {
		bp = getblk((BUFAREA *)NULL,(daddr_t)(lncntblk+(ino/SPERB)));
		if (bp->b_errs)
			errexit("Fatal I/O error\n");
		else
			sp = &bp->b_un.b_lnks[ino%SPERB];
	}
	switch(flg) {
		case 0:
			*sp = val;
			break;
		case 1:
			bp = NULL;
			break;
		case 2:
			(*sp)--;
			break;
		case 3:
			(*sp)++;
			break;
		default:
			abort();
	}
	if(bp != NULL)
		dirty(bp);
	return(*sp);
}

BUFAREA *
search(blk)
daddr_t blk;
{
	register BUFAREA *pbp, *bp;

	for(bp = (BUFAREA *) &poolhead; bp->b_next; ) {
		pbp = bp;
		bp = pbp->b_next;
		if(bp->b_bno == blk)
			break;
	}
	pbp->b_next = bp->b_next;
	bp->b_next = poolhead;
	poolhead = bp;
	return(bp);
}
