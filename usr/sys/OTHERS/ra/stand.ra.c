/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * RAxx disk device driver
 * RQDX1 (rx50,rd50,rd51,rd52,rd53)
*/
#include <sys/param.h>
#include <sys/inode.h>

#ifndef u_char
#define u_char char
#endif

#include <sys/mscp.h>
#include <sys/rareg.h>

#include "../saio.h"

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

static	struct	radevice *RDADDR = ((struct radevice *)0172150);

static	struct	ra {
	struct	rdca	ra_ca;
	struct	mscp	ra_rsp;
	struct	mscp	ra_cmd;
} rd;

struct 	mscp 	*racmd();
static	int 	rdinit;
static	long	rdonline[8];

static	int	rd_boff[] = { 0, 5868, 0, 8468, -1, -1, -1, -1 };

raopen(io)
	register struct iob 	*io;
{
	register struct mscp 	*mp;
	int 			i;

	if( rdinit == 0 )
	{
		RDADDR->raip = 0;
		while((RDADDR->rasa & RA_STEP1) == 0);
		RDADDR->rasa = RA_ERR;
		while((RDADDR->rasa & RA_STEP2) == 0);
		RDADDR->rasa = (short)&rd.ra_ca.ca_ringbase;
		while((RDADDR->rasa & RA_STEP3) == 0);
		RDADDR->rasa = (short)(segflag & 3);
		while((RDADDR->rasa & RA_STEP4) == 0);
		RDADDR->rasa = RA_GO;
		rd.ra_ca.ca_rspl = (short)&rd.ra_rsp.m_cmdref;
		rd.ra_ca.ca_rsph = (short)(segflag & 3);
		rd.ra_ca.ca_cmdl = (short)&rd.ra_cmd.m_cmdref;
		rd.ra_ca.ca_cmdh = (short)(segflag & 3);
		rd.ra_cmd.m_cntflgs = 0;
		if( racmd(M_O_STCON, io->i_unit) < 0 )
		{
			printf("RD: controller init error, STCON\n");
			return(-1);
		}
		rdinit = 1;
	}

	if( rdonline[io->i_unit & 7] == 0 )
		if( ramount(io) == -1 )
			return(-1);

	if( (io->i_boff < 0) || (io->i_boff > 7) ||
	    (rd_boff[io->i_boff] == -1) )
	{
		printf("RD: bad partition for unit=%d",io->i_unit & 7);
	}
	io->i_boff = rd_boff[io->i_boff];

	return(0);
}

raclose(io)
	register struct iob 	*io;
{
	rdonline[io->i_unit & 7] = 0;
	return(0);
}

ramount(io)
	register struct iob 	*io;
{
	if( racmd(M_O_ONLIN,io->i_unit) < 0 )
	{
		printf("RD: bring online error, unit=%d\n",io->i_unit & 7);
		return(-1);
	}
	rdonline[io->i_unit & 7] = rd.ra_rsp.m_uslow +  
	 	((long)(rd.ra_rsp.m_ushigh) << 16);
	return(0);
}

struct mscp *
racmd(op,unit)
	int op,unit;
{
	register	struct mscp 	*mp;
	register	int 		i;

	rd.ra_cmd.m_opcode = op;
	rd.ra_cmd.m_unit = unit & 7;
	rd.ra_rsp.m_header.ra_msglen = sizeof(struct mscp);
	rd.ra_cmd.m_header.ra_msglen = sizeof(struct mscp);
	rd.ra_ca.ca_rsph = RA_OWN | (segflag & 3);
	rd.ra_ca.ca_cmdh = RA_OWN | (segflag & 3);
	i = RDADDR->raip;
	while ((rd.ra_ca.ca_cmdh & RA_INT) == 0)
		;
	while ((rd.ra_ca.ca_rsph & RA_INT) == 0)
		;
	rd.ra_ca.ca_rspint = 0;
	rd.ra_ca.ca_cmdint = 0;
	mp = &rd.ra_rsp;
	if( ((mp->m_opcode & 0xff) != (op | M_O_END)) ||
	    ((mp->m_status & M_S_MASK) != M_S_SUCC) )
	{
		printf("RD: command error, unit=%d, opcode=%x, status=%x\n",
			unit & 7, mp->m_opcode & 0xff, mp->m_status);
		return(-1);
	}
	return(0);
}

rastrategy(io, func)
	register struct iob *io;
{
	register struct mscp *mp;

	if( io->i_bn >= rdonline[io->i_unit & 7] )
		return(0);

	mp = &rd.ra_cmd;
	mp->m_lbn_l = loint(io->i_bn);
	mp->m_lbn_h = hiint(io->i_bn);
	mp->m_bytecnt = io->i_cc;
	mp->m_buf_l = io->i_ma;
	mp->m_buf_h = segflag & 3;
	if( racmd(func == READ ? M_O_READ : M_O_WRITE,io->i_unit) < 0 )
		return(-1);

	return(io->i_cc);
}


