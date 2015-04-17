/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_dmc.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "dmc.h"
#if NDMC > 0

#define printd if(dmcdebug)printf
int dmcdebug = 1;
/*
 * DMC11 device driver, internet version
 *
 * TODO
 *	allow more than one outstanding read or write.
 */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "ioctl.h"
#include "tty.h"
#include "domain.h"
#include "protosw.h"
#include "socket.h"
#include "pdpuba/ubavar.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "net/if.h"
#include "pdpif/if_uba.h"
#include "pdpif/if_dmc.h"
#include "netinet/ip.h"
#include "netinet/in_var.h"
#include "netinet/ip_var.h"
#include "net/route.h"
#include "net/netisr.h"
#include "errno.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	dmcprobe(), dmcattach(), dmcinit(), dmcoutput();
struct	uba_device *dmcinfo[NDMC];
u_short	dmcstd[] = { 0 };
struct	uba_driver dmcdriver =
	{ dmcprobe, 0, dmcattach, 0, dmcstd, "dmc", dmcinfo };

#define	DMC_AF	0xff		/* 8 bits of address type in ui_flags */
#define	DMC_NET	0xffffff00	/* 24 bits of net number in ui_flags */

/*
 * DMC software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * sc_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct dmc_softc {
	struct	ifnet sc_if;		/* network-visible interface */
	struct	ifuba sc_ifuba;		/* UNIBUS resources */
	short	sc_flag;		/* flags */
	short	sc_oactive;		/* output active */
	int	sc_ubinfo;		/* UBA mapping info for base table */
	struct clist sc_que;		/* command queue */
} dmc_softc[NDMC];

/* flags */
#define	DMCRUN		01
#define	DMCBMAPPED	02		/* base table mapped */

struct dmc_base {
	short	d_base[128];		/* DMC base table */
} dmc_base[NDMC];

#define	loword(x)	((short *)&x)[0]
#define	hiword(x)	((short *)&x)[1]

dmcprobe(reg)
	caddr_t reg;
{
	register int br, cvec;
	register struct dmcdevice *addr = (struct dmcdevice *)reg;
	register int i;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	dmcrint(0); dmcxint(0);
#endif
	addr->bsel1 = DMC_MCLR;
	for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
		;
	if ((addr->bsel1 & DMC_RUN) == 0)
		return (0);
	addr->bsel1 &= ~DMC_MCLR;
	addr->bsel0 = DMC_RQI|DMC_IEI;
	DELAY(100000L);
	addr->bsel1 = DMC_MCLR;
	for (i = 100000; i && (addr->bsel1 & DMC_RUN) == 0; i--)
		;
#ifdef ECHACK
	br = 0x16;
#endif
	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
dmcattach(ui)
	register struct uba_device *ui;
{
	register struct dmc_softc *sc = &dmc_softc[ui->ui_unit];
	register struct sockaddr_in *sin;

	sc->sc_if.if_unit = ui->ui_unit;
	sc->sc_if.if_name = "dmc";
	sc->sc_if.if_mtu = DMCMTU;
	sc->sc_if.if_init = dmcinit;
	sc->sc_if.if_output = dmcoutput;
	sc->sc_if.if_reset = 0;
	sc->sc_if.if_flags = IFF_POINTOPOINT;
	/* DON'T KNOW IF THIS WILL WORK WITH A BDP AT HIGH SPEEDS */
	sc->sc_ifuba.ifu_flags = UBA_NEEDBDP | UBA_CANTWAIT;
	if_attach(&sc->sc_if);
}

/*
 * Initialization of interface; reinitialize UNIBUS usage.
 */
dmcinit(unit)
	int unit;
{
	register struct dmc_softc *sc = &dmc_softc[unit];
	register struct uba_device *ui = dmcinfo[unit];
	register struct dmcdevice *addr;
	int base;

	printd("dmcinit\n");
	if ((sc->sc_flag&DMCBMAPPED) == 0) {
		sc->sc_ubinfo = uballoc((caddr_t)&dmc_base[unit], 
					sizeof (struct dmc_base));
		sc->sc_flag |= DMCBMAPPED;
	}
	if (if_ubainit(&sc->sc_ifuba, ui->ui_ubanum, 0,
	    (int)btoc(DMCMTU)) == 0) {
		printf("dmc%d: can't initialize\n", unit);
		sc->sc_if.if_flags &= ~IFF_UP;
		return;
	}
	addr = (struct dmcdevice *)ui->ui_addr;
	addr->bsel2 |= DMC_IEO;
	base = sc->sc_ubinfo & 0x3ffff;
	printd("  base 0x%x\n", base);
	dmcload(sc, DMC_BASEI, base, (base>>2)&DMC_XMEM);
	dmcload(sc, DMC_CNTLI, 0, 0);
	base = sc->sc_ifuba.ifu_r.ifrw_info & 0x3ffff;
	dmcload(sc, DMC_READ, base, ((base>>2)&DMC_XMEM)|DMCMTU);
	printd("  first read queued, addr 0x%x\n", base);
	sc->sc_if.if_flags |= IFF_UP;
}

/*
 * Start output on interface.  Get another datagram
 * to send from the interface queue and map it to
 * the interface before starting output.
 */
dmcstart(dev)
	dev_t dev;
{
	int unit = minor(dev);
	struct uba_device *ui = dmcinfo[unit];
	register struct dmc_softc *sc = &dmc_softc[unit];
	int addr, len;
	struct mbuf *m;

	printd("dmcstart\n");
	/*
	 * Dequeue a request and map it to the UNIBUS.
	 * If no more requests, just return.
	 */
	IF_DEQUEUE(&sc->sc_if.if_snd, m);
	if (m == 0)
		return;
	len = if_wubaput(&sc->sc_ifuba, m);

	addr = sc->sc_ifuba.ifu_w.ifrw_info & 0x3ffff;
	printd("  len %d, addr 0x%x, ", len, addr);
	printd("mr 0x%lx\n", sc->sc_ifuba.ifu_w.ifrw_info);
	dmcload(sc, DMC_WRITE, addr, (len&DMC_CCOUNT)|((addr>>2)&DMC_XMEM));
	sc->sc_oactive = 1;
}

/*
 * Utility routine to load the DMC device registers.
 */
dmcload(sc, type, w0, w1)
	register struct dmc_softc *sc;
	int type, w0, w1;
{
	register struct dmcdevice *addr;
	register int unit, sps, n;

	printd("dmcload: 0x%x 0x%x 0x%x\n", type, w0, w1);
	unit = sc - dmc_softc;
	addr = (struct dmcdevice *)dmcinfo[unit]->ui_addr;
	sps = splimp();
	if ((n = sc->sc_que.c_cc) == 0)
		addr->bsel0 = type | DMC_RQI;
	else
		(void) putc(type | DMC_RQI, &sc->sc_que);
	(void) putw(w0, &sc->sc_que);
	(void) putw(w1, &sc->sc_que);
	if (n == 0)
		dmcrint(unit);
	splx(sps);
}

/*
 * DMC interface receiver interrupt.
 * Ready to accept another command,
 * pull one off the command queue.
 */
dmcrint(unit)
	int unit;
{
	register struct dmc_softc *sc;
	register struct dmcdevice *addr;
	register int n;

	addr = (struct dmcdevice *)dmcinfo[unit]->ui_addr;
	sc = &dmc_softc[unit];
	while (addr->bsel0&DMC_RDYI) {
		addr->sel4 = getw(&sc->sc_que);
		addr->sel6 = getw(&sc->sc_que);
		addr->bsel0 &= ~(DMC_IEI|DMC_RQI);
		while (addr->bsel0&DMC_RDYI)
			;
		if (sc->sc_que.c_cc == 0)
			return;
		addr->bsel0 = getc(&sc->sc_que);
		n = RDYSCAN;
		while (n-- && (addr->bsel0&DMC_RDYI) == 0)
			;
	}
	if (sc->sc_que.c_cc)
		addr->bsel0 |= DMC_IEI;
}

/*
 * DMC interface transmitter interrupt.
 * A transfer has completed, check for errors.
 * If it was a read, notify appropriate protocol.
 * If it was a write, pull the next one off the queue.
 */
dmcxint(unit)
	int unit;
{
	register struct dmc_softc *sc;
	struct uba_device *ui = dmcinfo[unit];
	struct dmcdevice *addr;
	struct mbuf *m;
	register struct ifqueue *inq;
	int arg, cmd, len;

	addr = (struct dmcdevice *)ui->ui_addr;
	arg = addr->sel6;
	cmd = addr->bsel2&7;
	addr->bsel2 &= ~DMC_RDYO;
	sc = &dmc_softc[unit];
	printd("dmcxint\n");
	switch (cmd) {

	case DMC_OUR:
		/*
		 * A read has completed.  Purge input buffered
		 * data path.  Pass packet to type specific
		 * higher-level input routine.
		 */
		sc->sc_if.if_ipackets++;
		len = arg & DMC_CCOUNT;
		printd("  read done, len %d\n", len);
		switch (sc->sc_flag & DMC_AF) {
#ifdef INET
		case AF_INET:
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;
#endif

		default:
			printf("dmc%d: unknown address type %d\n", unit,
			    sc->sc_flag & DMC_AF);
			goto setup;
		}
		m = if_rubaget(&sc->sc_ifuba, len, 0, &sc->sc_if);
		if (m == 0)
			goto setup;
		if (IF_QFULL(inq)) {
			IF_DROP(inq);
			(void) m_freem(m);
		} else
			IF_ENQUEUE(inq, m);

setup:
		arg = sc->sc_ifuba.ifu_r.ifrw_info & 0x3ffff;
		dmcload(sc, DMC_READ, arg, ((arg >> 2) & DMC_XMEM) | DMCMTU);
		return;

	case DMC_OUX:
		/*
		 * A write has completed, start another
		 * transfer if there is more data to send.
		 */
		if (sc->sc_oactive == 0)
			return;		/* SHOULD IT BE A FATAL ERROR? */
		printd("  write done\n");
		sc->sc_if.if_opackets++;
		sc->sc_oactive = 0;
		if (sc->sc_ifuba.ifu_xtofree) {
			(void) m_freem(sc->sc_ifuba.ifu_xtofree);
			sc->sc_ifuba.ifu_xtofree = 0;
		}
		if (sc->sc_if.if_snd.ifq_head == 0)
			return;
		dmcstart(unit);
		return;

	case DMC_CNTLO:
		arg &= DMC_CNTMASK;
		if (arg&DMC_FATAL) {
			addr->bsel1 = DMC_MCLR;
			sc->sc_flag &= ~DMCRUN;
			/*** DO SOMETHING TO RESTART DEVICE ***/
			printf("DMC FATAL ERROR 0%o\n", arg);
		} else {
			/* ACCUMULATE STATISTICS */
			printf("DMC SOFT ERROR 0%o\n", arg);
		}
		return;

	default:
		printf("dmc%d: bad control %o\n", unit, cmd);
	}
}

/*
 * DMC output routine.
 * Just send the data, header was supplied by
 * upper level protocol routines.
 */
dmcoutput(ifp, m, dst)
	register struct ifnet *ifp;
	register struct mbuf *m;
	struct sockaddr *dst;
{
	struct uba_device *ui = dmcinfo[ifp->if_unit];
	int s;

	printd("dmcoutput\n");
	if (dst->sa_family != AF_INET) {
		printf("dmc%d: af%d not supported\n", ifp->if_unit, AF_INET);
		m_freem(m);
		return (EAFNOSUPPORT);
	}
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		splx(s);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	if (dmc_softc[ifp->if_unit].sc_oactive == 0)
		dmcstart(ifp->if_unit);
	splx(s);
	return (0);
}
#endif NDMC
