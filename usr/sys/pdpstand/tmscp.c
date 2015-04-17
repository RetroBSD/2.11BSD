/*	@(#)tmscp.c	7.1.5 (2.11BSD GTE) 1998/1/30 */

/****************************************************************
 *        Licensed from Digital Equipment Corporation           *
 *                       Copyright (c)                          *
 *               Digital Equipment Corporation                  *
 *                   Maynard, Massachusetts                     *
 *                         1985, 1986                           *
 *                    All rights reserved.                      *
 *                                                              *
 *        The Information in this software is subject to change *
 *   without notice and should not be construed as a commitment *
 *   by  Digital  Equipment  Corporation.   Digital   makes  no *
 *   representations about the suitability of this software for *
 *   any purpose.  It is supplied "As Is" without expressed  or *
 *   implied  warranty.                                         *
 *                                                              *
 *        If the Regents of the University of California or its *
 *   licensees modify the software in a manner creating         *
 *   deriviative copyright rights, appropriate copyright        *
 *   legends may be placed on  the drivative work in addition   *
 *   to that set forth above.                                   *
 ***************************************************************/
/*
 * tmscp.c - TMSCP (TK50/TU81) standalone driver
 */
 
/* static char *sccsid = "@(#)tmscp.c	1.5	(ULTRIX) 4/18/86"; */

/* ------------------------------------------------------------------------
 * Modification History: /sys/pdpstand/tmscp.c
 *
 * 5-30-95 sms - new iob structure.
 * 4-20-91 sms - add multi controller and unit support (sms)
 * 8-20-90 steven m. schultz (sms@wlv.iipo.gtegsc.com)
 *	Port from 4.3BSD to 2.11BSD
 * 3-15-85  afd
 *	Don't ask for an interrupt when commands are issued and
 *	check ownership bit in the response descriptor to detect when a
 *	command is complete.  Necessary due to the TU81's failure to set
 *	the response interrupt field in the communications area.
 *
 * ------------------------------------------------------------------------
 */
 
#include "../h/param.h"
#include "saio.h"

/*
 * Parameters for the communications area
 * (Only 1 cmd & 1 rsp packet)
 */
#define	NRSPL2	0			/* Define these before including */
#define	NCMDL2	0			/*   tmscpreg.h and tmscp.h below */

#include "sys/buf.h"
#include "../pdpuba/tmscpreg.h"
#include "../pdp/tmscp.h"

#define	NTMS	2

	struct	tmscpdevice *TMScsr[NTMS + 1] =
		{
		(struct tmscpdevice *)0174500,
		(struct tmscpdevice *)0,
		(struct tmscpdevice *)-1
		};
 
struct tmscp tmscp[NTMS];
 
u_char tmsoffline[NTMS] = {1, 1};	/* Flag to prevent multiple STCON */
u_char tms_offline[NTMS][4] = {{1,1,1,1},
			       {1,1,1,1}}; /* Flag to prevent multiple ONLIN */
static char opnmsg[] = "tms%d: step %d failed sa=0%o\n";
 
extern int tapemark;		/* flag to indicate tapemark encountered
				   (see sys.c as to how it's used) */
 
/*
 * Open a tmscp device. Initialize the controller and set the unit online.
 */
tmscpopen(io)
	register struct iob *io;
{
	register struct tmscpdevice *tmscpaddr;
	int	ctlr = io->i_ctlr;
	int	unit = io->i_unit, bae, lo16;
	register struct tmscp *tms = &tmscp[ctlr];
 
	if (genopen(NTMS, io) < 0)
		return(-1);
	io->i_flgs |= F_TAPE;
	tmscpaddr = TMScsr[ctlr];

	/*
	 * Have the tmscp controller characteristics already been set up
	 * (STCON)?
	 */
	if (tmsoffline[ctlr])
		{
		/*
		 * Initialize the tmscp device and wait for the 4 steps
		 * to complete.
		 */
		tmscpaddr->tmscpip = 0;
		while ((tmscpaddr->tmscpsa & TMSCP_STEP1) == 0)
			;
		tmscpaddr->tmscpsa =TMSCP_ERR|(NCMDL2<<11)|(NRSPL2<<8);
 
		while ((tmscpaddr->tmscpsa & TMSCP_STEP2) == 0)
			;
#define STEP1MASK 0174377
#define STEP1GOOD (TMSCP_STEP2|TMSCP_IE|(NCMDL2<<3)|NRSPL2)
		iomapadr(&tms->tmscp_ca.ca_ringbase, &bae, &lo16);
		if ((tmscpaddr->tmscpsa&STEP1MASK) != STEP1GOOD)
			printf(opnmsg, ctlr, 1, tmscpaddr->tmscpsa);
		tmscpaddr->tmscpsa = lo16;
 
		while ((tmscpaddr->tmscpsa & TMSCP_STEP3) == 0)
			;
#define STEP2MASK 0174377
#define STEP2GOOD (TMSCP_STEP3)
		if ((tmscpaddr->tmscpsa&STEP2MASK) != STEP2GOOD)
			printf(opnmsg, ctlr, 2, tmscpaddr->tmscpsa);
		tmscpaddr->tmscpsa = bae;
 
		while ((tmscpaddr->tmscpsa & TMSCP_STEP4) == 0)
			;
#define STEP3MASK 0174000
#define STEP3GOOD TMSCP_STEP4
		if ((tmscpaddr->tmscpsa&STEP3MASK) != STEP3GOOD)
			printf(opnmsg, ctlr, 3, tmscpaddr->tmscpsa);
		tmscpaddr->tmscpsa = TMSCP_GO;
		if (tmscpcmd(io, M_OP_STCON, 0) == 0)
			{
			printf("%s STCON", devname(io));
			return(-1);
			}
		tmsoffline[ctlr] = 0;
		}
	tms->tmscp_cmd[0].mscp_unit = unit;
	/* 
	 * Has this unit been issued an ONLIN?
	 */
	if (tms_offline[ctlr][unit])
		{
		if (tmscpcmd(io, M_OP_ONLIN, 0) == 0)
			{
			printf("%s ONLIN", devname(io));
			return(-1);
			}
		tms_offline[ctlr][unit] = 0;
		}
	tmscpclose(io);		/* close just does a rewind */
	if	(io->i_part > 0)
		/*
		 * Skip forward the appropriate number of files on the tape.
		 */
		{
		tms->tmscp_cmd[0].mscp_tmkcnt = io->i_part;
		tms->tmscp_cmd[0].mscp_buffer_h = 0;
		tms->tmscp_cmd[0].mscp_bytecnt = 0;
		tmscpcmd(io, M_OP_REPOS, 0);
		tms->tmscp_cmd[0].mscp_tmkcnt = 0;
		}
	return(0);
}
 
/*
 * Close the device (rewind it to BOT)
 */
tmscpclose(io)
	register struct iob *io;
{
	register struct tmscp *tms = &tmscp[io->i_ctlr];

	tms->tmscp_cmd[0].mscp_buffer_l = 0;	/* tmkcnt */
	tms->tmscp_cmd[0].mscp_buffer_h = 0;
	tms->tmscp_cmd[0].mscp_bytecnt = 0;
	tms->tmscp_cmd[0].mscp_unit = io->i_unit;
	tmscpcmd(io, M_OP_REPOS, M_MD_REWND | M_MD_CLSEX);
}
 
/*
 * Set up tmscp command packet.  Cause the controller to poll to pick up
 * the command.
 */
tmscpcmd(io, op,mod)
	struct	iob *io;
	int op, mod;		/* opcode and modifier (usu 0) */
{
	int ctlr = io->i_ctlr;
	register struct tmscp *tms = &tmscp[ctlr];
	register struct mscp *mp;	/* ptr to cmd packet */
	int i;				/* read into to init polling */
	int	bae, lo16;
 
	/*
	 * Init cmd & rsp area
	 */
	iomapadr(&tms->tmscp_cmd[0].mscp_cmdref, &bae, &lo16);
	tms->tmscp_ca.ca_cmddsc[0].lsh = lo16;
	tms->tmscp_ca.ca_cmddsc[0].hsh = bae;
	tms->tmscp_cmd[0].mscp_dscptr = (long *)tms->tmscp_ca.ca_cmddsc;
	tms->tmscp_cmd[0].mscp_header.mscp_vcid = 1;	/* for tape */

	iomapadr(&tms->tmscp_rsp[0].mscp_cmdref, &bae, &lo16);
	tms->tmscp_ca.ca_rspdsc[0].lsh = lo16;
	tms->tmscp_ca.ca_rspdsc[0].hsh = bae;
	tms->tmscp_rsp[0].mscp_dscptr = (long *)tms->tmscp_ca.ca_rspdsc;
	tms->tmscp_cmd[0].mscp_cntflgs = 0;

	tms->tmscp_cmd[0].mscp_opcode = op;
	tms->tmscp_cmd[0].mscp_modifier = mod;
	tms->tmscp_cmd[0].mscp_header.mscp_msglen = sizeof (struct tmscp);
	tms->tmscp_ca.ca_cmddsc[0].hsh |= TMSCP_OWN;	/* | TMSCP_INT */
	tms->tmscp_rsp[0].mscp_header.mscp_msglen = sizeof (struct tmscp);
	tms->tmscp_ca.ca_rspdsc[0].hsh |= TMSCP_OWN;	/* | TMSCP_INT */
	tms->tmscp_cmd[0].mscp_zzz2 = 0;
 
	i = TMScsr[ctlr]->tmscpip;
	for (;;)
		{
		if (TMScsr[ctlr]->tmscpsa & TMSCP_ERR) {
			printf("%s Fatal err sa=%o\n",
				devname(io), TMScsr[ctlr]->tmscpsa);
			return(0);
		}
 
		if (tms->tmscp_ca.ca_cmdint)
			tms->tmscp_ca.ca_cmdint = 0;
		/*
		 * This is to handle the case of devices not setting the
		 * interrupt field in the communications area. Some
		 * devices (early TU81's) only clear the ownership field
		 * in the Response Descriptor.
		 */
/*
		if (tms->tmscp_ca.ca_rspint)
			break;
*/
		if (!(tms->tmscp_ca.ca_rspdsc[0].hsh & TMSCP_OWN))
			break;
		}
	tms->tmscp_ca.ca_rspint = 0;
	mp = &tms->tmscp_rsp[0];
	if (mp->mscp_opcode != (op|M_OP_END) ||
	   (mp->mscp_status&M_ST_MASK) != M_ST_SUCC) {
		/* Detect hitting tape mark.  This signifies the end of the
		 * tape mini-root file.  We don't want to return an error
		 * condition to the strategy routine.  Set tapemark flag
		 * for sys.c
		 */
		if ((mp->mscp_status & M_ST_MASK) == M_ST_TAPEM) {
			tapemark = 1;
			return(1);
		}
		printf("%s I/O err 0%o op=0%o mod=0%o\n", devname(io),
			mp->mscp_status, op, mod);
		return(0);
	}
	return(1);
}
 
/*
 * Set up to do reads and writes; call tmscpcmd to issue the cmd.
 */
tmscpstrategy(io, func)
	register struct iob *io;
	int func;
{
	int	ctlr = io->i_ctlr, unit = io->i_unit;
	int	bae, lo16;
	register struct tmscp *tms = &tmscp[ctlr];
	register struct mscp *mp;
 
	mp = &tms->tmscp_cmd[0];
	mp->mscp_lbn_l = loint(io->i_bn);
	mp->mscp_lbn_h = hiint(io->i_bn);
	mp->mscp_unit = unit;
	mp->mscp_bytecnt = io->i_cc;
	iomapadr(io->i_ma, &bae, &lo16);
	mp->mscp_buffer_l = lo16;
	mp->mscp_buffer_h = bae;
	if	(tmscpcmd(io, func == READ ? M_OP_READ : M_OP_WRITE, 0) ==0)
		return(-1);
	/*
	 * Detect hitting tape mark so we do it gracefully and return a
	 * character count of 0 to signify end of copy.
	 */
	if (tapemark)
		return(0);
	return(io->i_cc);
}

tmscpseek(io, space)
	register struct iob *io;
	int	space;
	{
	register struct tmscp *tms = &tmscp[io->i_ctlr];
	int	mod;

	if	(space == 0)
		return(0);
	if	(space < 0)
		{
		mod = M_MD_REVRS;
		space = -space;
		}
	else
		mod = 0;
	tms->tmscp_cmd[0].mscp_buffer_l = 0;
	tms->tmscp_cmd[0].mscp_buffer_h = 0;
	tms->tmscp_cmd[0].mscp_unit = io->i_unit;
	tms->tmscp_cmd[0].mscp_reccnt = space;
	tmscpcmd(io, M_OP_REPOS, mod | M_MD_OBJCT);
	return(0);
	}
