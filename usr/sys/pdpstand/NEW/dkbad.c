/*
 * SCCSID: @(#)dkbad.c	3.0	4/21/86
 */
/*
 *		LICENSED FROM DIGITAL EQUIPMENT CORPORATION
 *			       COPYRIGHT (c)
 *		       DIGITAL EQUIPMENT CORPORATION
 *			  MAYNARD, MASSACHUSETTS
 *			      1985, 1986, 1987
 *			   ALL RIGHTS RESERVED
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
 *	NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
 *	EQUIPMENT CORPORATION.  DIGITAL MAKES NO REPRESENTATIONS ABOUT
 *	THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  IT IS
 *	SUPPLIED "AS IS" WITHOUT EXPRESSED OR IMPLIED WARRANTY.
 *
 *	IF THE REGENTS OF THE UNIVERSITY OF CALIFORNIA OR ITS LICENSEES
 *	MODIFY THE SOFTWARE IN A MANNER CREATING DERIVATIVE COPYRIGHT
 *	RIGHTS, APPROPRIATE COPYRIGHT LEGENDS MAY BE PLACED ON THE
 *	DERIVATIVE WORK IN ADDITION TO THAT SET FORTH ABOVE.
 */

/*
 * ULTRIX-11	Standalone Common Bad Sector Code
 *
 * Jerry Brenner	12/20/82
 * Fred Canter		7/4/84 (modified for multiple dk_bad structures)
 */

#include <sys/param.h>
#include <sys/bads.h>
#include <sys/inode.h>
#include "saio.h"

struct dkres dkr[2];
/*
 * Search the bad sector table looking for
 * the specified sector.  Return index if found.
 * Return -1 if not found.
 */

isbad(bt, cyl, trk, sec)
	register struct dkbad *bt;
{
	register int i;
	register long blk, bblk;

	blk = ((long)cyl << 16) + (trk << 8) + sec;
	for (i = 0; i < 126; i++) {
		bblk=((long)bt->bt_badb[i].bt_cyl<<16)+bt->bt_badb[i].bt_trksec;
		if (blk == bblk)
			return (i);
		if (blk < bblk || bblk < 0)
			break;
	}
	return (-1);
}

fixbad(io, wcnt, flag, dkn)
struct iob *io;
unsigned wcnt;
int flag, dkn;
{
	int scnt, rcnt, tcnt;
	long tadr;

	tcnt = -(io->i_cc>>1);
	scnt = (wcnt - tcnt)<<1;
	scnt = (scnt/512)*512;
	rcnt = io->i_cc - scnt;
	tadr = (((long)segflag)<<16) + io->i_ma;
	tadr += scnt;
	if(flag){
		dkr[dkn].r_vma = tadr&0177777;
		dkr[dkn].r_vxm = tadr >> 16;
		if(rcnt > 512){
			dkr[dkn].r_cc = rcnt - 512;
			dkr[dkn].r_vcc = 512;
			tadr += 512;
			dkr[dkn].r_ma = tadr&0177777;
			dkr[dkn].r_xm = tadr >> 16;
			dkr[dkn].r_bn = io->i_bn +(scnt/512)+1;
		} else {
			dkr[dkn].r_vcc = rcnt<=0?512:rcnt;
			dkr[dkn].r_cc = 0;
			dkr[dkn].r_ma = 0;
		}
	}else{
		dkr[dkn].r_ma = tadr&0177777;
		dkr[dkn].r_xm = tadr >> 16;
		dkr[dkn].r_cc = rcnt;
		dkr[dkn].r_vcc = scnt;
		dkr[dkn].r_bn = io->i_bn + (scnt/512);
	}
}

/*
 * Allocate a bads structure for the disk,
 * or return index into dk_badf[] if one
 * allready allocated.
 */
extern	int	dk_badf[];

dkn_set(io)
struct iob *io;
{
	register int i;

	/* form sort of a major/minor device number */
	i = (io->i_ino.i_dev << 8) | io->i_unit;
	if(dk_badf[0] == i)
		return(0);
	else if(dk_badf[1] == i)
		return(1);
	else if(dk_badf[0] == 0) {
		dk_badf[0] = i;
		return(0);
	} else if(dk_badf[1] == 0) {
		dk_badf[1] = i;
		return(1);
	} else
		return(-1);
}

/*
 * This routine is called by the HP and HK drivers
 * to do ECC error correction on data in the user's buffer.
 * The problem is that the buffer is not mapped into the same
 * 64KB segment is the driver (most of the time).
 * Snarf KISA5 to map to the buffer. Everything is single threded,
 * so that should be safe.
 *
 * segflag, defines the 64KB segment where I/O buffer located.
 * addr - address within the 64KB segment
 * xor  - data to exclusive or with the data in the buffer
 */

#define	KISA5	((physadr)0172352)

fixecc(addr, xor)
unsigned addr;
unsigned xor;
{
	register int okisa5;
	register int i;
	register char *p;

	okisa5 = KISA5->r[0];
	i = segflag << 10;
	i |= ((addr >> 6) & 01600);
	KISA5->r[0] = i;
	p = (addr & 017777) | 0120000;
	*((int *)p) ^= xor;
	KISA5->r[0] = okisa5;
}

/*
 * This routine performs the same function as fixecc(), but is
 * only used by the HK driver. Instead of fixing the ECC in the user's
 * buffer, it fixes the sector headers read into the user's buffer
 * when revectoring bad blocks.
 */

fixhdr(addr, data)
unsigned addr;
unsigned data;
{
	register int okisa5;
	register int i;
	register char *p;

	okisa5 = KISA5->r[0];
	i = segflag << 10;
	i |= ((addr >> 6) & 01600);
	KISA5->r[0] = i;
	p = (addr & 017777) | 0120000;
	*((int *)p) = data;
	KISA5->r[0] = okisa5;
}
