/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_uba.c	1.2 (2.11BSD GTE) 4/3/93
 *
 *	2.11BSD - uballoc and ubmalloc calling conventions changed.
 *		  ubmalloc now only performs address computation, the
 *		  necessary UMRs are allocated at network startup.
 *		  sms@wlv.imsd.contel.com - 9/8/90
 */

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "domain.h"
#include "protosw.h"
#include "mbuf.h"
#include "buf.h"
#include "pdpuba/ubavar.h"
#include "map.h"
#include "uba.h"
#include "socket.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "net/if.h"
#include "pdpif/if_uba.h"

/*
 * Routines supporting UNIBUS network interfaces.
 */

if_ubainit(ifu, uban, hlen, nmr)
	register struct ifuba *ifu;
	int uban, hlen, nmr;		/* nmr in 64 byte clicks */
{
	if (ifu->ifu_r.ifrw_click)
		return(1);
	nmr = ctob(nmr);		/* convert clicks back to bytes */
	ifu->ifu_r.ifrw_click = m_ioget(nmr+hlen);
	ifu->ifu_w.ifrw_click = m_ioget(nmr+hlen);
	if (ifu->ifu_r.ifrw_click == 0 || ifu->ifu_w.ifrw_click == 0) {
		ifu->ifu_r.ifrw_click = ifu->ifu_w.ifrw_click = 0;
		return(0);
	}
	ifu->ifu_r.ifrw_info = ubmalloc(ifu->ifu_r.ifrw_click);
	ifu->ifu_w.ifrw_info = ubmalloc(ifu->ifu_w.ifrw_click);
	ifu->ifu_hlen = hlen;
	return(1);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.
 */
struct mbuf *
if_rubaget(ifu, totlen, off0, ifp)
	register struct ifuba *ifu;
	int totlen, off0;
	struct ifnet *ifp;
{
	register caddr_t cp = (caddr_t)ifu->ifu_hlen;
	register struct mbuf *m;
	struct mbuf *top, **mp;
	int click = ifu->ifu_r.ifrw_click;
	int off = off0, len;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m_freem(top);
			top = 0;
			goto out;
		}
		if (off) {
			len = totlen - off;
			cp = (caddr_t) (ifu->ifu_hlen + off);
		} else
			len = totlen;
		if (len >= NBPG) {
			if (ifp)
				goto nopage;
			MCLGET(m);
			if (m->m_len != CLBYTES)
				goto nopage;
			m->m_len = MIN(len, CLBYTES);
			goto copy;
		}
nopage:
		m->m_off = MMINOFF;
		if (ifp) {
			/*
			 *	Leave room for ifp.
			 */
			m->m_len = MIN(MLEN - sizeof(ifp), len);
			m->m_off += sizeof(ifp);
		} else
			m->m_len = MIN(MLEN, len);
copy:
		mbcopyin(click, cp, mtod(m, char *), (u_int)m->m_len);
		cp += m->m_len;
nocopy:
		*mp = m;
		mp = &m->m_next;
		if (off) {
			/* sort of an ALGOL-W style for statement... */
			off += m->m_len;
			if (off == totlen) {
				cp = (caddr_t) ifu->ifu_hlen;
				off = 0;
				totlen = off0;
			}
		} else
			totlen -= m->m_len;
		if (ifp) {
			/*
			 *	Prepend interface pointer to first mbuf.
			 */
			m->m_len += sizeof(ifp);
			m->m_off -= sizeof(ifp);
			*(mtod(m, struct ifnet **)) = ifp;
			ifp = NULL;
		}
	}
out:
	return(top);
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header.
 */
if_wubaput(ifu, m)
	struct ifuba *ifu;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register u_short off = 0;
	u_short click = ifu->ifu_w.ifrw_click;

	while (m) {
		mbcopyout(mtod(m, char *), click, off, (u_int)m->m_len);
		off += m->m_len;
		MFREE(m, mp);
		m = mp;
	}
	return(off);
}

#define	KDSA	((u_short *)0172260)	/* supervisor - was 172360.KERNEL */

/*
 *	Map UNIBUS virtual memory over some address in supervisor data
 *	space.  We're similar to the "mapalloc" routine used for
 *	raw I/O, but for different objects.  The kernel's 'ubmap' is
 *	tested since the network's "fake" 'ubmap' has gone away (this
 *	routine was the only one to use it).
 */
ubadr_t
uballoc(addr, size)
	caddr_t addr;
	u_int size;
{
	register int nregs;
	register struct ubmap *ubp;
	ubadr_t paddr, vaddr;
	u_int click, first;
	int page, offset;

	page = (((int)addr >> 13) & 07);
	offset = ((int)addr & 017777);
	click = KDSA[page];
	paddr = (ubadr_t)click << 6;
	paddr += offset;
	if (!mfkd(&ubmap))
		return(paddr);
	nregs = (int)btoub(size);
	first = MALLOC(ub_map, nregs);
#ifdef	DIAGNOSTIC
/*
 * Should never happen since this is only called by initialization routines
 * in the network drivers.
*/
	if	(!first)
		panic("uballoc");
#endif
	ubp = &UBMAP[first];
	vaddr = (ubadr_t)first << 13;
	while (nregs--) {
		ubp->ub_lo = loint(paddr);
		ubp->ub_hi = hiint(paddr);
		ubp++;
		paddr += (ubadr_t) UBPAGE;
	}
	return(vaddr);
}

/*
 *	Computes a physical address within the mapped I/O area managed by
 *	m_ioget.  For a UNIBUS machine, the m_ioget arena is
 *	already mapped by UMRs and mioumr and miostart have the base
 *	virtual and click addresses of the mapped arena.  For Q22 machines
 *	mioumr and miostart are 0, turning the calculation into a ctob
 * 	of the input click address.
 */
ubadr_t
ubmalloc(addr)
	memaddr addr;		/* pdp11 "clicks" */
{
	extern ubadr_t mioumr;
	extern memaddr miostart;

	return(((ubadr_t)(addr - miostart) << 6) + mioumr);
}
