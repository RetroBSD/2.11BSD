/*
 * 	1.2	(2.11BSD)	1998/1/28
 *
 * This routine was moved from the main TMSCP driver due to size problems.
 * The driver could become over 8kb in size and would not fit within an
 * overlay!
 *
 * Thought was given to simply deleting this routine altogether - who 
 * does crash dumps to tape (and the tmscp.c driver is the only one with
 * a crash dump routine) but in the end the decision was made to retain
 * the capability but move it to a separate file.
*/

#include "tms.h"
#if	NTMSCP > 0

#include "param.h"
#include "buf.h"
#include "machine/seg.h"
#include "tmscpreg.h"
#include "pdp/tmscp.h"
#include "errno.h"
#include "map.h"
#include "uba.h"
#include "pdp/seg.h"

#ifdef	TMSCP_DUMP
#define DBSIZE 16

extern	memaddr	tmscp[];
extern	char	*tmscpfatalerr;
extern	struct	tmscp_softc	tmscp_softc[];

tmsdump(dev)
	dev_t dev;
{
	register struct tmscpdevice *tmscpaddr;
	register struct	tmscp_softc *sc;
	register struct	mscp	*mp;
	daddr_t bn, dumpsize;
	long paddr, maddr;
	int unit = TMSUNIT(dev), count, ctlr = TMSCTLR(dev);
	struct	ubmap *ubp;
	segm seg5;

	if (ctlr >= NTMSCP)
		return (ENXIO);
	sc = &tmscp_softc[ctlr];
	tmscpaddr = sc->sc_addr;
	if (tmscpaddr == NULL)
		return(ENXIO);

	paddr = _iomap(tmscp[sc->sc_unit]);
	if (ubmap) {
		ubp = UBMAP;
		ubp->ub_lo = loint(paddr);
		ubp->ub_hi = hiint(paddr);
	}
	paddr += RINGBASE;
	saveseg5(seg5);
	mapseg5(tmscp[sc->sc_unit], MAPBUFDESC);
	mp = sc->sc_com->tmscp_rsp;
	sc->sc_com->tmscp_ca.ca_cmdint = sc->sc_com->tmscp_ca.ca_rspint = 0;
	bzero(mp, 2 * sizeof (*mp));

	tmscpaddr->tmscpip = 0;
	while ((tmscpaddr->tmscpsa & TMSCP_STEP1) == 0)
		if(tmscpaddr->tmscpsa & TMSCP_ERR) return(EFAULT);
	tmscpaddr->tmscpsa = TMSCP_ERR;
	while ((tmscpaddr->tmscpsa & TMSCP_STEP2) == 0)
		if(tmscpaddr->tmscpsa & TMSCP_ERR) return(EFAULT);
	tmscpaddr->tmscpsa = loint(paddr);
	while ((tmscpaddr->tmscpsa & TMSCP_STEP3) == 0)
		if(tmscpaddr->tmscpsa & TMSCP_ERR) return(EFAULT);
	tmscpaddr->tmscpsa = hiint(paddr);
	while ((tmscpaddr->tmscpsa & TMSCP_STEP4) == 0)
		if(tmscpaddr->tmscpsa & TMSCP_ERR) return(EFAULT);
	tmscpaddr->tmscpsa = TMSCP_GO;
	
	tmsginit(sc, sc->sc_com->tmscp_ca.ca_rspdsc, mp, 0, 2, 0);

	if (tmscpcmd(M_OP_STCON, unit, sc) == 0) {
		return(EFAULT);
	}
	sc->sc_com->tmscp_cmd[0].mscp_unit = unit;
	if (tmscpcmd(M_OP_ONLIN, unit, sc) == 0) {
		return(EFAULT);
	}

	dumpsize = 8 * 1024L;	 /* XXX */
	ubp = &UBMAP[1];
	for (paddr = 0; dumpsize > 0; dumpsize -= count) {
		count = MIN(dumpsize, DBSIZE);
		bn = paddr >> PGSHIFT;
		maddr = paddr;
		if (ubmap) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			maddr = (u_int)(1 << 13);
		}
	/* write it to the tape */
	mp = &sc->sc_com->tmscp_rsp[1];
	mp->mscp_lbn_l = loint(bn);
	mp->mscp_lbn_h = hiint(bn);
	mp->mscp_bytecnt = count * NBPG;
	mp->mscp_buffer_l = loint(maddr);
	mp->mscp_buffer_h = hiint(maddr);
	if (tmscpcmd(M_OP_WRITE, unit, sc) == 0)
		return(EIO);
	paddr += (DBSIZE << PGSHIFT);
	}
	restorseg5(seg5);
	return (0);
}

/*
 * Perform a standalone tmscp command.  This routine is only used by tmscpdump.
 */

tmscpcmd(op, unit, sc)
	int op;
	int unit;
	register struct tmscp_softc *sc;
{
	int i;
	register struct mscp *cmp, *rmp;
	Trl *rlp;

	cmp = &sc->sc_com->tmscp_rsp[1];
	rmp = &sc->sc_com->tmscp_rsp[0];
	rlp = &sc->sc_com->tmscp_ca.ca_rspdsc[0];

	cmp->mscp_opcode = op;
	cmp->mscp_unit = unit;
	cmp->mscp_header.mscp_msglen = sizeof (struct mscp);
	rmp->mscp_header.mscp_msglen = sizeof (struct mscp);
	rlp[0].hsh |= TMSCP_OWN|TMSCP_INT;
	rlp[1].hsh |= TMSCP_OWN|TMSCP_INT;
	if (sc->sc_addr->tmscpsa&TMSCP_ERR)
		printf(tmscpfatalerr, sc->sc_unit, unit, sc->sc_addr->tmscpsa);
	i = sc->sc_addr->tmscpip;

	while ((rlp[1].hsh & TMSCP_INT) == 0)
		;
	while ((rlp[0].hsh & TMSCP_INT) == 0)
		;

	sc->sc_com->tmscp_ca.ca_rspint = 0;
	sc->sc_com->tmscp_ca.ca_cmdint = 0;
	if (rmp->mscp_opcode != (op|M_OP_END) ||
	    (rmp->mscp_status&M_ST_MASK) != M_ST_SUCC)
		{
		printf("err: com %d opc 0x%x stat 0x%x\ndump ", op,
			rmp->mscp_opcode, rmp->mscp_status);
		return(0);
		}
	return(1);
}
#endif /* TMSCP_DUMP */
#endif /* NTMSCP */
