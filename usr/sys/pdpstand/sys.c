/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys.c	2.4 (2.11BSD) 1996/3/8
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../machine/seg.h"
#include "../machine/iopage.h"
#include "../machine/psl.h"
#include "../machine/machparam.h"
#include "saio.h"

#undef	KISA0
#define	KISA0	((u_short *) 0172340)
#define	KDSA0	((u_short *) 0172360)

extern	struct	devsw	devsw[];
extern	int	ssr3copy, bootctlr;
extern	long	atol();

/*
 * These buffers are only used in mapping inode blocks to disk sectors,
 * no good reason to make them global.
 *
 * The strange initialization is due to the fact that the 0'th case is
 * not legal since it would be used for quadruple indirect files.  The
 * use of the array 'b' is:
 *
 *	b[0] - not allocated, saving 1kb of space
 *	b[1] - triple indirect block
 *	b[2] - double indirect block
 *	b[3] - single indirect block
 *
 * In the 1kb filesystem double indirect files can be up to ~65 megabytes
 * (the old 512 byte filesystem double indirect files were up to ~8mb).
*/

static	char	tplind[DEV_BSIZE], dblind[DEV_BSIZE], sngind[DEV_BSIZE];

static	char	*b[4] = {
		NULL, 		/* j = 0 */
		tplind,		/* j = 1 */
		dblind,		/* j = 2 */
		sngind		/* j = 3 */
		};

static	daddr_t	blknos[4];

#define	NFILES	4
struct	iob	iob[NFILES];

ino_t dlook();
struct direct *readdir();

struct dirstuff {
	long loc;
	struct iob *io;
	};

/*
 * wfj - mods to trap to explicitly detail why we stopped
 */

static
openi(n, io)
	register ino_t n;
	register struct iob *io;
{
	register struct dinode *dp;
	daddr_t tdp;

	io->i_offset = 0;
	tdp = itod(n);
	io->i_bn = fsbtodb(tdp) + io->i_boff;
	io->i_cc = DEV_BSIZE;
	io->i_ma = io->i_buf;
	devread(io);

	dp = (struct dinode *)io->i_buf;
	dp = &dp[itoo(n)];
	io->i_ino.i_number = n;
	io->i_ino.i_mode = dp->di_mode;
	io->i_ino.i_size = dp->di_size;
	bcopy(dp->di_addr, io->i_ino.i_addr, NADDR * sizeof (daddr_t));
}

static
find(path, file)
	register char *path;
	struct iob *file;
{
	register char *q;
	register char c;
	ino_t n;

	if (path==NULL || *path=='\0') {
		printf("null path\n");
		return(0);
	}

	openi((ino_t) ROOTINO, file);
	while (*path) {
		while (*path == '/')
			path++;
		q = path;
		while (*q != '/' && *q != '\0')
			q++;
		c = *q;
		*q = '\0';
		if (q == path) path = "." ;	/* "/" means "/." */

		if ((n=dlook(path, file))!=0) {
			if (c=='\0')
				break;
			openi(n, file);
			*q = c;
			path = q;
			continue;
		} else {
			printf("%s not found\n", path);
			return(0);
		}
		/*NOTREACHED*/
	}
	return(n);
}

static daddr_t
sbmap(io, bn)
	register struct iob *io;
	daddr_t bn;
{
	register i;
	register struct inode *ip;
	int j, sh;
	daddr_t nb, *bap;

	ip = &io->i_ino;
	if (bn < 0) {
		printf("bn < 0\n");
		return((daddr_t)0);
	}

	/*
	 * blocks 0..NADDR-4 are direct blocks
	 */
	if (bn < NADDR-3) {
		i = bn;
		nb = ip->i_addr[i];
		return(nb);
	}

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for (j=3; j>0; j--) {
		sh += NSHIFT;
		nb <<= NSHIFT;
		if (bn < nb)
			break;
		bn -= nb;
	}
	if (j == 0) {
		printf("bn ovf %D\n", bn);
		return((daddr_t)0);
	}
/*
 * At this point:
 *
 *  j	  NADDR-j    meaning
 * ---    -------    -------
 *  0       7        illegal - tossed out in the 'if' above
 *  1       6        triple indirect block number
 *  2       5        double indirect block number
 *  3       4        single indirect block number
*/

	/*
	 * fetch the address from the inode
	 */
	nb = ip->i_addr[NADDR-j];
	if (nb == 0)
		goto bnvoid;

	/*
	 * fetch through the indirect blocks
	 */
	for (; j<=3; j++) {
		if (blknos[j] != nb) {
			io->i_bn = fsbtodb(nb) + io->i_boff;
			io->i_ma = b[j];
			io->i_cc = DEV_BSIZE;
			devread(io);
			blknos[j] = nb;
		}
		bap = (daddr_t *)b[j];
		sh -= NSHIFT;
		i = (bn>>sh) & NMASK;
		nb = bap[i];
		if (nb == 0) {
bnvoid:
			printf("bn void %D\n", bn);
			return((daddr_t)0);
		}
	}

	return(nb);
}

static ino_t
dlook(s, io)
	char *s;
	register struct iob *io;
{
	register struct direct *dp;
	register struct inode *ip;
	struct dirstuff dirp;
	int len;

	if (s==NULL || *s=='\0')
		return(0);
	ip = &io->i_ino;
	if ((ip->i_mode&IFMT) != IFDIR || !ip->i_size) {
		printf("%s: !directory,mode %o size: %D ip %o\n", s, 
			ip->i_mode, ip->i_size, ip);
		return(0);
	}

	len = strlen(s);
	dirp.loc = 0;
	dirp.io = io;
	for (dp = readdir(&dirp); dp; dp = readdir(&dirp)) {
		if (dp->d_ino == 0)
			continue;
		if (dp->d_namlen == len && !strcmp(s,dp->d_name))
			return(dp->d_ino);
	}
	return(0);
}

struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	register struct iob *io;
	daddr_t lbn, d, off;

	io = dirp->io;
	for (;;) {
		if (dirp->loc >= io->i_ino.i_size)
			return(NULL);
		off = blkoff(dirp->loc);
		if (off == 0) {
			lbn = lblkno(dirp->loc);
			d = sbmap(io, lbn);
			if (d == 0)
				return(NULL);
			io->i_bn = fsbtodb(d) + io->i_boff;
			io->i_ma = io->i_buf;
			io->i_cc = DEV_BSIZE;
			devread(io);
		}
		dp = (struct direct *)(io->i_buf + off);
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return(dp);
	}
}

/*
 * Modified to call the tape driver's seek routine.  This only works when
 * the record size is DEV_BSIZE (1kb).
*/
lseek(fdesc, addr, ptr)
	register int fdesc;
	off_t addr;
	int ptr;
{
	off_t	new;
	register struct iob *io;

	if (ptr != 0) {
		printf("lseek\n");
		return(-1);
	}
	fdesc -= 3;
	if	(fdesc < 0 || fdesc >= NFILES || 
			((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	new = addr / DEV_BSIZE;

	if	(io->i_flgs & F_TAPE)
		{
#ifdef	debug
		printf("seek: new=%D i_bn=%D\n", new, io->i_bn);
#endif
/*
 * Subtract 1 to acccount for the block currently in the buffer - the
 * tape is currently positioned at the next record.  Need to convert the 
 * block number from 512 byte to 1024 byte records (undo the fsbtodb).
*/
		devseek(io, (int)(new - dbtofsb(io->i_bn) - 1));
		}
	io->i_offset = addr;
	io->i_bn = fsbtodb(new) + io->i_boff;
	io->i_cc = 0;
	return(0);
}

/* if tape mark encountered, set flag  (in driver) */
int tapemark;

getc(fdesc)
	int fdesc;
{
	register struct iob *io;
	register unsigned char *p;
	register int c;
	int off;
	daddr_t tbn;

	if (fdesc >= 0 && fdesc <= 2)
		return(getchar());
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if (io->i_cc <= 0) {
		io->i_bn = fsbtodb(io->i_offset/(off_t)DEV_BSIZE);
		if (io->i_flgs&F_FILE) {
			tbn = sbmap(io, dbtofsb(io->i_bn));
			io->i_bn = fsbtodb(tbn) + io->i_boff;
		}
		io->i_ma = io->i_buf;
		io->i_cc = DEV_BSIZE;
#ifdef DEBUG
		printf("getc: fetch block %D, dev = %d\n", io->i_bn,
				io->i_ino.i_dev);
#endif
		tapemark = 0;
		devread(io);
		off = io->i_offset % (off_t)DEV_BSIZE;
		if (io->i_flgs&F_FILE) {
			if (io->i_offset+(DEV_BSIZE-off) >= io->i_ino.i_size)
				io->i_cc = io->i_ino.i_size - io->i_offset + off;
		}
		if	(tapemark)
			return(-1);
		io->i_cc -= off;
		if	(io->i_cc <= 0)
			return(-1);
		io->i_ma = &io->i_buf[off];
	}
	io->i_cc--;
	io->i_offset++;
	c = (int)(*(unsigned char *)io->i_ma++);
	return(c);
}
getw(fdesc)
	int fdesc;
{
	register w, i;
	register unsigned char *cp;
	int val;

	for (i = 0, val = 0, cp = (unsigned char *)&val; i < sizeof(val); i++) {
		w = getc(fdesc);
		if (w < 0) {
			if (i == 0)
				return(-1);
			else
				return(val);
		}
		*cp++ = w;
	}
	return(val);
}

read(fdesc, buf, count)
	register int fdesc;
	char *buf;
	int count;
{
	register i;
	register struct iob *file;

	if (fdesc >= 0 & fdesc <= 2) {
		i = count;
		do {
			*buf = getchar();
		} while (--i && *buf++ != '\n');
		return(count - i);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_READ) == 0)
		return(-1);
	if ((file->i_flgs&F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		i = devread(file);
		if	(i > 0)
			file->i_bn += (i / NBPG);
		return(i);
	}
	else {
		if (file->i_offset+count > file->i_ino.i_size)
			count = file->i_ino.i_size - file->i_offset;
		if ((i = count) <= 0)
			return(0);
		do {
			*buf++ = getc(fdesc+3);
		} while (--i);
		return(count);
	}
}

write(fdesc, buf, count)
	register int fdesc;
	char *buf;
	int count;
{
	register i;
	register struct iob *file;

	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		while (i--)
			putchar(*buf++);
		return(count);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_WRITE) == 0)
		return(-1);
	file->i_cc = count;
	file->i_ma = buf;
	i = devwrite(file);
	file->i_bn += (count / NBPG);
	return(i);
}

open(str, how)
	char *str;
	int how;
{
	register char *cp;
	int i, i2, i3;
	register struct iob *file;
	register struct devsw *dp;
	int fdesc;

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No file slots");
gotfile:
	(file = &iob[fdesc])->i_flgs |= F_ALLOC;

	for (cp = str; *cp && *cp != '('; cp++)
			;
	if (*cp != '(') {
		printf("Bad device\n");
		file->i_flgs = 0;
		return(-1);
	}
	*cp = '\0';
	for (dp = devsw; dp->dv_name; dp++) {
		if (strcmp(str, dp->dv_name) == 0)
			goto gotdev;
	}
	printf("Unknown device\n");
	file->i_flgs = 0;
	return(-1);
gotdev:
/*
 * The parse is a bit lengthier and complex now.  The syntax is now:
 *
 *	xx(ctlr, unit, partition)filename
 * or
 *	xx(unit, partition)filename
 *
 *
 * The controller number must be less than 4.  NOTE: many drivers
 * limit the number of controllers to 1 or 2.  If the controller is not 
 * specified it defaults to 0.
 *
 * The unit number must be less than 8.
 *
 * The partition number is also used to specify the tapefile to be loaded.  
 * When loading a tapefile the 'filename' must not be specified.  The partition
 * number must be less than 8.  This means that the number of standalone
 * programs is limited to 7.
*/

	*cp++ = '(';
	file->i_ino.i_dev = dp-devsw;
	for (i = 0; *cp >= '0' && *cp <= '9'; cp++) {
		i *= 10;
		i += (*cp - '0');
	}

	if (*cp++ != ',') {
badoff:
		printf("Bad offset or ctlr, unit, part out of bounds\n");
		file->i_flgs = 0;
		return(-1);
	}

	for	(i2 = 0; *cp >= '0' && *cp <= '9'; cp++)
		{
		i2 *= 10;
		i2 += (*cp - '0');
		}
/*
 * If we encounter a ')' now it means we have the two arg form 'ra(x,y)'.  If
 * a ',' is seen then we have the three arg form 'xp(x,y,z)'.  If neither a
 * ',' or ')' it is an error.
*/
	if	(*cp == ')')
		{
		file->i_ctlr = bootctlr;
		file->i_unit = i;
		file->i_part = i2;
		cp++;			/* skip ) */
		}
	else if	(*cp == ',')
		{
		for	(i3 = 0, cp++; *cp >= '0' && *cp <= '9'; cp++)
			{
			i3 *= 10;
			i3 += (*cp - '0');
			}
		file->i_ctlr = i;
		file->i_unit = i2;
		file->i_part = i3;
		if	(*cp++ != ')')
			goto badoff;
		}
	else
		goto badoff;
	if	(file->i_ctlr > 3 || file->i_unit > 7 || file->i_part > 7)
		goto badoff;

	if	(devopen(file) < 0)
		{
		file->i_flgs = 0;
		return(-1);
		}
	if (*cp == '\0') {
		file->i_flgs |= how+1;
		goto comret;
	}
	if ((i = find(cp, file)) == 0) {
		file->i_flgs = 0;
		return(-1);
	}
	if (how != 0) {
		printf("Can't write files\n");
		file->i_flgs = 0;
		return(-1);
	}
	openi(i, file);
	file->i_flgs |= F_FILE | (how+1);
comret:
	file->i_offset = 0;
	file->i_cc = 0;
	file->i_bn = 0;
	return(fdesc+3);
}

close(fdesc)
	register int fdesc;
{
	register struct iob *file;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_FILE) == 0)
		devclose(file);
	file->i_flgs = 0;
	return(0);
}

exit()
{
	_stop("Exit called");
}

_stop(s)
	char *s;
{
	printf("%s\n", s);
	_rtt();
}

extern char module[];

trap(r1, r0, nps, pc, ps)
	int nps, r1, r0, pc, ps;
{
	printf("Trap in %s,", module);
	switch (nps&7) {
	case 0:
		printf("bus error");
		break;
	case 1:
		printf("illegal instruction");
		break;
	case 2:
		printf("bpt/trace");
		break;
	case 3:
		printf("iot");
		break;
	case 4:
		printf("power fail");
		break;
	case 5:
		printf("emt");
		break;
	case 6:
		printf("sys/trap");
		break;
	default:
		printf("weird");
	}
	printf(" at loc %o\n", pc);
	printf("registers: r0=%o, r1=%o, ps=%o, nps=%o\n", r0, r1, ps, nps);
	for (;;)
		;
}

genopen(maxctlr, io)
	int maxctlr;
	struct iob *io;
	{
	register struct devsw *dp = &devsw[io->i_ino.i_dev];
	register char *cp;
	register int ctlr = io->i_ctlr;
	int csr;
	char line[64];

	if (ctlr >= maxctlr)
		return(-1);
	if (dp->dv_csr[ctlr])
		return(0);
	printf("%s%d csr[0%o]: ", dp->dv_name, ctlr, dp->dv_csr[ctlr]);
	gets(line);
	for (csr = 0, cp = line; *cp >= '0' && *cp <= '7'; cp++)
		csr = csr * 8 + (*cp - '0');
	if (csr == 0)
		return(-1);
	dp->dv_csr[ctlr] = (caddr_t *)csr;
/*
 * Tapes don't need this.  Disk drivers which support labels do not need
 * this either but disk drivers which do not support labels _do_ need this.
 * Doing it here is safe for everyone because label capable drivers will
 * load i_boff from the label _after_ calling this routine.
*/
	io->i_boff = 0;
	return(0);
	}

delay(i)
	int	i;
	{

	while	(i)
		i--;
	}

char *
ltoa(l)
	u_long l;
	{
	static char x[12];
	register char *cp = x + sizeof (x);

	do {
		*--cp = (l % 10) + '0';
		l /= 10;
	} while (l);
	return(cp);
	}

char *
itoa(i)
	register int i;
	{
	static char x[8];
	register char *cp = x + sizeof (x);

	do {
		*--cp = (i % 10) + '0';
		i /= 10;
	} while (i);
	return(cp);
	}

/*
 * Convert a 16 bit (virtual) address into a 18 bit (UNIBUS) address.
 * The address extension bits (bits 16, 17) are placed into 'bae' while
 * the lower order bits (bits 0 - 15) are placed in 'lo16'.
 *
 * This routine was primarily created to handle split I/D kernel mode
 * utilities.  Boot itself must run non split I/D and runs in user mode.
 * A side effect of this routine is that Boot no longer must be loaded at
 * a 64Kb boundary.
 *
 * Everything (Boot and standalone utilities) must still run in the low
 * 248kb of memory which is mapped by the UNIBUS mapping registers.
*/

iomapadr(buf, bae, lo16)
	memaddr	buf;
	u_short *bae, *lo16;
	{
	u_long  uadr;
	int	curmode = *PS & PSL_CURMOD, segno;
	u_int	nb, ts;
	extern	char	module[];

	if	(curmode == 0)
		{		/* Kernel mode - we're in a utility */
/*
 * For split I/D we need to emulate the kernel's method of looking at
 * the segment registers and calculating the physical address.  Whew, forgot
 * that ;-)
*/
		nb = ((int)buf >> 6) & 01777;
		segno = nb >> 7;
		if	(ssr3copy & 4)
			ts = KDSA0[segno];	/* kernel I/D is on */
		else
			ts = KISA0[segno];	/* kernel I/D is off */
		ts += (nb & 0177);
		uadr = ((u_long)ts << 6) + (buf & 077);
		}
	else if	(curmode == PSL_CURMOD)
		{		/* User mode - we're in Boot */
		uadr = UISA[0];			/* No I/D for Boot! */
		uadr <<= 6;			/* clicks to bytes */
		uadr += buf;
		}
	else
		_stop("Bad PSL mode");
	
	*bae = (uadr >> 16) & 3;		/* 18 bit mode */
	*lo16 = uadr & 0xffff;
	}
