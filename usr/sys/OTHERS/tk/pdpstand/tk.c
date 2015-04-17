/**********************************************************************
 *   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.    *
 *   All Rights Reserved. 					      *
 *   Reference "/usr/src/COPYRIGHT" for applicable restrictions       *
 **********************************************************************/

/*
 * ULTRIX-11 Stand-alone driver for TMSCP magtape
 *
 * SCCSID: @(#)tk.c	3.0	4/21/86
 *
 * Chung-Wu Lee, Jan-25-85
 *
 *	start supporting TU81.
 *
 * Chung-Wu Lee, Feb-05-85
 *
 *	Supports only one TMSCP controller.
 *
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/tmscp.h>
#include "saio.h"
#include "tk_saio.h"

/*
 * UQPORT registers and structures
 */

struct device {
	int	tkaip;		/* initialization and polling */
	int	tkasa;		/* status and address */
};

struct	device	*tk_csr;

#define	TK_ERR		0100000	/* error bit */
#define	TK_STEP4	0040000	/* step 4 has started */
#define	TK_STEP3	0020000	/* step 3 has started */
#define	TK_STEP2	0010000	/* step 2 has started */
#define	TK_STEP1	0004000	/* step 1 has started */
#define	TK_SMASK	0074000	/* mask for checking step bit */
#define	TK_NV		0002000	/* no host settable interrupt vector */
#define	TK_QB		0001000	/* controller supports Q22 bus */
#define	TK_DI		0000400	/* controller implements diagnostics */
#define	TK_IE		0000200	/* interrupt enable */
#define	TK_PI		0000001	/* host requests adapter purge interrupts */
#define	TK_GO		0000001	/* start operation, after init */
#define	TK_DELI		200	/* SA reg checking loop count (see tkbits) */

/*
 * Parameters for the communications area
 */

#define	NRSPL2	0		/* log2 number of response packets */
#define	NCMDL2	0		/* log2 number of command packets */
#define	NRSP	(1<<NRSPL2)
#define	NCMD	(1<<NCMDL2)

/*
 * UQPORT Communications Area
 */

struct tkca {
	short	ca_xxx1;	/* unused */
	char	ca_xxx2;	/* unused */
	char	ca_bdp;		/* BDP to purge */
	short	ca_cmdint;	/* command queue transition interrupt flag */
	short	ca_rspint;	/* response queue transition interrupt flag */
	struct {
		unsigned int rl;
		unsigned int rh;
	} ca_rspdsc[NRSP];	/* response descriptors */
	struct {
		unsigned int cl;
		unsigned int ch;
	} ca_cmddsc[NCMD];	/* command descriptors */
};

#define	ca_ringbase	ca_rspdsc[0].rl

#define	TK_OWN	0100000	/* TK owns this descriptor */
#define	TK_INT	0040000	/* allow interrupt on ring transition */

/*
 * Controller states
 */
#define	S_IDLE	0		/* hasn't been initialized */
#define	S_STEP1	1		/* doing step 1 init */
#define	S_STEP2	2		/* doing step 2 init */
#define	S_STEP3	3		/* doing step 3 init */
#define	S_SCHAR	4		/* doing "set controller characteristics" */
#define	S_RUN	5		/* running */

struct tk {
	struct tkca	tk_ca;		/* communications area */
	struct tmscp	tk_rsp[NRSP];	/* response packets */
	struct tmscp	tk_cmd[NCMD];	/* command packets */
} tk;

struct tmscp *tkcmd();

int	tk_stat;		/* Info saved for error messages */
int	tk_ecode;
int	tk_eflags;
int	tk_ctid;		/* controller type ID + micro-code rev level */
char	*tk_dct;		/* controller type for error messages */

/*
 * Drive info obtained from on-line and get unit status commands
 */
struct	tk_drv	tk_drv;

/*
 * Open a TK.  Initialize the device and
 * set the unit online.
 */

tkopen(io)
register struct iob *io;
{
	register struct device *tkaddr;
	int unit, i;
	char *p;

	unit = io->i_unit&7;
	if(unit >= NTK) {
		printf("\nNo such device");
		return(-1);
	}
	tkaddr = (struct device *)devsw[io->i_ino.i_dev].dv_csr;
	tk_csr = tkaddr;	/* save CSR for tkcmd() */
	if(tk_drv.tk_openf) {
		printf("\n%s ONLINE already", tk_dct);
		return(-1);
	}
	p = (caddr_t)&tk;
	for(i=0; i<sizeof(struct tk); i++)
		*p++ = 0;
	if (tkqinit(tkaddr) != 0) {
		tkerror("INIT FAILED", tkaddr);
		return(-1);
	}
/*
 * Init the drives.
 */
	if(tkinit(unit) == 0) {
		printf("\n%s OFFLINE: status=%o", tk_dct, tk_stat);
		return(-1);
	}
	tk.tk_cmd[0].m_modifier = M_M_REWND;
	tk.tk_cmd[0].m_tmcnt = 0;
	tk.tk_cmd[0].m_buf_h = 0;
	tk.tk_cmd[0].m_bytecnt = 0;
	tk.tk_cmd[0].m_zzz2 = 0;
	tkcmd(M_O_REPOS);		/* rewind to the BOT */
	if(io->i_boff > 0) {		/* skip if offset is not zero */
		tk.tk_cmd[0].m_modifier = 0;
		tk.tk_cmd[0].m_tmcnt = io->i_boff;
		tk.tk_cmd[0].m_bytecnt = 0;
		tk.tk_cmd[0].m_zzz2 = 0;
		if(tkcmd(M_O_REPOS) == 0) {
			printf("\n%s REPOS FAILED: no such address", tk_dct);
			return(-1);
		}
	}
	tk_drv.tk_openf++;
	return(0);
}

tkclose(io)
register struct iob *io;
{
	tk.tk_cmd[0].m_modifier = M_M_REWND;
	tk.tk_cmd[0].m_tmcnt = 0;
	tk.tk_cmd[0].m_buf_h = 0;
	tk.tk_cmd[0].m_bytecnt = 0;
	tk.tk_cmd[0].m_zzz2 = 0;
	tkcmd(M_O_REPOS);		/* rewind to the BOT */
	tk_drv.tk_openf = 0;
}

struct tmscp *
tkcmd(op)
int op;
{
	struct tmscp *mp;
	int i;
	register struct device *tkaddr;

	tkaddr = tk_csr;
	tk.tk_cmd[0].m_opcode = op;
	if (op == M_O_ONLIN)
		tk.tk_cmd[0].m_modifier |= M_M_CLSEX;
	tk.tk_ca.ca_rspdsc[0].rh |= TK_OWN;
	tk.tk_ca.ca_cmddsc[0].ch |= TK_OWN;
	i = tkaddr->tkaip;
	while(tk.tk_ca.ca_cmddsc[0].ch & TK_OWN) ;
	while(tk.tk_ca.ca_rspdsc[0].rh & TK_OWN) ;
	tk.tk_ca.ca_rspint = 0;
	mp = &tk.tk_rsp[0];
	tk_stat = mp->m_status;
	tk_ecode = mp->m_opcode&0377;
	tk_eflags = mp->m_flags&0377;
	if ((mp->m_opcode & 0377) != (op|M_O_END) ||
	    mp->m_status != M_S_SUCC)
		return(0);
	return(mp);
}

tkstrategy(io, func)
	register struct iob *io;
{
	register struct tmscp *mp;
	int i,unit, op;
	char *p;

	unit = io->i_unit&7;
	if(unit >= NTK) {
		printf("\nNo such device");
		return(-1);
	}
	p = 0;
	if(devsw[io->i_ino.i_dev].dv_flags == DV_TAPE)
	{
	   i = (tk_ctid>>4) & 07;
	   if (i != TK50 && i != TU81)
		p = "TK50/TU81 ";
	}
	else
		p = "";
	if(p) {
		printf("\n%s: unit %d not %smagtape!\n",
			tk_dct, unit, p);
		tk_drv.tk_openf = 0;
		return(-1);
	}
	mp = &tk.tk_cmd[0];
	mp->m_unit = unit;
	mp->m_bytecnt = io->i_cc;
	mp->m_zzz2 = 0;
	mp->m_buf_l = io->i_ma;
	mp->m_buf_h = segflag;
	if(func == READ)
		op = M_O_READ;
	else
		op = M_O_WRITE;
	if((mp = tkcmd(op)) == 0) {
		printf("\n%s magtape error: ", tk_dct);
		printf("endcode=%o flags=%o status=%o\n",
			tk_ecode, tk_eflags, tk_stat);
		printf("(FATAL ERROR)\n");
		return(-1);
	}
	return(io->i_cc);
}

/*
 * Initialize a drive,
 * do GET UNIT STATUS and ONLINE commands
 * and save the results.
 */
tkinit(unit)
register int unit;
{
	register struct tmscp *mp;

	tk.tk_cmd[0].m_unit = unit;
	mp = &tk.tk_rsp[0];
	tk_drv.tk_online = 0;	/* mark unit off-line */
	tk_drv.tk_dt = 0;		/* mark unit non-existent */
	tk.tk_cmd[0].m_modifier = 0;
	if(tkcmd(M_O_ONLIN) != 0) {		/* ON-LINE command */
		tk_drv.tk_online = 1;	/* unit is on-line */
		tk_drv.tk_dt = *((int *)&mp->m_mediaid) & 0177;
	}
	return(tk_drv.tk_online);
}
tkqinit(addr)
register struct device *addr;
{
	register i, j;

	for (j=0; j<3; j++) {
		addr->tkaip = 0;	/* start initialization */
		i = 0;
		while(addr->tkasa != 0) {
			if (++i > 1000)
				break;
		}
		if(tkbits(addr, TK_DELI, TK_STEP1))
			continue;
		addr->tkasa = TK_ERR;
		if(tkbits(addr, TK_DELI, TK_STEP2))
			continue;
		addr->tkasa = (short)&tk.tk_ca.ca_ringbase;
		if(tkbits(addr, TK_DELI, TK_STEP3))
			continue;
		addr->tkasa = segflag;
		if(tkbits(addr, TK_DELI, TK_STEP4))
			continue;
		tk_ctid = addr->tkasa & 0377;	/* save controller ID */
		switch((tk_ctid>>4) & 017) {
		case TK50:
			tk_dct = "TK50";
			break;
		case TU81:
			tk_dct = "TU81";
			break;
		default:
			tk_dct = "TMSCP";
			break;
		}
		addr->tkasa = TK_GO;
		tk.tk_ca.ca_rspdsc[0].rl = &tk.tk_rsp[0].m_cmdref;
		tk.tk_ca.ca_rspdsc[0].rh = segflag;
		tk.tk_rsp[0].m_header.tk_msglen =
			sizeof(struct tmscp) - sizeof(struct tmscp_header);
		tk.tk_ca.ca_cmddsc[0].cl = &tk.tk_cmd[0].m_cmdref;
		tk.tk_ca.ca_cmddsc[0].ch = segflag;
		tk.tk_cmd[0].m_header.tk_msglen =
		sizeof(struct tmscp) - sizeof(struct tmscp_header);
		tk.tk_cmd[0].m_header.tk_vcid = 1;
		tk.tk_cmd[0].m_cntflgs = 0;
		/* need to set the density if TU81 */
		if (tkcmd(M_O_STCON) == 0) {
			printf("\n%s STCON FAILED: can't init controller", tk_dct);
			return(TK_ERR);
		}
		return(0);
	}
	return(TK_ERR);
}
tkbits(addr, delay, step)
register struct device *addr;
int	delay;
int	step;
{
	register int i;

	if(step == TK_STEP1) {
	    for(i=0; i<32767; i++) ;
	    if((addr->tkasa&TK_SMASK) != TK_STEP1)
		return(TK_ERR);
	    else
		return(0);
	} else {
	    while(1) {
		for(i=0; i<delay; i++) ;
		if((addr->tkasa & step) == 0)
		    continue;
		if((addr->tkasa & TK_SMASK) != step)
		    return(TK_ERR);
		else
		    return(0);
	    }
	}
}
tkerror(str, addr)
char *str;
struct device *addr;
{
	printf("\nTMSCP cntrl at %o: %s (SA=%o)\n", addr, str, addr->tkasa);
}
