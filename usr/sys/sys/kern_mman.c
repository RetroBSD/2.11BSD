/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "proc.h"
#include "vm.h"
#include "text.h"
#include "systm.h"

sbrk()
{
	struct a {
		char	*nsiz;
	};
	register int n, d;

	/* set n to new data size */
	n = btoc((int)((struct a *)u.u_ap)->nsiz);
	if (!u.u_sep)
		if (u.u_ovdata.uo_ovbase)
			n -= u.u_ovdata.uo_dbase;
		else
			n -= ctos(u.u_tsize) * stoc(1);
	if (n < 0)
		n = 0;
	if (estabur(u.u_tsize, n, u.u_ssize, u.u_sep, RO))
		return;
	expand(n, S_DATA);
	/* set d to (new - old) */
	d = n - u.u_dsize;
	if (d > 0)
		clear(u.u_procp->p_daddr + u.u_dsize, d);
	u.u_dsize = n;
}

/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
	register unsigned sp;
{
	register int si;

	if (sp >= -ctob(u.u_ssize))
		return (0);
	si = (-sp) / ctob(1) - u.u_ssize + SINCR;
	/*
	 * Round the increment back to a segment boundary if necessary.
	 */
	if (ctos(si + u.u_ssize) > ctos(((-sp) + ctob(1) - 1) / ctob(1)))
		si = stoc(ctos(((-sp) + ctob(1) - 1) / ctob(1))) - u.u_ssize;
	if (si <= 0)
		return (0);
	if (estabur(u.u_tsize, u.u_dsize, u.u_ssize + si, u.u_sep, RO))
		return (0);
	/*
	 *  expand will put the stack in the right place;
	 *  no copy required here.
	 */
	expand(u.u_ssize + si, S_STACK);
	u.u_ssize += si;
	clear(u.u_procp->p_saddr, si);
	return (1);
}

/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.  The
 * argument sep specifies if the text and data+stack segments are to be
 * separated.  The last argument determines whether the text segment is
 * read-write or read-only.
 */
estabur(nt, nd, ns, sep, xrw)
	u_int nt, nd, ns;
	int sep, xrw;
{
	register int a;
	register short *ap, *dp;
	u_int ts;

	if (u.u_ovdata.uo_ovbase && nt)
		ts = u.u_ovdata.uo_dbase;
	else
		ts = nt;
	if (sep) {
#ifndef NONSEPARATE
		if (!sep_id)
			goto nomem;
		if (ctos(ts) > 8 || ctos(nd)+ctos(ns) > 8)
#endif !NONSEPARATE
			goto nomem;
	} else
		if (ctos(ts) + ctos(nd) + ctos(ns) > 8)
			goto nomem;
	if (u.u_ovdata.uo_ovbase && nt)
		ts = u.u_ovdata.uo_ov_offst[NOVL];
	if (ts + nd + ns + USIZE > maxmem) {
nomem:		u.u_error = ENOMEM;
		return (-1);
	}
	a = 0;
	ap = &u.u_uisa[0];
	dp = &u.u_uisd[0];
	while (nt >= 128) {
		*dp++ = (127 << 8) | xrw | TX;
		*ap++ = a;
		a += 128;
		nt -= 128;
	}
	if (nt) {
		*dp++ = ((nt - 1) << 8) | xrw | TX;
		*ap++ = a;
	}
#ifdef NONSEPARATE
	if (u.u_ovdata.uo_ovbase && ts)
#else !NONSEPARATE
	if ((u.u_ovdata.uo_ovbase && ts) && !sep)
#endif NONSEPARATE
		/*
		 * overlay process, adjust accordingly.
		 * The overlay segment's registers will be set by
 		 * choverlay() from sureg().
		 */ 
		for (a = 0; a < u.u_ovdata.uo_nseg; a++) {
			*ap++ = 0;
			*dp++ = 0;
		}
#ifndef NONSEPARATE
	if (sep)
		while(ap < &u.u_uisa[8]) {
			*ap++ = 0;
			*dp++ = 0;
		}
#endif !NONSEPARATE

	a = 0;
	while (nd >= 128) {
		*dp++ = (127 << 8) | RW;
		*ap++ = a;
		a += 128;
		nd -= 128;
	}
	if (nd) {
		*dp++ = ((nd - 1) << 8) | RW;
		*ap++ = a;
		a += nd;
	}
	while (ap < &u.u_uisa[8]) {
		if (*dp & ABS) {
			dp++;
			ap++;
			continue;
		}
		*dp++ = 0;
		*ap++ = 0;
	}
#ifndef NONSEPARATE
	if (sep)
		while (ap < &u.u_uisa[16]) {
			if(*dp & ABS) {
				dp++;
				ap++;
				continue;
			}
			*dp++ = 0;
			*ap++ = 0;
		}
#endif !NONSEPARATE
	a = ns;
	while (ns >= 128) {
		a -= 128;
		ns -= 128;
		*--dp = (0 << 8) | RW | ED;
		*--ap = a;
	}
	if (ns) {
		*--dp = ((128 - ns) << 8) | RW | ED;
		*--ap = a - 128;
	}
#ifndef NONSEPARATE
	if (!sep) {
		ap = &u.u_uisa[0];
		dp = &u.u_uisa[8];
		while(ap < &u.u_uisa[8])
			*dp++ = *ap++;
		ap = &u.u_uisd[0];
		dp = &u.u_uisd[8];
		while(ap < &u.u_uisd[8])
			*dp++ = *ap++;
	}
#endif !NONSEPARATE
	sureg();
	return(0);
}

/*
 * Load the user hardware segmentation registers from the software
 * prototype.  The software registers must have been setup prior by
 * estabur.
 */
sureg()
{
	register int	*rdp;
	register short	*uap,
			*udp;
	short	*limudp;
	int *rap;
	int taddr, daddr, saddr;
	struct text *tp;

	taddr = daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
	if ((tp = u.u_procp->p_textp) != NULL)
		taddr = tp->x_caddr;
#ifndef NONSEPARATE
	limudp = &u.u_uisd[16];
	if (!sep_id)
#endif !NONSEPARATE
		limudp = &u.u_uisd[8];
	rap = (int *) UISA;
	rdp = (int *) UISD;
	uap = &u.u_uisa[0];
	for (udp = &u.u_uisd[0]; udp < limudp;) {
		*rap++ = *uap++ + (*udp & TX? taddr:
			(*udp & ED ? saddr: (*udp & ABS ? 0 : daddr)));
		*rdp++ = *udp++;
	}
	/*
	 *  Since software prototypes are not maintained for overlay
	 *  segments, force overlay change.  The test for TX is because
	 *  there is no text if called from core().
	 */
	if (u.u_ovdata.uo_ovbase && (u.u_uisd[0] & TX))
		choverlay(u.u_uisd[0] & ACCESS);
}

/*
 * Routine to change overlays.  Only hardware registers are changed; must be
 * called from sureg since the software prototypes will be out of date.
 */
choverlay(xrw)
	int xrw;
{
	register u_short *rap, *rdp;
	register int nt;
	int addr;
	u_short *limrdp;

	rap = &(UISA[u.u_ovdata.uo_ovbase]);
	rdp = &(UISD[u.u_ovdata.uo_ovbase]);
	limrdp = &(UISD[u.u_ovdata.uo_ovbase + u.u_ovdata.uo_nseg]);
	if (u.u_ovdata.uo_curov) {
		addr = u.u_ovdata.uo_ov_offst[u.u_ovdata.uo_curov - 1];
		nt = u.u_ovdata.uo_ov_offst[u.u_ovdata.uo_curov] - addr;
		addr += u.u_procp->p_textp->x_caddr;
		while (nt >= 128) {
			*rap++ = addr;
			*rdp++ = (127 << 8) | xrw;
			addr += 128;
			nt -= 128;
		}
		if (nt) {
			*rap++ = addr;
			*rdp++ = ((nt-1) << 8) | xrw;
		}
	}
	while (rdp < limrdp) {
		*rap++ = 0;
		*rdp++ = 0;
	}
#ifndef NONSEPARATE
	/*
	 * This section copies the UISA/UISD registers to the
	 * UDSA/UDSD registers.  It is only needed for data fetches
	 * on the overlaid segment, which normally don't happen.
	 */
	if (!u.u_sep && sep_id) {
		rdp = &(UISD[u.u_ovdata.uo_ovbase]);
		rap = rdp + 8;
		/* limrdp is still correct */
		while (rdp < limrdp)
			*rap++ = *rdp++;
		rdp = &(UISA[u.u_ovdata.uo_ovbase]);
		rap = rdp + 8;
		limrdp = &(UISA[u.u_ovdata.uo_ovbase + u.u_ovdata.uo_nseg]);
		while (rdp < limrdp)
			*rap++ = *rdp++;
	}
#endif !NONSEPARATE
}
