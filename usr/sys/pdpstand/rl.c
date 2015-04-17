/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rl.c	2.6 (2.11BSD) 1997/11/7
 */

/*
 *  RL disk driver
 *
 *	Modified to handle disklabels - 1995/06/15
 *	RL driver modified for the standalone shell.
 *	Armando P. Stettner  Digital Equipment Corp.  July, 1980
 */

#include "../h/param.h"
#include "../pdpuba/rlreg.h"
#include "saio.h"

#define	NRL	2

	struct	rldevice *RLcsr[NRL + 1] =
		{
		(struct rldevice *)0174400,
		(struct rldevice *)0,
		(struct rldevice *)-1
		};

#define	BLKRL1	10240		/* Number of UNIX blocks for an RL01 drive */
#define BLKRL2	20480		/* Number of UNIX blocks for an RL02 drive */
#define RLCYLSZ 10240		/* bytes per cylinder */
#define RLSECSZ 256		/* bytes per sector */

struct	Rldrives
{
	int	cn[4];		/* location of heads for each drive */
	int	type[4];	/* # blocks on drive (RL01/02) */
	int	com;		/* read or write command word */
	int	chn;		/* cylinder and head number */
	unsigned int bleft;	/* bytes left to be transferred */
	unsigned int bpart;	/* number of bytes transferred */
	int	sn;		/* sector number */
	union {
		int	w[2];
		long	l;
	} addr;			/* address of memory for transfer */

} rl[NRL] = {{-1,-1,-1,-1,-1,-1,-1,-1},
	     {-1,-1,-1,-1,-1,-1,-1,-1}};  /* initialize cn[] and type[] */

rlstrategy(io, func)
	register struct iob *io;
	int func;
{
	int drive = io->i_unit;
	int dif;
	int head;
	int bae, lo16;
	int ctlr = io->i_ctlr;
	register struct rldevice *rladdr;
	register struct Rldrives *rlp;

	rladdr = RLcsr[ctlr];
	rlp = &rl[ctlr];

	dif = deveovchk(io);
	if	(dif <= 0)
		return(dif);

	iomapadr(io->i_ma, &bae, &lo16);
	rlp->chn = io->i_bn/20;
	rlp->sn = (io->i_bn%20) << 1;
	rlp->bleft = io->i_cc;
	rlp->addr.w[0] = bae;
	rlp->addr.w[1] = lo16;
	rlp->com = (drive << 8);
	if (func == READ)
		rlp->com |= RL_RCOM;
	else
		rlp->com |= RL_WCOM;
reading:
	/*
	 * One has to seek an RL head, relativily.
	 */
	dif =(rlp->cn[drive] >> 1) - (rlp->chn >>1);
	head = (rlp->chn & 1) << 4;
	if (dif < 0)
		rladdr->rlda = (-dif <<7) | RLDA_SEEKHI | head;
	else
		rladdr->rlda = (dif << 7) | RLDA_SEEKLO | head;
	rladdr->rlcs = (drive << 8) | RL_SEEK;
	rlp->cn[drive] = rlp->chn; 	/* keep current, our notion of where the heads are */
	if (rlp->bleft < (rlp->bpart = RLCYLSZ - (rl->sn * RLSECSZ)))
		rlp->bpart = rlp->bleft;
	while ((rladdr->rlcs&RL_CRDY) == 0)
		continue;
	rladdr->rlda = (rlp->chn << 6) | rlp->sn;
	rladdr->rlba = (caddr_t) rlp->addr.w[1];
	rladdr->rlmp = -(rlp->bpart >> 1);
	rladdr->rlcs = rlp->com | rlp->addr.w[0] << 4;
	while ((rladdr->rlcs & RL_CRDY) == 0)	/* wait for completion */
		continue;
	if (rladdr->rlcs < 0) {
		/* check error bit */
		if (rladdr->rlcs & 040000) {
			/* Drive error */
			/*
			 * get status from drive
			 */
			rladdr->rlda = RLDA_GS;
			rladdr->rlcs = (drive << 8) | RL_GETSTATUS;
			while ((rladdr->rlcs & RL_CRDY) == 0)	/* wait for controller */
				continue;
		}
		printf("%s err cy=%d, hd=%d, sc=%d, rlcs=%o, rlmp=%o\n",
			devname(io), rlp->chn>>01, rlp->chn&01, rlp->sn, 
			rladdr->rlcs, rladdr->rlmp);
		return(-1);
	}
	/*
	 * Determine if there is more to read to satisfy this request.
	 * This is to compensate for the lack of spiraling reads.
	 */
	if ((rlp->bleft -= rlp->bpart) > 0) {
		rlp->addr.l += rlp->bpart;
		rlp->sn = 0;
		rlp->chn++;
		goto reading;	/* read some more */
	}
	return(io->i_cc);
}

rlopen(io)
	register struct iob *io;
	{

	if	(io->i_unit > 3)
		return(-1);
	if	(genopen(NRL, io) < 0)
		return(-1);
	rlgsts(io);		/* get status and head position */
	if	(devlabel(io, READLABEL) < 0)
		return(-1);
	io->i_boff = io->i_label.d_partitions[io->i_part].p_offset;
	return(0);
	}

/*
 * We must determine what type of drive we are talking to in order 
 * to determine how many blocks are on the device.  The rl.type[]
 * array has been initialized with -1's so that we may test first
 * contact with a particular drive and do this determination only once.
 *
 * RL02 GET STATUS BAND-AID - Fred Canter 10/14/80
 *
 * For some unknown reason the RL02 (seems to be
 * only drive 1) does not return a valid drive status
 * the first time that a GET STATUS request is issued
 * for the drive, in fact it can take up to three or more
 * GET STATUS requests to obtain the correct status.
 * In order to overcome this "HACK" the driver has been
 * modified to issue a GET STATUS request, validate the
 * drive status returned, and then use it to determine the
 * drive type. If a valid status is not returned after eight
 * attempts, then an error message is printed.
 */

rlgsts(io)
	struct	iob	*io;
	{
	int	ctr;
	int	drive = io->i_unit;
	int	ctlr = io->i_ctlr;
	register struct	Rldrives *rlp = &rl[ctlr];
	register struct rldevice *rladdr = RLcsr[ctlr];


	if (rlp->type[drive] < 0) {
		ctr = 0;
		do {
			/* load this register; what a dumb controller */
			rladdr->rlda = RLDA_RESET|RLDA_GS;
			/* set up csr */
			rladdr->rlcs = (drive << 8) | RL_GETSTATUS;
			while ((rladdr->rlcs & RL_CRDY) == 0)	/* wait for it */		
				continue;
		} while (((rladdr->rlmp & 0177477) != 035) && (++ctr < 8));
		if (ctr >= 8)
			printf("\nCan't get %s sts\n", devname(io));
		if (rladdr->rlmp & RLMP_DTYP) 
			rlp->type[drive] = BLKRL2;	/* drive is RL02 */
		else
			rlp->type[drive] = BLKRL1;	/* drive RL01 */
		/*
		 * When device is first touched, find out where the heads are.
		 */
		rladdr->rlcs = (drive << 8) | RL_RHDR;
		while ((rladdr->rlcs&RL_CRDY) == 0)
			continue;
		rlp->cn[drive] = ((rladdr->rlmp) >> 6) & 01777;
	}
	return;
}

/*
 * This generates a default label.  'rlopen' has already been called so
 * we can use the 'types' field as the number of sectors on the device.
*/

rllabel(io)
	register struct iob *io;
	{
	register struct disklabel *lp = &io->i_label;
	daddr_t	nblks = rl[io->i_ctlr].type[io->i_unit];

	lp->d_type = DTYPE_DEC;
	lp->d_partitions[0].p_size = nblks;
	lp->d_nsectors = 20;		/* sectors per track */
	lp->d_ntracks = 2;		/* tracks per cylinder */
	lp->d_secpercyl = 40;		/* sectors per cylinder */
	lp->d_ncylinders = nblks / (20 * 2);
	lp->d_secperunit = nblks;
	return(0);
	}
