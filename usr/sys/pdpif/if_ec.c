/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_ec.c	7.1 (Berkeley) 6/5/86
 *			1.0 (2.10BSD  EATON IMSD sms@etn-wlv.eaton.com 12/28/87)
*/

#include "ec.h"
#if NEC > 0

/*
 * 3Com Ethernet Controller interface
 *
 * Adapted from the 4.3BSD version to run on 2.10BSD.  Major differences:
 *
 *	1) static instead of autoconfiguration, 'autoconfig' has problems with
 *	   our triple vector.
 *	2) reduction of the number of buffers from 16 to 12, this is because
 *	   the unibus memory for this device is 'enabled' by clipping resistors
 *	   on the cpu board and must start on a mod 4 UMR boundary and we only
 *	   have UMRs 28, 29 and 30.  The number of UMRs available has to be
 *	   reduced in ubinit() (/sys/pdp/machdep2.c) if this driver is to 
 *	   be used.
 *	3) Buffers are mapped thru APR5, copyv() is used to go to/from mbufs.
 *	4) Exponential backup redone to use a count instead of a mask.
 *	5) The ec_softc structure reduced in size to remove unused fields.
 *	6) Modified to run in supervisor mode, about all this required was
 * 	   changing the macros in seg.h to use supervisor registers if 
 *	   SUPERVISOR is defined (from the makefiles) AND changing where
 *	   SEG5 is mapped in/out.  Use mbcopyin/out instead of copyv.
 *	7) Broken ethernet cable showed up a problem in collision handling,
 *	   the backoff being done via a DELAY loop effectively hangs the system.
 *	   Changed to use a timeout with the number of ticks being the number
 *	   of collisions (up to 16 max).
 *
 * Who knows if trailers and NS work, i don't.  Coding style changed to reflect
 * personal preferences, conditional vax code removed to improve readability.
 *
 * Oh, one more thing.  The 3Com board is hardwired to interrupt at spl6 for
 * the receiver, spl5 for the collision detect, and spl4 for the transmitter.
 * References to splimp() have been replaced in this driver with splhigh().
 * you'll have to change splimp() to be spl6 and recompile the whole kernel
 * in order to avoid recursive interrupts caused by the receiver using splimp
 * anywhere in its path.  TRUST ME, you crash if you don't do this.  Better
 * to lose a few clock ticks than the system!
*/

#include "param.h"
#include "../machine/seg.h"
#include "mbuf.h"
#include "buf.h"
#include "domain.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef INET
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

#include "if_ecreg.h"
#include "if_uba.h"
#include "../pdpuba/ubavar.h"

#define	MAPBUFDESC	(((btoc(2048) - 1) << 8 ) | RW)
#define	BUFP		((caddr_t)0120000)
#undef	ECRHBF
#define	ECRHBF		11
#define	ECNUMBUFS	(ECRHBF + 1)

extern	struct	ifnet loif;
	int	ecattach(), ecinit(), ecioctl(), ecoutput(), ecunjam();
	struct	uba_device *ecinfo[NEC];
	u_short ecstd[] = { 0 };
	struct	uba_driver ecdriver =
		{ 0, 0, ecattach, 0, ecstd, "ec", ecinfo };
	struct	mbuf *ecget();

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
*/
	struct	ec_softc {
		struct	arpcom es_ac;	/* common Ethernet structures */
#define	es_if	es_ac.ac_if		/* network-visible interface */
#define	es_addr	es_ac.ac_enaddr		/* hardware Ethernet address */
		u_char	es_mask;	/* mask for current output delay */
		u_char	es_oactive;	/* is output active? */
		memaddr	es_buf[ECNUMBUFS]; /* virtual click buffer addresses */
		} ec_softc[NEC];

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
ecattach(ui)
	struct uba_device *ui;
	{
	struct ec_softc *es = &ec_softc[ui->ui_unit];
register struct ifnet *ifp = &es->es_if;
register struct ecdevice *addr = (struct ecdevice *)ui->ui_addr;
	int i, j;
	u_char *cp;

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "ec";
	ifp->if_mtu = ETHERMTU;

/* Read the ethernet address off the board, one nibble at a time.  */

	addr->ec_xcr = EC_UECLR; /* zero address pointer */
	addr->ec_rcr = EC_AROM;
	cp = es->es_addr;
#define	NEXTBIT	addr->ec_rcr = EC_AROM|EC_ASTEP; addr->ec_rcr = EC_AROM
	for	(i=0; i < sizeof (es->es_addr); i++)
		{
		*cp = 0;
		for	(j=0; j<=4; j+=4)
			{
			*cp |= ((addr->ec_rcr >> 8) & 0xf) << j;
			NEXTBIT; NEXTBIT; NEXTBIT; NEXTBIT;
			}
		cp++;
		}
	printf("ec%d: addr %s\n", ui->ui_unit, ether_sprintf(es->es_addr));
	ifp->if_init = ecinit;
	ifp->if_ioctl = ecioctl;
	ifp->if_output = ecoutput;
	ifp->if_reset = 0;
	ifp->if_flags = IFF_BROADCAST;
/*
 * the (memaddr)(0177000) below is UMR28 translated into clicks.
*/
	for	(i = 0; i < ECNUMBUFS; i++)
		es->es_buf[i] = (memaddr)(0177000) + (memaddr)(btoc(2048) * i);
	if_attach(ifp);
	}

/*
 * Initialization of interface; clear recorded pending operations.
*/
ecinit(unit)
	int unit;
	{
	struct	ec_softc *es = &ec_softc[unit];
	struct	ecdevice *addr;
register struct ifnet *ifp = &es->es_if;
	int	i, s;

	/* not yet, if address still unknown */
	if	(ifp->if_addrlist == (struct ifaddr *)0)
		return;

	/*
	 * Hang receive buffers and start any pending writes.
	 * Writing into the rcr also makes sure the memory
	 * is turned on.
	 */
	if	((ifp->if_flags & IFF_RUNNING) == 0)
		{
		addr = (struct ecdevice *)ecinfo[unit]->ui_addr;
		s = splhigh();
		/*
		 * write our ethernet address into the address recognition ROM 
		 * so we can always use the same EC_READ bits (referencing ROM),
		 * in case we change the address sometime.
		 * Note that this is safe here as the receiver is NOT armed.
		 */
		ec_setaddr(es->es_addr, unit);
		/*
		 * Arm the receiver
		 */
		for	(i = ECRHBF; i >= ECRLBF; i--)
			addr->ec_rcr = EC_READ | i;
		es->es_oactive = 0;
		es->es_mask = 1;
		es->es_if.if_flags |= IFF_RUNNING;
		if	(es->es_if.if_snd.ifq_head)
			ecstart(unit);
		splx(s);
		}
	}

/*
 * Start output on interface.  Get another datagram to send
 * off of the interface queue, and copy it to the interface
 * before starting the output.
 */
ecstart(unit)
	int	unit;
	{
register struct ec_softc *es = &ec_softc[unit];
	struct	ecdevice *addr;
	struct	mbuf *m;

	if	((es->es_if.if_flags & IFF_RUNNING) == 0)
		return;
	IF_DEQUEUE(&es->es_if.if_snd, m);
	if	(m == 0)
		return;
	ecput(es->es_buf[ECTBF], m);
	addr = (struct ecdevice *)ecinfo[unit]->ui_addr;
	addr->ec_xcr = EC_WRITE|ECTBF;
	es->es_oactive = 1;
	}

/*
 * Ethernet interface transmitter interrupt.
 * Start another output if more data to send.
 */
ecxint(unit)
	int unit;
	{
register struct ec_softc *es = &ec_softc[unit];
register struct ecdevice *addr = (struct ecdevice *)ecinfo[unit]->ui_addr;
register int s;

	if	(es->es_oactive == 0)
		return;
	if	(!(addr->ec_xcr&EC_XDONE) || (addr->ec_xcr&EC_XBN) != ECTBF)
		{
		printf("ec%d: stray xint, xcr=%b\n",unit,addr->ec_xcr,EC_XBITS);
		es->es_oactive = 0;
		addr->ec_xcr = EC_XCLR;
		return;
		}
	es->es_if.if_opackets++;
	es->es_oactive = 0;
	es->es_mask = 1;
	addr->ec_xcr = EC_XCLR;
	s = splimp();
	if	(es->es_if.if_snd.ifq_head)
		ecstart(unit);
	splx(s);
	}

/*
 * Collision on ethernet interface.  Do exponential
 * backoff, and retransmit.  If have backed off all
 * the way print warning diagnostic, and drop packet.
 */
eccollide(unit)
	int unit;
	{
register struct ec_softc *es = &ec_softc[unit];
register struct ecdevice *addr = (struct ecdevice *)ecinfo[unit]->ui_addr;
register int	i;
	long	delay;

	es->es_if.if_collisions++;
	if	(es->es_oactive == 0)
		return;
	if	(es->es_mask++ >= 16)
		{
		es->es_if.if_oerrors++;
		printf("ec%d: send err\n", unit);
		/*
		 * Reset interface, then requeue rcv buffers.
		 * Some incoming packets may be lost, but that
		 * can't be helped.
		 */
		addr->ec_xcr = EC_UECLR;
		for	(i=ECRHBF; i>=ECRLBF; i--)
			addr->ec_rcr = EC_READ|i;
		/*
		 * Reset and transmit next packet (if any).
		 */
		es->es_oactive = 0;
		es->es_mask = 1;
		if	(es->es_if.if_snd.ifq_head)
			ecstart(unit);
		return;
		}
	/*
	 * use a timeout instead of a delay loop - the loop hung the system
	 * when someone unscrewed a terminator on the net.
	 *
	 * this isn't exponential, but it sure beats a hung up system in the
	 * face of a broken cable.
	 */
	TIMEOUT(ecunjam, addr, es->es_mask);
	}

ecunjam(addr)
	struct	ecdevice *addr;
	{

	/*
	 * Clear the controller's collision flag, thus enabling retransmit.
	 */
	addr->ec_xcr = EC_CLEAR;
	}

/*
 * Ethernet interface receiver interrupt.
 * If input error just drop packet.
 * Otherwise examine packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */

ecrint(unit)
	int unit;
	{
	struct ecdevice *addr = (struct ecdevice *)ecinfo[unit]->ui_addr;

	while	(addr->ec_rcr & EC_RDONE)
		ecread(unit);
	}

ecread(unit)
	int unit;
	{
register struct ec_softc *es = &ec_softc[unit];
register struct ether_header *ec;
register struct ifqueue *inq;
	struct	ecdevice *addr = (struct ecdevice *)ecinfo[unit]->ui_addr;
    	struct	mbuf *m;
	int	len, off = 0, resid, ecoff, rbuf, type;
	u_char	*ecbuf;
	segm	sav5;

	es->es_if.if_ipackets++;
	rbuf = addr->ec_rcr & EC_RBN;
	if	(rbuf < ECRLBF || rbuf > ECRHBF)
		panic("ecrint");	/* sanity */
/*
 * we change SDSA5 only while NOT looking at mbufs (there might be some on SEG5)
 * and carefully restore it before calling 'ecget' who uses mbcopyin().
 * the save/restore seg routines are ifdef'd on SUPERVISOR (which had better
 * be set when compiling the network stuff!!).
*/
	saveseg5(sav5);
	mapseg5(es->es_buf[rbuf], MAPBUFDESC);
	ecbuf = (u_char *)SEG5;
	ecoff = *(short *)SEG5;
	if	(ecoff <= ECRDOFF || ecoff > 2046)
		{
		es->es_if.if_ierrors++;
#ifdef	notyet
		printf("ec%d ecoff=0%o rbuf=0%o\n", unit, ecoff, rbuf);
#endif
		goto setup;
		}

	/*
	 * Get input data length.
	 * Get pointer to ethernet header (in input buffer).
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	len = ecoff - ECRDOFF - sizeof (struct ether_header);
	ec = (struct ether_header *)(ecbuf + ECRDOFF);
	ec->ether_type = ntohs((u_short)ec->ether_type);
#ifdef weliketrailers
#define ecdataaddr(ec, off, type) ((type)(((caddr_t)((ec)+1)+(off))))
	if	(ec->ether_type >= ETHERTYPE_TRAIL &&
		    ec->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)
		{
		off = (ec->ether_type - ETHERTYPE_TRAIL) * 512;
		if	(off >= ETHERMTU)
			goto setup;		/* sanity */
		ec->ether_type = ntohs(*ecdataaddr(ec, off, u_short *));
		resid = ntohs(*(ecdataaddr(ec, off+2, u_short *)));
		if	(off + resid > len)
			goto setup;		/* sanity */
		len = off + resid;
		}
	else
		off = 0;
	if	(!len)
		goto setup;
#else
	if	(!len || (ec->ether_type >= ETHERTYPE_TRAIL &&
		    ec->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER))
		{
		printf("ec%d type=%x len=%d\n", unit, ec->ether_type,len);
		es->es_if.if_ierrors++;
		goto setup;
		}
#endif
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; ecget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	type = ec->ether_type;		/* save before restoring mapping */
	restorseg5(sav5);		/* put it back now! */
	m = ecget(es->es_buf[rbuf], len, off, &es->es_if);
	if	(m == 0)
		goto setup;
#ifdef weliketrailers
	if	(off)
		{
		struct	ifnet *ifp;
		
		ifp = *(mtod(m, struct ifnet **));
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
		*(mtod(m, struct ifnet **)) = ifp;
		}
#endif
	switch	(type)
		{
#ifdef INET
		case	ETHERTYPE_IP:
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;
		case	ETHERTYPE_ARP:
			arpinput(&es->es_ac, m);
			goto setup;
#endif
#ifdef NS
		case	ETHERTYPE_NS:
			schednetisr(NETISR_NS);
			inq = &nsintrq;
			break;
#endif
		default:
			m_freem(m);
			goto setup;
		}
	if	(IF_QFULL(inq))
		{
		IF_DROP(inq);
		m_freem(m);
		goto setup;
		}
	IF_ENQUEUE(inq, m);
setup:
	/* Reset for next packet. */
	restorseg5(sav5);		/* put it back before leaving */
	addr->ec_rcr = EC_READ|EC_RCLR|rbuf;
	}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder and trailers
 * are allowed (which we should NEVER do).
 * If destination is this address or broadcast, send packet to
 * loop device to kludge around the fact that 3com interfaces can't
 * talk to themselves.
 */
ecoutput(ifp, m0, dst)
	struct	ifnet *ifp;
	struct	mbuf *m0;
	struct	sockaddr *dst;
	{
	int	type, s, error, usetrailers;
 	u_char	edst[6];
	struct	in_addr idst;
	struct	ec_softc *es = &ec_softc[ifp->if_unit];
register struct mbuf *m = m0;
register struct ether_header *ec;
register int off;
	u_short	*p;
	struct	mbuf *mcopy = (struct mbuf *)0;

	if	((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		{
		error = ENETDOWN;
		goto bad;
		}
	switch	(dst->sa_family)
		{
#ifdef INET
		case	AF_INET:
			idst = ((struct sockaddr_in *)dst)->sin_addr;
			if	(!arpresolve(&es->es_ac, m, &idst, edst,
					&usetrailers))
				return(0);	/* if not yet resolved */
			if	(!bcmp(edst, etherbroadcastaddr, sizeof(edst)))
				mcopy = m_copy(m, 0, (int)M_COPYALL);
			off = ntohs((u_short)mtod(m, struct ip *)->ip_len) 
					- m->m_len;
		/* need per host negotiation */
			if	(usetrailers && off > 0 && (off & 0x1ff) == 0 &&
				    m->m_off >= MMINOFF + 2 * sizeof (u_short))
				{
				type = ETHERTYPE_TRAIL + (off>>9);
				m->m_off -= 2 * sizeof (u_short);
				m->m_len += 2 * sizeof (u_short);
				p = mtod(m, u_short *);
				*p++ =ntohs((u_short)ETHERTYPE_IP);
				*p = ntohs((u_short)m->m_len);
				goto gottrailertype;
				}
			type = ETHERTYPE_IP;
			off = 0;
			goto gottype;
#endif
#ifdef NS
		case	AF_NS:
			bcopy(&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
				    edst, sizeof (edst));
			if	(!bcmp((caddr_t)edst, (caddr_t)&ns_broadhost,
					sizeof(edst)))
				mcopy = m_copy(m, 0, (int)M_COPYALL);
			else if (!bcmp((caddr_t)edst, (caddr_t)&ns_thishost,
					sizeof(edst)))
				return(looutput(&loif, m, dst));
			type = ETHERTYPE_NS;
			off = 0;
			goto gottype;
#endif
		case	AF_UNSPEC:
			ec = (struct ether_header *)dst->sa_data;
			bcopy(ec->ether_dhost, (caddr_t)edst, sizeof (edst));
			type = ec->ether_type;
			goto gottype;
		default:
			printf("ec%d: af%d\n", ifp->if_unit, dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
		}
gottrailertype:
	/*
	 * Packet to be sent as trailer: move first packet
	 * (control information) to end of chain.
	 */
	while	(m->m_next)
		m = m->m_next;
	m->m_next = m0;
	m = m0->m_next;
	m0->m_next = 0;
	m0 = m;
gottype:
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if	(m->m_off > MMAXOFF ||
		    MMINOFF + sizeof (struct ether_header) > m->m_off)
		{
		m = m_get(M_DONTWAIT, MT_HEADER);
		if	(m == 0)
			{
			error = ENOBUFS;
			goto bad;
			}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct ether_header);
		}
	else
		{
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
		}
	ec = mtod(m, struct ether_header *);
 	bcopy((caddr_t)edst, (caddr_t)ec->ether_dhost, sizeof (edst));
	bcopy(es->es_addr, (caddr_t)ec->ether_shost, sizeof(ec->ether_shost));
	ec->ether_type = htons((u_short)type);

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splhigh();
	if	(IF_QFULL(&ifp->if_snd))
		{
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto qfull;
		}
	IF_ENQUEUE(&ifp->if_snd, m);
	if	(es->es_oactive == 0)
		ecstart(ifp->if_unit);
	splx(s);
	error = mcopy ? looutput(&loif, mcopy, dst) : 0;
	return(error);
qfull:
	m0 = m;
	splx(s);
bad:
	m_freem(m0);
	if	(mcopy)
		m_freem(mcopy);
	return(error);
	}

/*
 * Routine to copy from mbuf chain to transmit
 * buffer in UNIBUS memory.
 * If packet size is less than the minimum legal size,
 * the buffer is expanded.  We probably should zero out the extra
 * bytes for security, but that would slow things down.
 */
ecput(ecbuf, m)
	memaddr	ecbuf;
	struct	mbuf *m;
	{
register struct mbuf *mp;
register u_short off;
	segm	sav5;

	for	(off = 2048, mp = m; mp; mp = mp->m_next)
		off -= mp->m_len;
	if	(2048 - off < ETHERMIN + sizeof (struct ether_header))
		off = 2048 - ETHERMIN - sizeof (struct ether_header);
	saveseg5(sav5);
	mapseg5(ecbuf, MAPBUFDESC);
	*(u_short *)SEG5 = off;
	restorseg5(sav5);
	for	(mp = m; mp; mp = mp->m_next)
		{
		register unsigned len = mp->m_len;

		if	(len == 0)
			continue;
		mbcopyout(mtod(mp, caddr_t), ecbuf, off, len);
		off += len;
		}
	m_freem(m);
	}

/*
 * Routine to copy from UNIBUS memory into mbufs.
*/
struct mbuf *
ecget(ecbuf, totlen, off0, ifp)
	memaddr	ecbuf;
	int	totlen, off0;
	struct	ifnet *ifp;
	{
register struct mbuf *m;
register int off = off0, len;
	struct	mbuf *top = 0, **mp = &top;
	u_short	distance;

	distance = ECRDOFF + sizeof (struct ether_header);
	while	(totlen > 0)
		{
		MGET(m, M_DONTWAIT, MT_DATA);
		if	(m == 0)
			goto bad;
		if	(off)
			{
			len = totlen - off;
			distance = ECRDOFF + off + sizeof (struct ether_header);
			}
		else
			len = totlen;
		if	(ifp)
			len += sizeof(ifp);
		if	(len >= NBPG)
			{
			MCLGET(m);
			if	(m->m_len == CLBYTES)
				m->m_len = len = MIN(len, CLBYTES);
			else
				m->m_len = len = MIN(MLEN, len);
			}
		else
			m->m_len = len = MIN(MLEN, len);
		if	(ifp)
			{
			/* Prepend interface pointer to first mbuf.  */
			*(mtod(m, struct ifnet **)) = ifp;
			len -= sizeof(ifp);
			}
		mbcopyin(ecbuf, distance, 
		      ifp ? mtod(m,caddr_t)+sizeof(ifp) : mtod(m,caddr_t), len);
		ifp = (struct ifnet *)0;
		distance += len;
		*mp = m;
		mp = &m->m_next;
		if	(off == 0)
			{
			totlen -= len;
			continue;
			}
		off += len;
		if	(off == totlen)
			{
			distance = ECRDOFF + sizeof (struct ether_header);
			off = 0;
			totlen = off0;
			}
		}
	return(top);
bad:
	m_freem(top);
	return(0);
	}

/*
 * Process an ioctl request.
 */
ecioctl(ifp, cmd, data)
register struct ifnet *ifp;
	int	cmd;
	caddr_t	data;
	{
register struct ifaddr *ifa = (struct ifaddr *)data;
	struct	ec_softc *es = &ec_softc[ifp->if_unit];
	struct	ecdevice *addr;
	int	s = splhigh(), error = 0;

	addr = (struct ecdevice *)ecinfo[ifp->if_unit]->ui_addr;
	switch	(cmd)
		{
		case	SIOCSIFADDR:
			ifp->if_flags |= IFF_UP;
			switch	(ifa->ifa_addr.sa_family)
				{
#ifdef INET
				case	AF_INET:
					ecinit(ifp->if_unit); 
					((struct arpcom *)ifp)->ac_ipaddr =
						IA_SIN(ifa)->sin_addr;
					arpwhohas((struct arpcom *)ifp, 
						&IA_SIN(ifa)->sin_addr);
					break;
#endif
#ifdef NS
				case	AF_NS:
					{
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

					if	(ns_nullhost(*ina))
					ina->x_host = *(union ns_host *)
							(es->es_addr);
					else
						{
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
						ifp->if_flags &= ~IFF_RUNNING; 
						bcopy(ina->x_host.c_host,
							es->es_addr,
							sizeof(es->es_addr));
						}
					ecinit(ifp->if_unit); /* do ec_setaddr*/
					break;
					}
#endif
				default:
					ecinit(ifp->if_unit);
					break;
				}
			break;
		case	SIOCSIFFLAGS:
			if	((ifp->if_flags & IFF_UP) == 0 &&
				  ifp->if_flags & IFF_RUNNING)
				{
				addr->ec_xcr = EC_UECLR;
				ifp->if_flags &= ~IFF_RUNNING;
				}
			else if (ifp->if_flags & IFF_UP &&
				  (ifp->if_flags & IFF_RUNNING) == 0)
				ecinit(ifp->if_unit);
			break;
		default:
			error = EINVAL;
		}
	splx(s);
	return(error);
	}

ec_setaddr(physaddr,unit)
	u_char	*physaddr;
	int	unit;
	{
	struct	ec_softc *es = &ec_softc[unit];
	struct	uba_device *ui = ecinfo[unit];
	struct	ecdevice *addr = (struct ecdevice *)ui->ui_addr;
register char nibble;
register int i, j;

	/*
	 * Use the ethernet address supplied
	 * Note that we do a UECLR here, so the receive buffers
	 * must be requeued.
	 */
	
#ifdef DEBUG
	printf("ec_setaddr: setting address for unit %d = %s",
		unit, ether_sprintf(physaddr));
#endif
	addr->ec_xcr = EC_UECLR;
	addr->ec_rcr = 0;
	/* load requested address */
	for	(i = 0; i < 6; i++)
		{ /* 6 bytes of address */
		es->es_addr[i] = physaddr[i];
		nibble = physaddr[i] & 0xf; /* lower nibble */
		addr->ec_rcr = (nibble << 8);
		addr->ec_rcr = (nibble << 8) + EC_AWCLK; /* latch nibble */
		addr->ec_rcr = (nibble << 8);
		for	(j=0; j < 4; j++)
			{
			addr->ec_rcr = 0;
			addr->ec_rcr = EC_ASTEP; /* step counter */
			addr->ec_rcr = 0;
			}
		nibble = (physaddr[i] >> 4) & 0xf; /* upper nibble */
		addr->ec_rcr = (nibble << 8);
		addr->ec_rcr = (nibble << 8) + EC_AWCLK; /* latch nibble */
		addr->ec_rcr = (nibble << 8);
		for	(j=0; j < 4; j++)
			{
			addr->ec_rcr = 0;
			addr->ec_rcr = EC_ASTEP; /* step counter */
			addr->ec_rcr = 0;
			}
		}
#ifdef DEBUG
	/*
	 * Read the ethernet address off the board, one nibble at a time.
	 */
	addr->ec_xcr = EC_UECLR;
	addr->ec_rcr = 0; /* read RAM */
	cp = es->es_addr;
#undef NEXTBIT
#define	NEXTBIT	addr->ec_rcr = EC_ASTEP; addr->ec_rcr = 0
	for	(i=0; i < sizeof (es->es_addr); i++)
		{
		*cp = 0;
		for	(j=0; j<=4; j+=4)
			{
			*cp |= ((addr->ec_rcr >> 8) & 0xf) << j;
			NEXTBIT; NEXTBIT; NEXTBIT; NEXTBIT;
			}
		cp++;
		}
	printf("ec_setaddr: RAM address for unit %d = %s",
		unit, ether_sprintf(physaddr));
#endif
	}
#endif
