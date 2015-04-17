/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ra.c	2.8 (2.11BSD GTE) 1998/1/30
 */

/*
 * MSCP disk device driver (rx23, rx33, rx50, rd??, ra??, rz??)
 */
#include "../h/param.h"
#include "../machine/mscp.h"
#include "../pdpuba/rareg.h"
#include "saio.h"

#define	NRA	2
#define	RA_SMASK	(RA_STEP4|RA_STEP3|RA_STEP2|RA_STEP1)

	struct	radevice *RAcsr[NRA + 1] =
		{
		(struct radevice *)0172150,
		(struct	radevice *)0,
		(struct radevice *)-1
		};

/*
 * RA Communications Area
*/
struct  rdca {
	short	ca_xxx1;	/* unused */
	char	ca_xxx2;	/* unused */
	char	ca_bdp;		/* BDP to purge */
	short	ca_cmdint;	/* command queue transition interrupt flag */
	short	ca_rspint;	/* response queue transition interrupt flag */
	short	ca_rspl;	/* response descriptors */
	short	ca_rsph;
	short	ca_cmdl;	/* command descriptors */
	short	ca_cmdh;
};

#define	ca_ringbase	ca_rspl

#define	RA_OWN	0x8000	/* RQ owns this descriptor */
#define	RA_INT	0x4000	/* allow interrupt on ring transition */

static	struct	ra {
	struct	rdca	ra_ca;
	struct	mscp	ra_rsp;
	struct	mscp	ra_cmd;
} rd[NRA];

static	u_char 	rainit[NRA];
static	int	mx();

/*
 * This contains the volume size in sectors of units which have been
 * brought online.  This value is used at default label generation time
 * along with the results of a 'get unit status' command to compute the
 * "geometry" of the drive.
*/
static	long	raonline[NRA][8];

raopen(io)
	register struct iob *io;
{
	register struct radevice *raaddr;
	register struct ra *racom;
	struct 	disklabel *lp = &io->i_label;
	int i, ctlr, unit, bae, lo16;

	ctlr = io->i_ctlr;
	unit = io->i_unit;
	if	(genopen(NRA, io) < 0)
		return(-1);
	raaddr = RAcsr[ctlr];
	racom = &rd[ctlr];

	if (rainit[ctlr] == 0) {
again:		raaddr->raip = 0;
		if 	(ra_step(raaddr, RA_STEP1, 1))
			goto again;
		raaddr->rasa = RA_ERR | (0154/4);
		if	(ra_step(raaddr, RA_STEP2, 2))
			goto again;
		iomapadr(&racom->ra_ca.ca_ringbase, &bae, &lo16);
		raaddr->rasa = lo16;
		if	(ra_step(raaddr, RA_STEP3, 3))
			goto again;
		raaddr->rasa = bae;
		if	(ra_step(raaddr, RA_STEP4, 4))
			goto again;
		raaddr->rasa = RA_GO;
		if (racmd(M_OP_STCON, io) < 0) {
			printf("%s STCON err\n", devname(io));
			return(-1);
		}
		rainit[ctlr] = 1;
	}

	if (raonline[ctlr][unit] == 0)
		if (ramount(io) == -1)
			return(-1);
	if	(devlabel(io, READLABEL) == -1)
		return(-1);
	io->i_boff = lp->d_partitions[io->i_part].p_offset;
	return(0);
}

raclose(io)
	register struct iob *io;
{
	raonline[io->i_ctlr][io->i_unit] = 0;
	return(0);
}

ramount(io)
	register struct iob *io;
{
	register int ctlr = io->i_ctlr;
	register int unit = io->i_unit;

	if (racmd(M_OP_ONLIN, io) < 0) {
		printf("%s !online\n", devname(io));
		return(-1);
	}
	raonline[ctlr][unit] = rd[ctlr].ra_rsp.m_uslow +  
		((long)(rd[ctlr].ra_rsp.m_ushigh) << 16);
	return(0);
}

racmd(op, io)
	int op;
	struct iob *io;
{
	register struct mscp *mp;
	int ctlr = io->i_ctlr;
	int unit = io->i_unit;
	register struct ra *racom = &rd[ctlr];
	struct	radevice *csr = RAcsr[ctlr];
	int i, bae, lo16;

	racom->ra_cmd.m_opcode = op;
	racom->ra_cmd.m_unit = unit;
	racom->ra_cmd.m_cntflgs = 0;
	racom->ra_rsp.m_header.mscp_msglen = sizeof(struct mscp);
	racom->ra_cmd.m_header.mscp_msglen = sizeof(struct mscp);

	iomapadr(&racom->ra_rsp.m_cmdref, &bae, &lo16);
	racom->ra_ca.ca_rspl = lo16;
	racom->ra_ca.ca_rsph = RA_OWN | bae;

	iomapadr(&racom->ra_cmd.m_cmdref, &bae, &lo16);
	racom->ra_ca.ca_cmdl = lo16;
	racom->ra_ca.ca_cmdh = RA_OWN | bae;

	i = csr->raip;

	mp = &racom->ra_rsp;
	while (1) {
		while	(racom->ra_ca.ca_cmdh & RA_OWN) {
			delay(200);		/* SA access delay */
			if	(csr->rasa & (RA_ERR|RA_SMASK))
				goto fail;
		}
		while	(racom->ra_ca.ca_rsph & RA_OWN) {
			delay(200);		/* SA access delay */
			if	(csr->rasa & (RA_ERR|RA_SMASK))
				goto fail;
		}
		racom->ra_ca.ca_cmdint = 0;
		racom->ra_ca.ca_rspint = 0;
		if (mp->m_opcode == (op | M_OP_END))
			break;
		printf("%s rsp %x op %x ignored\n", devname(io),
			mp->m_header.mscp_credits & 0xf0, mp->m_opcode);
		racom->ra_ca.ca_rsph |= RA_OWN;
	}
	if ((mp->m_status & M_ST_MASK) != M_ST_SUCC) {
		printf("%s err op=%x sts=%x\n", devname(io),
			mp->m_opcode, mp->m_status);
		return(-1);
	}
	return(0);
fail:
	printf("%s rasa=%x\n", devname(io), csr->rasa);
}

rastrategy(io, func)
	register struct iob *io;
	int func;
{
	register struct mscp *mp;
	struct ra *racom;
	int	bae, lo16, i;

	i = deveovchk(io);		/* check for end of volume/partition */
	if	(i <= 0)
		return(i);

	racom = &rd[io->i_ctlr];
	mp = &racom->ra_cmd;
	iomapadr(io->i_ma, &bae, &lo16);
	mp->m_lbn_l = loint(io->i_bn);
	mp->m_lbn_h = hiint(io->i_bn);
	mp->m_bytecnt = io->i_cc;
	mp->m_buf_l = lo16;
	mp->m_buf_h = bae;
	if	(racmd(func == READ ? M_OP_READ : M_OP_WRITE, io) < 0)
		return(-1);
	return(io->i_cc);
}

ra_step(csr, mask, step)
	register struct radevice *csr;
	int mask, step;
	{
	register int	cnt;

	for	(cnt = 0; (csr->rasa & mask) == 0; )
		{
		delay(2000);
		cnt++;
		if	(cnt < 5000)
			continue;
		printf("ra(%o) fail step %d. retrying\n",csr,step);
		return(1);
		}
	return(0);
	}

/*
 * This routine is called by the general 'devlabel' routine out of conf.c
 * and is used by the standalone disklabel program to initialize the
 * default disklabel.  The MSCP driver does not need geometry info but
 * it is almost trivial (because the drive has already been brought online by
 * 'raopen') to fetch the required information with a 'get unit status' 
 * command.
*/

ralabel(io)
	struct	iob	*io;
	{
	register struct disklabel *lp = &io->i_label;
	register char *cp, *dp;
	daddr_t	nblks = raonline[io->i_ctlr][io->i_unit];
	struct	mscp *mp = &rd[io->i_ctlr].ra_rsp;
	int	nameid, numid;

	lp->d_type = DTYPE_MSCP;
	lp->d_partitions[0].p_size = nblks;  /* span the drive with 'a' */
/*	lp->d_secperunit = nblks;	     /* size of entire volume */

	if	(racmd(M_OP_GTUNT, io) != 0)
		{
		printf("%s GTUNT failed\n", devname(io));
		return(-1);
		}
/*
 * Yes it's a lot of code but since the standalone utilities (at least 'restor')
 * are likely going to end up split I/D anyhow why not get the information.
 *
 * sectors/track
 * tracks/group * group/cyl = tracks/cyl
 * sectors/track * tracks/cyl = sectors/cyl
 * sectors / sectors/cyl = cyl
*/
	lp->d_nsectors = mp->m_track;
	lp->d_ntracks = mp->m_group * mp->m_cylinder;
	lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	lp->d_ncylinders = nblks / lp->d_secpercyl;
	nameid = (((loint(mp->m_mediaid) & 0x3f) << 9) |
		  ((hiint(mp->m_mediaid) >> 7) & 0x1ff));
	numid = hiint(mp->m_mediaid) & 0x7f;
/*
 * Next put 'RA81' or 'RD54', etc into the typename field.
*/
	cp = lp->d_typename;
	*cp++ = mx(nameid, 2);
	*cp++ = mx(nameid, 1);
	dp = itoa(numid);
	while	(*cp++ = *dp++)
		;
	*cp = mx(nameid, 0);
	if	(*cp != ' ')
		cp++;
	*cp = '\0';
	return(0);
	}

/*
 * this is a routine rather than a macro to save space - shifting, etc 
 * generates a lot of code.
*/

static
mx(l, i)
	int l, i;
	{
	register int c;

	c = (l >> (5 * i)) & 0x1f;
	if	(c == 0)
		c = ' ' - '@';
	return(c + '@');
	}
