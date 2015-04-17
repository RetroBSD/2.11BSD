/*	@(#)if_qe.c	1.3 (2.11BSD) 1997/2/16 */
 
/****************************************************************
 *								*
 *        Licensed from Digital Equipment Corporation 		*
 *                       Copyright (c) 				*
 *               Digital Equipment Corporation			*
 *                   Maynard, Massachusetts 			*
 *                         1985, 1986 				*
 *                    All rights reserved. 			*
 *								*
 *        The Information in this software is subject to change *
 *   without notice and should not be construed as a commitment *
 *   by  Digital  Equipment  Corporation.   Digital   makes  no *
 *   representations about the suitability of this software for *
 *   any purpose.  It is supplied "As Is" without expressed  or *
 *   implied  warranty. 					*
 *								*
 *        If the Regents of the University of California or its *
 *   licensees modify the software in a manner creating  	*
 *   derivative copyright rights, appropriate copyright  	*
 *   legends may be placed on the derivative work in addition   *
 *   to that set forth above. 					*
 *								*
 ****************************************************************/
/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 16-Nov-90 -- sms@wlv.imsd.contel.com
 *	Ported from 4.3BSD to 2.11BSD as a replacement for the previous
 *	2.10BSD deqna driver which was flakey.  'trailers' are completely
 *	removed from this version - they are a bad idea and never really
 *	worked.  Advantage is taken of this being a Qbus driver - memory
 *	is allocated from the system and physical addresses computed.
 *
 * 15-Apr-86  -- afd
 *	Rename "unused_multi" to "qunused_multi" for extending Generic
 *	kernel to MicroVAXen.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 * 12 March 86 -- Jeff Chase
 *	Modified to handle the new MCLGET macro
 *	Changed if_qe_data.c to use more receive buffers
 *	Added a flag to poke with adb to log qe_restarts on console
 *
 * 19 Oct 85 -- rjl
 *	Changed the watch dog timer from 30 seconds to 3.  VMS is using
 * 	less than 1 second in their's. Also turned the printf into an
 *	mprintf.
 *
 *  09/16/85 -- Larry Cohen
 * 		Add 43bsd alpha tape changes for subnet routing		
 *
 *  1 Aug 85 -- rjl
 *	Panic on a non-existent memory interrupt and the case where a packet
 *	was chained.  The first should never happen because non-existant 
 *	memory interrupts cause a bus reset. The second should never happen
 *	because we hang 2k input buffers on the device.
 *
 *  1 Aug 85 -- rich
 *      Fixed the broadcast loopback code to handle Clusters without
 *      wedging the system.
 *
 *  27 Feb. 85 -- ejf
 *	Return default hardware address on ioctl request.
 *
 *  12 Feb. 85 -- ejf
 *	Added internal extended loopback capability.
 *
 *  27 Dec. 84 -- rjl
 *	Fixed bug that caused every other transmit descriptor to be used
 *	instead of every descriptor.
 *
 *  21 Dec. 84 -- rjl
 *	Added watchdog timer to mask hardware bug that causes device lockup.
 *
 *  18 Dec. 84 -- rjl
 *	Reworked driver to use q-bus mapping routines.  MicroVAX-I now does
 *	copying instead of m-buf shuffleing.
 *	A number of deficencies in the hardware/firmware were compensated
 *	for. See comments in qestart and qerint.
 *
 *  14 Nov. 84 -- jf
 *	Added usage counts for multicast addresses.
 *	Updated general protocol support to allow access to the Ethernet
 *	header.
 *
 *  04 Oct. 84 -- jf
 *	Added support for new ioctls to add and delete multicast addresses
 *	and set the physical address.
 *	Add support for general protocols.
 *
 *  14 Aug. 84 -- rjl
 *	Integrated Shannon changes. (allow arp above 1024 and ? )
 *
 *  13 Feb. 84 -- rjl
 *
 *	Initial version of driver. derived from IL driver.
 * ---------------------------------------------------------------------
 */
 
#include "qe.h"
#if	NQE > 0
/*
 * Digital Q-BUS to NI Adapter
 */

#include "param.h"
#include "pdp/seg.h"
#include "pdp/psl.h"
#include "map.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "time.h"
#include "kernel.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef INET
#include "domain.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"
#endif

#ifdef NS
#include "../netns/ns.h"
#include "../netns/ns_if.h"
#endif

#include "if_qereg.h"
#include "if_uba.h"
#include "../pdpuba/ubavar.h"
 
#define NRCV	13	 		/* Receive descriptors (was 25) */
#define NXMT	5	 		/* Transmit descriptors		*/
#define NTOT	(NXMT + NRCV)
 
#define MINDATA 60
 
/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * is_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	qe_softc {
	struct	arpcom is_ac;		/* Ethernet common part 	*/
#define	is_if	is_ac.ac_if		/* network-visible interface 	*/
#define	is_addr	is_ac.ac_enaddr		/* hardware Ethernet address 	*/
	struct	ifuba qe_ifr[NRCV];	/*	for receive buffers;	*/
	struct	ifuba qe_ifw[NXMT];	/*	for xmit buffers;	*/
	int	qe_flags;		/* software state		*/
#define	QEF_RUNNING	0x01
#define	QEF_SETADDR	0x02
	long	setupaddr;		/* physaddr info for setup pkts  */
	long	rringaddr;		/* physaddr info for rings	*/
	long	tringaddr;		/*       ""			*/
	struct	qe_ring rring[NRCV+1];	/* Receive ring descriptors 	*/
	struct	qe_ring tring[NXMT+1];	/* Transmit ring descriptors 	*/
	u_char	setup_pkt[16][8];	/* Setup packet			*/
	u_char	rindex;			/* Receive index		*/
	u_char	tindex;			/* Transmit index		*/
	int	otindex;		/* Old transmit index		*/
	int	qe_intvec;		/* Interrupt vector 		*/
	struct	qedevice *addr;		/* device addr			*/
	u_char 	setupqueued;		/* setup packet queued		*/
	u_char	nxmit;			/* Transmits in progress	*/
	int	timeout;		/* watchdog			*/
	int	qe_restarts;		/* timeouts			*/
} qe_softc[NQE];

struct	uba_device *qeinfo[NQE];
 
int	qeattach(), qeintr(), qewatch(), qeinit(),qeoutput(),qeioctl();
 
extern struct ifnet loif;
u_short qestd[] = { 0 };
struct	uba_driver qedriver =
	{ 0, 0, qeattach, 0, qestd, "qe", qeinfo };
 
#define QE_TIMEO	(15)
#define	QEUNIT(x)	minor(x)
int watchrun = 0;			/* watchdog running	*/
/*
 * The deqna shouldn't receive more than ETHERMTU + sizeof(struct ether_header)
 * but will actually take in up to 2048 bytes. To guard against the receiver
 * chaining buffers (which we aren't prepared to handle) we allocate 2kb 
 * size buffers.
 */
#define MAXPACKETSIZE 2048		/* Should really be ETHERMTU	*/

/*
 * The C compiler's propensity for prepending '_'s to names is the reason
 * for the routine below.  We need the "handler" address (the code which
 * sets up the interrupt stack frame) in order to initialize the vector.
*/

static int qefoo()
	{
	asm("mov $qeintr, r0");		/* return value is in r0 */
	}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
qeattach(ui)
	struct uba_device *ui;
{
	register struct qe_softc *sc = &qe_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	register int i;
	int	islqa = 0;
	extern int nextiv();
 
	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "qe";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST;
 
	/*
	 * Read the address from the prom and save it.
	 */
	for	( i=0 ; i<6 ; i++ )
		sc->setup_pkt[i][1] = sc->is_addr[i] = addr->qe_sta_addr[i] & 0xff;  
	/*
 	 * Determine if this is a DEQNA or a DELQA...
	*/
	addr->qe_vector |= QE_VEC_ID;
	if	(addr->qe_vector & QE_VEC_ID)
		islqa = 1;
	addr->qe_vector &= ~QE_VEC_ID;
 
	/*
	 * Allocate a floating vector and initialize it with the address of
	 * the interrupt handler and PSW (supervisor mode, priority 4, unit
	 * number in the low bits.
	 */
	i = SKcall(nextiv, 0);
	sc->qe_intvec = i;
	mtkd(i, qefoo());
	mtkd(i+2, PSL_CURSUP | PSL_BR4 | ifp->if_unit);

	/*
	 * map the communications area onto the device 
	 */
	sc->rringaddr = startnet + (long)sc->rring;
	sc->tringaddr = startnet + (long)sc->tring;
	sc->setupaddr =	startnet + (long)sc->setup_pkt;

	/*
	 * init buffers and maps
	 */
	if (qbaini(sc->qe_ifr, NRCV) == 0)
		sc->is_if.if_flags &= ~IFF_UP;
	if (qbaini(sc->qe_ifw, NXMT) == 0)
		sc->is_if.if_flags &= ~IFF_UP;
 
	ifp->if_init = qeinit;
	ifp->if_output = qeoutput;
	ifp->if_ioctl = qeioctl;
	ifp->if_reset = 0;
	if_attach(ifp);

	printf("qe%d: DEC DE%sA addr %s\n",ifp->if_unit, islqa ? "LQ": "QN",
		ether_sprintf(&sc->is_addr));
}
 
/*
 * Initialization of interface. 
 */
qeinit(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct uba_device *ui = qeinfo[unit];
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	register struct ifnet *ifp = &sc->is_if;
	register i;
	int s;
 
	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
			return;
	/*
	 * Init the buffer descriptors and indexes for each of the lists and
	 * loop them back to form a ring.
	 */
	for (i = 0; i < NRCV; i++) {
		qeinitdesc(&sc->rring[i],
			sc->qe_ifr[i].ifu_r.ifrw_info, MAXPACKETSIZE);
		sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
		sc->rring[i].qe_valid = 1;
	}
	qeinitdesc(&sc->rring[i], (long)NULL, 0);
 
	sc->rring[i].qe_addr_lo = loint(sc->rringaddr);
	sc->rring[i].qe_addr_hi = hiint(sc->rringaddr);
	sc->rring[i].qe_chain = 1;
	sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
	sc->rring[i].qe_valid = 1;
 
	for( i = 0 ; i <= NXMT ; i++ )
		qeinitdesc(&sc->tring[i], (long)NULL, 0);
	i--;
 
	sc->tring[i].qe_addr_lo = loint(sc->tringaddr);
	sc->tring[i].qe_addr_hi = hiint(sc->tringaddr);
	sc->tring[i].qe_chain = 1;
	sc->tring[i].qe_flag = sc->tring[i].qe_status1 = QE_NOTYET;
	sc->tring[i].qe_valid = 1;
 
	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;
 
	/*
	 * Take the interface out of reset, program the vector, 
	 * enable interrupts, and tell the world we are up.
	 */
	s = splimp();
	addr->qe_vector = sc->qe_intvec;
	sc->addr = addr;
	addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT |
	    QE_RCV_INT | QE_ILOOP;
	addr->qe_rcvlist_lo = loint(sc->rringaddr);
	addr->qe_rcvlist_hi = hiint(sc->rringaddr);
	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	sc->qe_flags |= QEF_RUNNING;
	qesetup(sc);
	qestart(unit);
	splx(s);
 
}
 
/*
 * Start output on interface.
 *
 */
qestart(dev)
	dev_t dev;
{
	int unit = QEUNIT(dev);
	struct uba_device *ui = qeinfo[unit];
	register struct qe_softc *sc = &qe_softc[unit];
	register struct qedevice *addr;
	register struct qe_ring *rp;
	register index;
	struct mbuf *m;
	int len, s;
	long buf_addr;
 
	 
	s = splimp();
	addr = (struct qedevice *)ui->ui_addr;
	/*
	 * The deqna doesn't look at anything but the valid bit
	 * to determine if it should transmit this packet. If you have
	 * a ring and fill it the device will loop indefinately on the
	 * packet and continue to flood the net with packets until you
	 * break the ring. For this reason we never queue more than n-1
	 * packets in the transmit ring. 
	 *
	 * The microcoders should have obeyed their own defination of the
	 * flag and status words, but instead we have to compensate.
	 */
	for( index = sc->tindex; 
		sc->tring[index].qe_valid == 0 && sc->nxmit < (NXMT-1) ;
		sc->tindex = index = ++index % NXMT){
		rp = &sc->tring[index];
		if( sc->setupqueued ) {
			buf_addr = sc->setupaddr;
			len = 128;
			rp->qe_setup = 1;
			sc->setupqueued = 0;
		} else {
			IF_DEQUEUE(&sc->is_if.if_snd, m);
			if( m == 0 ){
				splx(s);
				return;
			}
			buf_addr = sc->qe_ifw[index].ifu_w.ifrw_info;
			len = if_wubaput(&sc->qe_ifw[index], m);
		}
		/*
		 *  Does buffer end on odd byte ? 
		 */
		if (len < MINDATA)
			len = MINDATA;
		if (len & 1) {
			len++;
			rp->qe_odd_end = 1;
		}
		rp->qe_buf_len = -(len/2);
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_addr_lo = loint(buf_addr);
		rp->qe_addr_hi = hiint(buf_addr);
		rp->qe_eomsg = 1;
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
		sc->nxmit++;
		/*
		 * If the watchdog time isn't running kick it.
		 */
		sc->timeout=1;
		if (watchrun == 0) { 
			watchrun++; 
			TIMEOUT(qewatch, (caddr_t)0, QE_TIMEO);
		}

		/*
		 * See if the xmit list is invalid.
		 */
		if( addr->qe_csr & QE_XL_INVALID ) {
			buf_addr = sc->tringaddr + (index * sizeof (struct qe_ring));
			addr->qe_xmtlist_lo = loint(buf_addr);
			addr->qe_xmtlist_hi = hiint(buf_addr);
		}
	}
	splx(s);
}
 
/*
 * Ethernet interface interrupt processor
 */
qeintr(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	struct qedevice *addr = (struct qedevice *)qeinfo[unit]->ui_addr;
	int s, csr;
	long buf_addr;
 
	s = splimp();
	csr = addr->qe_csr;
	addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	if (csr & QE_RCV_INT) 
		qerint(unit);
	if (csr & QE_XMIT_INT)
		qetint(unit);
	if (csr & QE_NEX_MEM_INT)
		panic("qe: Non existant memory interrupt");
	
	if( addr->qe_csr & QE_RL_INVALID && sc->rring[sc->rindex].qe_status1 == QE_NOTYET ) {
	    buf_addr = sc->rringaddr + (sc->rindex * sizeof(struct qe_ring));
		addr->qe_rcvlist_lo = loint(buf_addr);
		addr->qe_rcvlist_hi = hiint(buf_addr);
	}
	splx(s);
}
 
/*
 * Ethernet interface transmit interrupt.
 */
 
qetint(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct qe_ring *rp;
	int status1, setupflag;
	short len;
 
	while (sc->otindex != sc->tindex && sc->tring[sc->otindex].qe_status1 != QE_NOTYET && sc->nxmit > 0) {
		/*
		 * Save the status words from the descriptor so that it can
		 * be released.
		 */
		rp = &sc->tring[sc->otindex];
		status1 = rp->qe_status1;
		setupflag = rp->qe_setup;
		len = (-rp->qe_buf_len) * 2;
		if (rp->qe_odd_end)
			len++;
		/*
		 * Init the buffer descriptor
		 */
		bzero((caddr_t)rp, sizeof(struct qe_ring));
		if (--sc->nxmit == 0)
			sc->timeout = 0;
		if (!setupflag) {
			/*
			 * Do some statistics.
			 */
			sc->is_if.if_opackets++;
			sc->is_if.if_collisions += (status1 & QE_CCNT) >> 4;
			if (status1 & QE_ERROR)
				sc->is_if.if_oerrors++;
		}
		sc->otindex = ++sc->otindex % NXMT;
	}
	qestart(unit);
}
 
/*
 * Ethernet interface receiver interrupt.
 * If can't determine length from type, then have to drop packet.  
 * Othewise decapsulate packet based on type and pass to type specific 
 * higher-level input routine.
 */
qerint(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct qe_ring *rp;
	int len, status1, status2;
	long bufaddr;
 
	/*
	 * Traverse the receive ring looking for packets to pass back.
	 * The search is complete when we find a descriptor not in use.
	 *
	 * As in the transmit case the deqna doesn't honor it's own protocols
	 * so there exists the possibility that the device can beat us around
	 * the ring. The proper way to guard against this is to insure that
	 * there is always at least one invalid descriptor. We chose instead
	 * to make the ring large enough to minimize the problem. With a ring
	 * size of 4 we haven't been able to see the problem. To be safe we
	 * increased that to 5.
	 *
	 */

	for( ; sc->rring[sc->rindex].qe_status1 != QE_NOTYET ; sc->rindex = ++sc->rindex % NRCV ){
		rp = &sc->rring[sc->rindex];
		status1 = rp->qe_status1;
		status2 = rp->qe_status2;
		bzero((caddr_t)rp, sizeof(struct qe_ring));
		if ((status1 & QE_MASK) == QE_MASK)
			panic("qe: chained packet");
		len = ((status1 & QE_RBL_HI) | (status2 & QE_RBL_LO)) + 60;
		sc->is_if.if_ipackets++;
 
		if (status1 & QE_ERROR)
			sc->is_if.if_ierrors++;
		else {
			/*
			 * We don't process setup packets.
			 */
			if (!(status1 & QE_ESETUP))
				qeread(sc, &sc->qe_ifr[sc->rindex],
					len - sizeof(struct ether_header));
		}
		/*
		 * Return the buffer to the ring
		 */
		bufaddr = sc->qe_ifr[sc->rindex].ifu_r.ifrw_info;
		rp->qe_buf_len = -((MAXPACKETSIZE)/2);
		rp->qe_addr_lo = loint(bufaddr);
		rp->qe_addr_hi = hiint(bufaddr);
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
	}
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 */
qeoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int type, s, error;
	u_char edst[6];
	struct in_addr idst;
	register struct qe_softc *is = &qe_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct ether_header *eh;
	int usetrailers;
	struct mbuf *mcopy = (struct mbuf *)0;
 
	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		error = ENETDOWN;
		goto bad;
	}

	switch (dst->sa_family) {
 
#ifdef INET
	case AF_INET:
		idst = ((struct sockaddr_in *)dst)->sin_addr;
		if (!arpresolve(&is->is_ac, m, &idst, edst, &usetrailers))
			return (0);	/* if not yet resolved */
		if (!bcmp(edst, etherbroadcastaddr,sizeof (edst)))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		type = ETHERTYPE_IP;
		goto gottype;
#endif
#ifdef NS
	case AF_NS:
		type = ETHERTYPE_NS;
 		bcopy((caddr_t)&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
		    (caddr_t)edst, sizeof (edst));
		if (!bcmp(edst, &ns_broadcast, sizeof (edst)))
			return(looutput(&loif, m, dst));
		goto gottype;
#endif

 
	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
 		bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		type = eh->ether_type;
		goto gottype;
 
	default:
		printf("qe%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}
 
gottype:
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct ether_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct ether_header);
	} else {
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
	}
	eh = mtod(m, struct ether_header *);
	eh->ether_type = htons((u_short)type);
 	bcopy((caddr_t)edst, (caddr_t)eh->ether_dhost, sizeof (edst));
 	bcopy((caddr_t)is->is_addr, (caddr_t)eh->ether_shost, sizeof (is->is_addr));
 
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		if (mcopy)
			m_freem(mcopy);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	qestart(ifp->if_unit);
	splx(s);
	return(mcopy ? looutput(&loif, mcopy, dst) : 0);
 
bad:
	m_freem(m0);
	if (mcopy)
		m_freem(mcopy);
	return(error);
}
 
 
/*
 * Process an ioctl request.
 */
qeioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct qe_softc *sc = &qe_softc[ifp->if_unit];
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s = splimp(), error = 0;
 
	switch (cmd) {
 
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		qeinit(ifp->if_unit);
		switch(ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);
			
			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)(sc->is_addr);
			else
				qe_setaddr(ina->x_host.c_host, ifp->if_unit);
			break;
		    }
#endif
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    sc->qe_flags & QEF_RUNNING) {
			((struct qedevice *)
			   (qeinfo[ifp->if_unit]->ui_addr))->qe_csr = QE_RESET;
			sc->qe_flags &= ~QEF_RUNNING;
		} else if (ifp->if_flags & IFF_UP &&
		    (sc->qe_flags & QEF_RUNNING) == 0)
			qerestart(sc);
		break;

	default:
		error = EINVAL;
 
	}
	splx(s);
	return (error);
}
 
/*
 * set ethernet address for unit
 */
qe_setaddr(physaddr, unit)
	u_char *physaddr;
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register int i;

	for (i = 0; i < 6; i++)
		sc->setup_pkt[i][1] = sc->is_addr[i] = physaddr[i];
	sc->qe_flags |= QEF_SETADDR;
	if (sc->is_if.if_flags & IFF_RUNNING)
		qesetup(sc);
	qeinit(unit);
}
 
 
/*
 * Initialize a ring descriptor with mbuf allocation side effects
 */
qeinitdesc(rp, addr, len)
	register struct qe_ring *rp;
	long addr; 			/* physical address */
	int len;
{
	/*
	 * clear the entire descriptor
	 */
	bzero((caddr_t)rp, sizeof(struct qe_ring));
 
	if( len ) {
		rp->qe_buf_len = -(len/2);
		rp->qe_addr_lo = loint(addr);
		rp->qe_addr_hi = hiint(addr);
	}
}
/*
 * Build a setup packet - the physical address will already be present
 * in first column.
 */
qesetup( sc )
struct qe_softc *sc;
{
	register i, j;
 
	/*
	 * Copy the target address to the rest of the entries in this row.
	 */
	 for ( j = 0; j < 6 ; j++ )
		for ( i = 2 ; i < 8 ; i++ )
			sc->setup_pkt[j][i] = sc->setup_pkt[j][1];
	/*
	 * Duplicate the first half.
	 */
	bcopy((caddr_t)sc->setup_pkt[0], (caddr_t)sc->setup_pkt[8], 64);
	/*
	 * Fill in the broadcast address.
	 */
	for ( i = 0; i < 6 ; i++ )
		sc->setup_pkt[i][2] = 0xff;
	sc->setupqueued++;
}

/*
 * Pass a packet to the higher levels.
 */
qeread(sc, ifuba, len)
	register struct qe_softc *sc;
	struct ifuba *ifuba;
	int len;
{
	struct ether_header *eh;
    	struct mbuf *m;
	struct ifqueue *inq;
	register int type;
	segm seg5;
 
	/*
	 * Count trailers as errors and drop the packet.
	 * SEG5 is mapped out briefly to peek at the packet type, swap the
	 * bytes and then SEG5 is restored.
	 */
 
	saveseg5(seg5);
	mapseg5(ifuba->ifu_r.ifrw_click, 077406);	/* 8k r/w for 1 word */
	eh = (struct ether_header *)SEG5;
	eh->ether_type = ntohs((u_short)eh->ether_type);
	type = eh->ether_type;
	restorseg5(seg5);
	if (len == 0 || type >= ETHERTYPE_TRAIL &&
	    type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		sc->is_if.if_ierrors++;
		return;
	}
 
	/*
	 * Pull packet off interface.
	 */
	m = if_rubaget(ifuba, len, 0, &sc->is_if);
 
	if (m == 0)
		return;
 
	switch (type) {

#ifdef INET
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		arpinput(&sc->is_ac, m);
		return;
#endif
#ifdef NS
	case ETHERTYPE_NS:
		schednetisr(NETISR_NS);
		inq = &nsintrq;
		break;

#endif
 
	default:
		m_freem(m);
		return;
	}
 
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
}

/*
 * Watchdog timer routine. There is a condition in the hardware that
 * causes the board to lock up under heavy load. This routine detects
 * the hang up and restarts the device.
 */
qewatch()
{
	register struct qe_softc *sc;
	register int i;
	int inprogress=0;
 
	for (i = 0; i < NQE; i++) {
		sc = &qe_softc[i];
		if (sc->timeout) 
			if (++sc->timeout > 3 ) {
				printf("qerestart: restarted qe%d %d\n",
				     i, ++sc->qe_restarts);
				qerestart(sc);
			} else
				inprogress++;
	}
	if (inprogress) {
		TIMEOUT(qewatch, (caddr_t)0, QE_TIMEO);
		watchrun++;
	} else
		watchrun=0;
}
/*
 * Restart for board lockup problem.
 */
qerestart(sc)
	register struct qe_softc *sc;
{
	register struct ifnet *ifp = &sc->is_if;
	register struct qedevice *addr = sc->addr;
	register struct qe_ring *rp;
	register i;
 
	addr->qe_csr = QE_RESET;
	addr->qe_csr &= ~QE_RESET;
	sc->timeout = 0;
	qesetup( sc );
	for (i = 0, rp = sc->tring; i < NXMT; rp++, i++) {
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 0;
	}
	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;
	addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT |
	    QE_RCV_INT | QE_ILOOP;
	addr->qe_rcvlist_lo = loint(sc->rringaddr);
	addr->qe_rcvlist_hi = hiint(sc->rringaddr);
	sc->qe_flags |= QEF_RUNNING;
	qestart(ifp->if_unit);
}

qbaini(ifuba, num)
	struct ifuba *ifuba;
	int num;
{
	register int i;
	register memaddr click;

	for (i = 0; i < num; i++) {
		click = m_ioget(MAXPACKETSIZE);
		if (click == 0) {
			click = MALLOC(coremap, btoc(MAXPACKETSIZE));
			if (click == 0) {
				printf("qe: can't get dma memory\n");
				return(0);
			}
		}
		ifuba[i].ifu_hlen = sizeof (struct ether_header);
		ifuba[i].ifu_w.ifrw_click = ifuba[i].ifu_r.ifrw_click = click;
		ifuba[i].ifu_w.ifrw_info = ifuba[i].ifu_r.ifrw_info = 
			ctob((long)click);
	}
	return(1);
}
#endif
