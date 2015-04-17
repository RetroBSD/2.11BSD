/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_sri.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "sri.h"
#if	NSRI > 0

/*
 * SRI dr11c ARPAnet IMP interface driver.
 */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "domain.h"
#include "protosw.h"
#include "socket.h"
#include "pdpuba/ubavar.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "net/if.h"
#include "pdpif/if_sri.h"
#include "netimp/if_imp.h"
#include "pdpif/if_uba.h"

int     sriprobe(), sriattach(), srirint(), srixint();
struct  uba_device *sriinfo[NSRI];
u_short sristd[] = { 0 };
struct  uba_driver sridriver =
	{ sriprobe, 0, sriattach, 0, sristd, "sri", sriinfo };
#define	SRIUNIT(x)	minor(x)

#define IFRADDR sc->sri_ifuba.ifu_r.ifrw_info
#define IFWADDR sc->sri_ifuba.ifu_w.ifrw_info

int	sriinit(), sristart(), srireset();

/*
 * "Lower half" of IMP interface driver.
 *
 * Each IMP interface is handled by a common module which handles
 * the IMP-host protocol and a hardware driver which manages the
 * hardware specific details of talking with the IMP.
 *
 * The hardware portion of the IMP driver handles DMA and related
 * management of UNIBUS resources.  The IMP protocol module interprets
 * contents of these messages and "controls" the actions of the
 * hardware module during IMP resets, but not, for instance, during
 * UNIBUS resets.
 *
 * The two modules are coupled at "attach time", and ever after,
 * through the imp interface structure.  Higher level protocols,
 * e.g. IP, interact with the IMP driver, rather than the SRI.
 */
struct	sri_softc {
	/* pdma registers, shared with assembly helper */
	struct sridevice *sri_addr;     /* hardware address */
	char    *sri_iba;               /* in byte addr */
	short   sri_ibc;                /* in byte count */
	short   sri_iclick;             /* in click addr */
	short   sri_ibf;                /* in buf (last byte, plus flags) */
	short   sri_ibusy;              /* in dma busy flag */
	char    *sri_oba;               /* out byte addr */
	short   sri_obc;                /* out byte count */
	short   sri_oclick;             /* out click addr */
	short   sri_oend;               /* out end flags */
	/* end pdma */
	struct	ifnet *sri_if;		/* pointer to IMP's ifnet struct */
	struct	impcb *sri_ic;		/* data structure shared with IMP */
	struct	ifuba sri_ifuba;	/* UNIBUS resources */
	struct	mbuf *sri_iq;		/* input reassembly queue */
	short	sri_olen;		/* size of last message sent */
	char	sri_flush;		/* flush remainder of message */
} sri_softc[NSRI];

/*
 * Reset the IMP and cause a transmitter interrupt.
 */
sriprobe(reg)
	caddr_t reg;
{
#if !pdp11
	register int br, cvec;		/* r11, r10 value-result */
	register struct sridevice *addr = (struct sridevice *)reg;
	int i;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	srirint(0); srixint(0);
#endif
	addr->csr = 0;
	addr->obf = OUT_HRDY;
	DELAY(100000L);
	addr->csr = SRI_OINT|SRI_OENB;
	addr->obf = OUT_LAST;
	DELAY(100000L);
	addr->csr = 0;
	addr->obf = OUT_HNRDY;
	if(cvec && cvec != 0x200)
		return(1);
	addr->csr = SRI_IINT|SRI_IENB;
	i = addr->ibf;
	DELAY(100000L);
	addr->csr = 0;
	if(cvec && cvec != 0x200)
		cvec -= 4;              /* report as xmit intr */
	return(1);
#endif
}

/*
 * Call the IMP module to allow it to set up its internal
 * state, then tie the two modules together by setting up
 * the back pointers to common data structures.
 */
sriattach(ui)
	struct uba_device *ui;
{
	register struct sri_softc *sc = &sri_softc[ui->ui_unit];
	register struct impcb *ip;
	struct ifimpcb {
		struct	ifnet ifimp_if;
		struct	impcb ifimp_impcb;
	} *ifimp;

	if ((ifimp = (struct ifimpcb *)impattach(ui, srireset)) == 0)
		panic("sriattach");
	sc->sri_if = &ifimp->ifimp_if;
	ip = &ifimp->ifimp_impcb;
	sc->sri_ic = ip;
	ip->ic_init = sriinit;
	ip->ic_start = sristart;
	sc->sri_if->if_reset = srireset;
	sc->sri_addr = (struct sridevice *) ui->ui_addr;
}

/*
 * Reset interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
srireset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;
	struct sri_softc *sc;

	if (unit >= NSRI || (ui = sriinfo[unit]) == 0 || ui->ui_alive == 0 ||
	    ui->ui_ubanum != uban)
		return;
	printf(" sri%d", unit);
	sc = &sri_softc[unit];
	/* must go through IMP to allow it to set state */
	(*sc->sri_if->if_init)(unit);
}

/*
 * Initialize interface: clear recorded pending operations,
 * and retrieve, and initialize UNIBUS resources.  Note
 * return value is used by IMP init routine to mark IMP
 * unavailable for outgoing traffic.
 */
sriinit(unit)
	int unit;
{	
	register struct sri_softc *sc;
	register struct uba_device *ui;
	register struct sridevice *addr;
	int x, info;

	if (unit >= NSRI || (ui = sriinfo[unit]) == 0 || ui->ui_alive == 0) {
		printf("sri%d: not alive\n", unit);
		return (0);
	}
	sc = &sri_softc[unit];
	/*
	 * Header length is 0 since we have to passs
	 * the IMP leader up to the protocol interpretation
	 * routines.  If we had the header length as
	 * sizeof(struct imp_leader), then the if_ routines
	 * would asssume we handle it on input and output.
	 */

	if (if_ubainit(&sc->sri_ifuba, ui->ui_ubanum, 0,
	     (int)btoc(IMPMTU)) == 0) {
		printf("sri%d: can't initialize\n", unit);
		goto down;
	}
	addr = (struct sridevice *)ui->ui_addr;
#if pdp11
	sc->sri_iclick = (sc->sri_ifuba.ifu_r.ifrw_click);
	sc->sri_oclick = (sc->sri_ifuba.ifu_w.ifrw_click);
#endif

	/*
	 * Reset the imp interface;
	 * the delays are pure guesswork.
	 */
	addr->csr = 0; DELAY(5000L);
	addr->obf = OUT_HRDY;           /* close the relay */
	DELAY(10000L);
	addr->obf = 0;
	/* YECH!!! */
	x = 5;				/* was 500 before rdy line code!!! */
	while (x-- > 0) {
		if ((addr->ibf & IN_INRDY) == 0 )
			break;
		DELAY(10000L);
	}
	if (x <= 0) {
/*		printf("sri%d: imp doesn't respond, ibf=%b\n", unit,
			addr->ibf, SRI_INBITS);
*/
		return(0);	/* goto down;*/
	}

	/*
	 * Put up a read.  We can't restart any outstanding writes
	 * until we're back in synch with the IMP (i.e. we've flushed
	 * the NOOPs it throws at us).
	 * Note: IMPMTU includes the leader.
	 */
	x = splimp();
	sc->sri_iba = (char *)IFRADDR;
	sc->sri_ibc = IMPMTU;
	sc->sri_ibusy = -1;     /* skip leading zeros */
	addr->csr |= (SRI_IINT|SRI_IENB);
	splx(x);
	return (1);
down:
	ui->ui_alive = 0;
	return (0);
}

/*
 * Start output on an interface.
 */
sristart(dev)
	dev_t dev;
{
	int unit = SRIUNIT(dev), info;
	register struct sri_softc *sc = &sri_softc[unit];
	register struct sridevice *addr;
	struct mbuf *m;
	u_short cmd;

	if (sc->sri_ic->ic_oactive)
		goto restart;
	
	/*
	 * Not already active, deqeue a request and
	 * map it onto the UNIBUS.  If no more
	 * requeusts, just return.
	 */
	IF_DEQUEUE(&sc->sri_if->if_snd, m);
	if (m == 0) {
		sc->sri_ic->ic_oactive = 0;
		return;
	}
	sc->sri_olen = ((if_wubaput(&sc->sri_ifuba, m) + 1 ) & ~1);

restart:
	addr = (struct sridevice *)sriinfo[unit]->ui_addr;
	sc->sri_oba = (char *)IFWADDR;
	sc->sri_obc = sc->sri_olen;
	sc->sri_oend = OUT_LAST;
	sc->sri_obc--;
	addr->csr |= (SRI_OENB|SRI_OINT);
	addr->obf = (*sc->sri_oba++ & 0377);
	sc->sri_ic->ic_oactive = 1;
}

/*
 * Output interrupt handler.
 */
srixint(unit)
{
	register struct sri_softc *sc = &sri_softc[unit];
	register struct sridevice *addr = sc->sri_addr;
	int burst,delay;
	register int x;

	burst = 0;
	while(sc->sri_obc > 0) {
		x = (*sc->sri_oba++ & 0377);
		if(--sc->sri_obc <= 0) x |= sc->sri_oend;
		addr->obf = x;
		if(++burst > 16) goto out;
		for(delay=0 ;; delay++) {
			if(delay > 12) goto out;
			if(addr->csr&SRI_OREQ) break;
		}
	}

	addr->csr &= ~SRI_OINT;
	if (sc->sri_ic->ic_oactive == 0) {
		printf("sri%d: stray xmit interrupt\n", unit);
		goto out;
	}
	sridump("out",IFWADDR,sc->sri_olen);
	sc->sri_if->if_opackets++;
	sc->sri_ic->ic_oactive = 0;
	if (sc->sri_obc != 0) { /* only happens if IMP ready drop */
		printf("sri%d: output error, csr=%b\n", unit,
			addr->csr, SRI_BITS);
		sc->sri_if->if_oerrors++;
	}
	if (sc->sri_ifuba.ifu_xtofree) {
		m_freem(sc->sri_ifuba.ifu_xtofree);
		sc->sri_ifuba.ifu_xtofree = 0;
	}
	if (sc->sri_if->if_snd.ifq_head)
		sristart(unit);
out:
	return;
}

/*
 * Input interrupt handler
 */
srirint(unit)
{
	register struct sri_softc *sc = &sri_softc[unit];
    	struct mbuf *m;
	int len, info;
	register struct sridevice *addr = sc->sri_addr;
	int burst,delay;
	register int x;

	burst = 0;
	for(;;) {
		addr->csr &= ~SRI_IENB; /* prevents next read from starting */
		sc->sri_ibf = x = addr->ibf;
		if(x & IN_CHECK) break; /* LAST or error */
		if(sc->sri_ibc <= 0) break;     /* spurrious int */
		x &= 0377;
		if(sc->sri_ibusy < 0) { /* flushing leading zeros */
			if(x == 0) goto next;
			sc->sri_ibusy = 1;
		}
		*sc->sri_iba++ = x;
		if(--sc->sri_ibc <= 0) break;   /* count exhausted */
	next:
		addr->csr |= SRI_IENB;  /* start next read */
		if(++burst > 16) goto out;
		for(delay=0 ;; delay++) {
			if(delay > 12) goto out;
			if(addr->csr&SRI_IREQ) break;
		}
	}

	x = sc->sri_ibf;
	/* grab the last byte if EOM */
	if((x & IN_LAST) && !(x & IN_INRDY) && sc->sri_ibc > 0) {
		*sc->sri_iba++ = (x & 0377);
		sc->sri_ibc--;
	}
	addr->csr &= ~SRI_IINT;
	sc->sri_if->if_ipackets++;
	if ((x & IN_INRDY)) {
		printf("sri%d: input error, ibf=%b\n", unit,
		    x, SRI_INBITS);
		sc->sri_if->if_ierrors++;
		sc->sri_flush = 1;
		if(sc->sri_obc > 0) {   /* if output active */
			sc->sri_obc = -1;       /* flush it */
			srixint(unit);
		}
 		/* tell "other half" of module that ready line just dropped */
		/* Kludge, I know, but the alternative is to create an mbuf */
		impinput(unit, (struct 	mbuf *) 0); /* ready line dropped */
		goto out; /* leave interrupts un-enabled */ /* FIX THIS!!!! */
	}

	if (sc->sri_flush) {
		if (x & IN_LAST)
			sc->sri_flush = 0;
		goto setup;
	}
	len = IMPMTU - sc->sri_ibc;
	if (len < 10 || len > IMPMTU) {
		printf("sri%d: bad length=%d\n", len);
		sc->sri_if->if_ierrors++;
		goto setup;
	}
	sridump("in ",IFRADDR,len);

	/*
	 * The next to last parameter is always 0 since using
	 * trailers on the ARPAnet is insane.
	 */
	m = if_rubaget(&sc->sri_ifuba, len, 0, &sc->sri_if);
	if (m == 0)
		goto setup;
	if ((x & IN_LAST) == 0) {
		if (sc->sri_iq)
			m_cat(sc->sri_iq, m);
		else
			sc->sri_iq = m;
		goto setup;
	}
	if (sc->sri_iq) {
		m_cat(sc->sri_iq, m);
		m = sc->sri_iq;
		sc->sri_iq = 0;
	}
	impinput(unit, m);

setup:
	/*
	 * Setup for next message.
	 */
	sc->sri_iba = (char *)IFRADDR;
	sc->sri_ibc = IMPMTU;
	sc->sri_ibusy = -1;     /* skip leading zeros */
	addr->csr |= (SRI_IINT|SRI_IENB);
out:
	return;
}

sridump(str,aba,abc)
char *str,*aba;
{
	int col,i;

	if(str[0] != 07)
		if(!sridebug()) return;
	printf("%s  ",str);
	col = 0;
	for(; abc ; abc--) {
		i = *aba++ & 0377;
		printf("%o ",i);
		if(++col > 31) {
			col = 0;
			printf("\n   ");
		}
	}
	printf("\n");
}

#if vax

sridebug()
{
		return( (mfpr(RXCS) & RXCS_DONE) == 0 &&
			(mfpr(RXDB) & 0177) == 07);
}

#endif
#if pdp11

sridebug()
{
		return( (*(char *)0177560 & 0200) == 0 &&
			(*(char *)0177562 & 0177) == 07);
}

#endif
#endif NSRI
