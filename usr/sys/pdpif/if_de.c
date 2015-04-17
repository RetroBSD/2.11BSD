/*
 * SCCSID: @(#)if_de.c	1.1	(2.11BSD GTE)	12/31/93
 *	2.11BSD - Remove dereset since 1) it was never called, and 2)
 *		  wouldn't work if it were called. Also uballoc and
 *		  ubmalloc calling convention changed. - sms
 */
#include "de.h"
#if NDE > 0
/*
 * DEC DEUNA interface
 *
 *	Lou Salkind
 *	New York University
 *
 * TODO:
 *	timeout routine (get statistics)
 */

#include "param.h"
#include "../machine/seg.h"
#include "systm.h"
#include "mbuf.h"
#include "domain.h"
#include "protosw.h"
#include "ioctl.h"
#include "errno.h"
#include "time.h"

/*
 * Define DE_DO_BCTRS to get the DEUNA/DELUA to add code to allow 
 * ioctl() for clearing/getting I/O stats from the board.
#define DE_DO_BCTRS
 */

/*
 * Define DE_DO_PHYSADDR to get the DEUNA/DELUA to add code to allow 
 * ioctl() for setting/getting physical hardware address.
#define DE_DO_PHYSADDR
 */

/*
 * Define DE_DO_MULTI to get the DEUNA/DELUA to handle multi-cast addresses
#define DE_DO_MULTI
 */

/*
 * Define DE_INT_LOOPBACK to get the DEUNA/DELUA internal loopback code turned on
#define DE_INT_LOOPBACK
 */

/*
 * Define DE_DEBUG to get the DEUNA/DELUA debug code turned on
#define DE_DEBUG
 */

#include "../pdpif/if_de.h"

#ifndef YES
#define YES	1
#else		/* YES */
#undef	YES
#define	YES	1
#endif		/* YES */
#ifndef NO
#define NO	0
#else		/* NO */
#undef	NO
#define	NO	0
#endif		/* NO */

#define	MAPBUFDESC	(((btoc(ETHERMTU+sizeof(struct ether_header)) - 1) << 8 ) | RW)
#define	CRC_LEN		4	/* Length of CRC added to packet by board */

struct de_softc de_softc[NDE];  

#ifdef	DE_DEBUG
/*
 * Setting dedebug turns on the level of debugging, iff DE_DEBUG
 * was defined at compile time.  The current levels are:
 * 1 - added informative messages and error messages
 * 2 - more added messages
 * 3 - still more messages, like one for every loop packet seen, etc.
 */
#define	DE_DEBUG_LEVEL 1
int dedebug = DE_DEBUG_LEVEL;
#endif		/* DE_DEBUG */

#ifdef DE_DO_MULTI
u_char unused_multi[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#endif		/* DE_DO_MULTI */
#ifdef not_on_pdp
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
#endif		/* not_on_pdp */

int	deprobe(), deattach(), deintr(), deinit(), deoutput(), deioctl();

struct	mbuf *deget();

u_short destd[] = { 0174510 };

struct	uba_device *deinfo[NDE];

struct	uba_driver dedriver = 
	{ deprobe, 0, deattach, 0, destd, "de", deinfo };

#ifdef	DE_INT_LOOPBACK
#define	DELUA_LOOP_LEN	(32 - sizeof(struct ether_header))
#endif	/* DE_INT_LOOPBACK */

#ifdef	DE_DO_BCTRS
extern	struct timeval time;
struct	timeval atv;
#endif		/* DE_DO_BCTRS */

deprobe(reg)
	caddr_t reg;
{
#ifdef not_on_pdp
	register int br, cvec;		/* r11, r10 value-result */
	register struct dedevice *addr = (struct dedevice *)reg;
	register i;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	i = 0; derint(i); deintr(i);
#endif		/* lint */

       /*
        * Make sure self-test is finished 
        * Self-test on a DELUA can take 15 seconds.
        */
        for (i = 0; i < 160 &&
             (addr->pcsr0 & PCSR0_FATI) == 0 &&
             (addr->pcsr1 & PCSR1_STMASK) == STAT_RESET;
             ++i)
                DELAY(100000);
        if ((addr->pcsr0 & PCSR0_FATI) != 0 ||
            (addr->pcsr1 & PCSR1_STMASK) != STAT_READY)
                return(0);

        addr->pcsr0 = 0;


	addr->pcsr0 = PCSR0_RSET;

	/*
	 * just in case this is not a deuna or delua 
	 * dont wait for more than 30 secs
         */
        for (i = 0; i < 300 && (addr->pcsr0 & PCSR0_INTR) == 0; i++)
                DELAY(100000);
        if ((addr->pcsr0 & PCSR0_INTR) == 0) 
                return(0);

	/* make board interrupt by executing a GETPCBB command */
	addr->pcsr0 = PCSR0_INTE;
	addr->pcsr2 = 0;
	addr->pcsr3 = 0;
	addr->pcsr0 = PCSR0_INTE|CMD_GETPCBB;
	DELAY(100000);
#endif		/* not_on_pdp */
	return(1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
deattach(ui)
struct	uba_device *ui;
{
	register struct de_softc *ds = &de_softc[ui->ui_unit];
	register struct ifnet *ifp = &ds->ds_if;
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	int i;
	u_short csr1;

	/*
	 * Is it a DEUNA or a DELULA? Save the device id.
	 */
	csr1 = addr->pcsr1;
	ds->ds_devid = (csr1 & PCSR1_DEVID) >> 4;

#ifdef DE_DEBUG
	if (dedebug >= 1)
		printf("de%d: Device Type: %s\n", ui->ui_unit,
			(ds->ds_devid == DEUNA) ? "DEUNA" : "DELUA");
#endif		/* DE_DEBUG */
	/*
	 * Board Status Check
	 */
	if (csr1 & 0xff80) {
		if (ds->ds_devid == DEUNA)
			printf("de%d: hardware error, pcsr1=%b\n", 
				ui->ui_unit, csr1, PCSR1_BITS);
		else
			printf("de%d: hardware error, pcsr1=%b\n", 
				ui->ui_unit, csr1, PCSR1_BITS_DELUA);
	}

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "de";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags |= IFF_BROADCAST;
#ifdef	DE_DEBUG
	ifp->if_flags |= IFF_DEBUG;
	printf("de%d: DEBUG enabled level=%d\n",ifp->if_unit,dedebug);
#endif		/* DE_DEBUG */

#ifdef DE_DO_MULTI
	/*
	 * Fill the multicast address table with unused entries (broadcast
	 * address) so that we can always give the full table to the device
	 * and we don't have to worry about gaps.
	 */
	for (i=0; i < NMULTI; i++)
		bcopy(unused_multi,(u_char *)&ds->ds_multicast[i],MULTISIZE);

#endif		/* DE_DO_MULTI */
	/*
	 * Reset the board and map the pcbb buffer onto the Unibus.
	 */
	addr->pcsr0 = PCSR0_RSET;
	(void) dewait(ui, "board reset", YES, NO);

	ds->ds_ubaddr = uballoc(INCORE_BASE(ds), INCORE_SIZE);
	addr->pcsr2 = ds->ds_ubaddr & 0xffff;
	addr->pcsr3 = (ds->ds_ubaddr >> 16) & 0x3;
	addr->pclow = CMD_GETPCBB;
	(void) dewait(ui, "get pcbb", YES, NO);

	ds->ds_pcbb.pcbb0 = FC_RDPHYAD;
	addr->pclow = CMD_GETCMD;
	(void) dewait(ui, "read addr", YES, NO);

	bcopy((caddr_t)&ds->ds_pcbb.pcbb2,(caddr_t)ds->ds_addr,
	    sizeof (ds->ds_addr));
#ifdef	DE_DEBUG
	if (dedebug >= 1)
		printf("de%d: hardware address %s\n",
			ui->ui_unit, ether_sprintf(ds->ds_addr));
#endif		/* DE_DEBUG */
	ifp->if_init = deinit;
	ifp->if_output = deoutput;
	ifp->if_ioctl = deioctl;
	ifp->if_reset = 0;
	ds->ds_deuba.difu_flags = UBA_CANTWAIT;

	if_attach(ifp);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
deinit(unit)
	int unit;
{
	register struct de_softc *ds = &de_softc[unit];
	register struct uba_device *ui = deinfo[unit];
	register struct dedevice *addr;
	register struct ifrw *ifrw;
	struct ifnet *ifp = &ds->ds_if;
	int s;
	struct de_ring *rp;
	ubadr_t incaddr;

	/* not yet, if address still unknown */
	/* DECnet must set this somewhere to make device happy */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;

	if (ds->ds_flags & DSF_RUNNING)
		return;

	if ((ifp->if_flags & IFF_RUNNING) == 0) {	/* First time */
		/*
		 * Only allocate the resources ONCE.
		 * ~IFF_RUNNING && ~DSF_RUNNING
		 */
		if (de_ubainit(&ds->ds_deuba, ui->ui_ubanum, 
		    sizeof (struct ether_header), 
		    (int) btoc(ETHERMTU)) == 0) { 
			printf("de%d: can't initialize\n", unit);
			ds->ds_if.if_flags &= ~IFF_UP;
			return;
		}
	}
	addr = (struct dedevice *)ui->ui_addr;

	/*
	 * Set up the PCBB - this is done in deattach() also, but
	 * has to be redone here in case the board is reset (PCSR0_RSET)
	 * and this routine is called.	Note that ds->ds_ubaddr is set
	 * in deattach() and all we do here is tell the
	 * DEUNA/DELUA where it can find its PCBB.
	 */
	addr->pcsr2 = ds->ds_ubaddr & 0xffff;
	addr->pcsr3 = (ds->ds_ubaddr >> 16) & 0x3;
	addr->pclow = CMD_GETPCBB;
	(void) dewait(ui, "get pcbb", YES, NO);

	/*
	 * Set the transmit and receive ring header addresses
	 */
	incaddr = ds->ds_ubaddr + UDBBUF_OFFSET;
	ds->ds_pcbb.pcbb0 = FC_WTRING;
	ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
	ds->ds_pcbb.pcbb4 = (incaddr >> 16) & 0x3;

	incaddr = ds->ds_ubaddr + XRENT_OFFSET;
	ds->ds_udbbuf.b_tdrbl = incaddr & 0xffff;
	ds->ds_udbbuf.b_tdrbh = (incaddr >> 16) & 0x3;
	ds->ds_udbbuf.b_telen = sizeof (struct de_ring) / sizeof (short);
	ds->ds_udbbuf.b_trlen = NXMT;
	incaddr = ds->ds_ubaddr + RRENT_OFFSET;
	ds->ds_udbbuf.b_rdrbl = incaddr & 0xffff;
	ds->ds_udbbuf.b_rdrbh = (incaddr >> 16) & 0x3;
	ds->ds_udbbuf.b_relen = sizeof (struct de_ring) / sizeof (short);
	ds->ds_udbbuf.b_rrlen = NRCV;

	addr->pclow = CMD_GETCMD;
	(void) dewait(ui, "wtring", NO, NO);

	/*
	 * Initialize the board's mode
	 */
	ds->ds_pcbb.pcbb0 = FC_WTMODE;
	/*
	 * Let hardware do padding & set MTCH bit on broadcast
 	 */
	ds->ds_pcbb.pcbb2 = MOD_TPAD|MOD_HDX;
	addr->pclow = CMD_GETCMD;
	(void) dewait(ui, "wtmode", NO, NO);

	/*
	 * Set up the receive and transmit ring entries
	 */
	ifrw = &ds->ds_deuba.difu_w[0];
	for (rp = &ds->ds_xrent[0]; rp < &ds->ds_xrent[NXMT]; rp++) {
		rp->r_segbl = ifrw->ifrw_info & 0xffff;
		rp->r_segbh = (ifrw->ifrw_info >> 16) & 0x3;
		rp->r_flags = 0;
		ifrw++;
	}
	ifrw = &ds->ds_deuba.difu_r[0];
	for (rp = &ds->ds_rrent[0]; rp < &ds->ds_rrent[NRCV]; rp++) {
		rp->r_slen = sizeof (struct de_buf);
		rp->r_segbl = ifrw->ifrw_info & 0xffff;
		rp->r_segbh = (ifrw->ifrw_info >> 16) & 0x3;
		rp->r_flags = RFLG_OWN;		/* hang receive */
		ifrw++;
	}

	/*
	 * Start up the board (rah rah)
	 */
	s = splimp();
	ds->ds_nxmit = ds->ds_rindex = ds->ds_xindex = ds->ds_xfree = 0;
	ds->ds_if.if_flags |= IFF_RUNNING;
	destart(unit);				/* queue output packets */
	addr->pclow = PCSR0_INTE;		/* avoid interlock */
	ds->ds_flags |= DSF_RUNNING;
#ifdef	NS
	if (ds->ds_flags & DSF_SETADDR)
		de_setaddr(ds->ds_addr, unit);
#endif		/* NS */
	addr->pclow = CMD_START | PCSR0_INTE;
	splx(s);
#ifdef	DE_DO_BCTRS
	cpfromkern(&time, &atv, sizeof(struct timeval));
	ds->ds_ztime = atv.tv_sec;
#endif		/* DE_DO_BCTRS */
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 */
static
destart(unit)
	int unit;
{
        int len;
	register struct uba_device *ui = deinfo[unit];
	struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	register struct de_softc *ds = &de_softc[unit];
	register struct de_ring *rp;
	struct mbuf *m;
	register int nxmit;

	if (ds->ds_flags & DSF_LOCK)
		return;

	for (nxmit = ds->ds_nxmit; nxmit < NXMT; nxmit++) {
		IF_DEQUEUE(&ds->ds_if.if_snd, m);
		if (m == 0)
			break;
		rp = &ds->ds_xrent[ds->ds_xfree];
		if (rp->r_flags & XFLG_OWN)
			panic("deuna xmit in progress");
		len = deput(&ds->ds_deuba, ds->ds_xfree, m);
		rp->r_slen = len;
		rp->r_tdrerr = 0;
		rp->r_flags = XFLG_STP|XFLG_ENP|XFLG_OWN;

		ds->ds_xfree++;
		if (ds->ds_xfree == NXMT)
			ds->ds_xfree = 0;
	}
	if (ds->ds_nxmit != nxmit) {
		ds->ds_nxmit = nxmit;
		if (ds->ds_flags & DSF_RUNNING)
			addr->pclow = PCSR0_INTE|CMD_PDMD;
	} else if (ds->ds_nxmit == NXMT) {
		/*
		 * poke device if we have something to send and 
		 * transmit ring is full. 
		 */
#ifdef	DE_DEBUG
		if (dedebug >= 1) { 
			rp = &ds->ds_xrent[0];
			printf("de%d: did not transmit: %d, %d, %d, flag0=%x, flag1=%x\n",
				unit, ds->ds_xindex, ds->ds_nxmit, ds->ds_xfree,
				rp->r_flags, (rp+1)->r_flags);
		}
#endif		/* DE_DEBUG */
		if (ds->ds_flags & DSF_RUNNING)
			addr->pclow = PCSR0_INTE|CMD_PDMD;
	}
}

/*
 * Command done interrupt.
 */
deintr(unit)
	int unit;
{
	struct uba_device *ui = deinfo[unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	register struct de_softc *ds = &de_softc[unit];
	register struct de_ring *rp;
	register struct ifrw *ifrw;
	short csr0;

	/* save flags right away - clear out interrupt bits */
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;

	if (csr0 & PCSR0_FATI) {
		if (ds->ds_devid == DEUNA)
			printf(
		          "de%d: unsolicited state change, csr0=%b, csr1=%b\n",
			   unit, csr0, PCSR0_BITS,
			   addr->pcsr1, PCSR1_BITS);
		else
			printf(
		          "de%d: unsolicited state change, csr0=%b, csr1=%b\n",
			   unit, csr0, PCSR0_BITS,
			   addr->pcsr1, PCSR1_BITS_DELUA);
	}

	if (csr0 & PCSR0_SERI) {
		if (ds->ds_devid == DEUNA)
			printf("de%d: status error, csr0=%b, csr1=%b\n",
			   unit, csr0, PCSR0_BITS,
			   addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: status error, csr0=%b, csr1=%b\n",
			   unit, csr0, PCSR0_BITS,
			   addr->pcsr1, PCSR1_BITS_DELUA);
	}

	ds->ds_flags |= DSF_LOCK;

	/*
	 * if receive, put receive buffer on mbuf
	 * and hang the request again
	 */
	rp = &ds->ds_rrent[ds->ds_rindex];
	if ((rp->r_flags & RFLG_OWN) == 0)
		derecv(unit);

	/*
	 * Poll transmit ring and check status.
	 * Be careful about loopback requests.
	 * Then free buffer space and check for
	 * more transmit requests.
	 */
	for ( ; ds->ds_nxmit > 0; ds->ds_nxmit--) {
		rp = &ds->ds_xrent[ds->ds_xindex];
		if (rp->r_flags & XFLG_OWN)
			break;
		ds->ds_if.if_opackets++;
		ifrw = &ds->ds_deuba.difu_w[ds->ds_xindex];
		/* check for unusual conditions */
		if (rp->r_flags & (XFLG_ERRS|XFLG_MTCH|XFLG_ONE|XFLG_MORE)) {
			if (rp->r_flags & XFLG_ERRS) {
				/* output error */
				ds->ds_if.if_oerrors++;
#ifdef	DE_DEBUG
				if (dedebug >= 1)
				    printf("de%d: oerror, flags=%b tdrerr=%b (len=%d)\n",
					unit, rp->r_flags, XFLG_BITS,
					rp->r_tdrerr, XERR_BITS,
					rp->r_slen);
#endif		/* DE_DEBUG */
			} else  {
				if (rp->r_flags & XFLG_ONE) {
					/* one collision */
					ds->ds_if.if_collisions++;
				} else if (rp->r_flags & XFLG_MORE) {
					/* more than one collision */
					ds->ds_if.if_collisions += 2; /*guess*/
				}
				if ((rp->r_flags & XFLG_MTCH) &&
					!(ds->ds_if.if_flags & IFF_LOOPBACK)) {
					/* received our own packet */
					ds->ds_if.if_ipackets++;
#ifdef DE_DEBUG
					if (dedebug >= 3)
						printf("de%d: loopback packet\n",
							unit);
#endif		/* DE_DEBUG */
					deread(ds, ifrw,
					    rp->r_slen - sizeof (struct ether_header));
				}
			}
		}
		/* check if next transmit buffer also finished */
		ds->ds_xindex++;
		if (ds->ds_xindex >= NXMT)
			ds->ds_xindex = 0;
	}
	ds->ds_flags &= ~DSF_LOCK;
	destart(unit);

	if (csr0 & PCSR0_RCBI) {
		ds->ds_if.if_ierrors++;
#ifdef DE_DEBUG
		if (dedebug >= 1)
			printf("de%d: buffer unavailable\n", ui->ui_unit);
#endif		/* DE_DEBUG */
		addr->pclow = PCSR0_INTE|CMD_PDMD;
	}
}

/*
 * Ethernet interface receiver interface.
 * If input error just drop packet.
 * Otherwise purge input buffered data path and examine 
 * packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
static
derecv(unit)
	int unit;
{
	register struct de_softc *ds = &de_softc[unit];
	register struct de_ring *rp;
	register int len;
	struct ether_header *eh;

	rp = &ds->ds_rrent[ds->ds_rindex];
	while ((rp->r_flags & RFLG_OWN) == 0) {
		ds->ds_if.if_ipackets++;
		len = (rp->r_lenerr&RERR_MLEN) - sizeof (struct ether_header)
			- CRC_LEN;	/* don't forget checksum! */
                if( ! (ds->ds_if.if_flags & IFF_LOOPBACK) ) {
		/* check for errors */
		    if ((rp->r_flags & (RFLG_ERRS|RFLG_FRAM|RFLG_OFLO|RFLG_CRC)) ||
			(rp->r_flags&(RFLG_STP|RFLG_ENP)) != (RFLG_STP|RFLG_ENP) ||
			(rp->r_lenerr & (RERR_BUFL|RERR_UBTO|RERR_NCHN)) ||
		        len < ETHERMIN || len > ETHERMTU) {
		  	    ds->ds_if.if_ierrors++;
#ifdef	DE_DEBUG
			    if (dedebug >= 1)
			     printf("de%d: ierror, flags=%b lenerr=%b (len=%d)\n",
				unit, rp->r_flags, RFLG_BITS,
				rp->r_lenerr, RERR_BITS, len);
#endif		/* DE_DEBUG */
		    } else
			deread(ds, &ds->ds_deuba.difu_r[ds->ds_rindex], len);
                } else {
			int	ret;
			segm	sav5;

			saveseg5(sav5);
			mapseg5(ds->ds_deuba.difu_r[ds->ds_rindex].ifrw_click, MAPBUFDESC);
			eh = (struct ether_header *) SEG5;
                        ret = bcmp(eh->ether_dhost, ds->ds_addr, 6);
			restorseg5(sav5);
			if (ret == NULL)
                                deread(ds, &ds->ds_deuba.difu_r[ds->ds_rindex], len);
                }

		/* hang the receive buffer again */
		rp->r_lenerr = 0;
		rp->r_flags = RFLG_OWN;

		/* check next receive buffer */
		ds->ds_rindex++;
		if (ds->ds_rindex >= NRCV)
			ds->ds_rindex = 0;
		rp = &ds->ds_rrent[ds->ds_rindex];
	}
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
static
deread(ds, ifrw, len)
	register struct de_softc *ds;
	struct ifrw *ifrw;
	int len;
{
	struct ether_header *eh;
    	struct mbuf *m;
	struct protosw *pr;
	int off, resid, s;
	struct ifqueue *inq;
	segm	sav5;
	int	type;

	/*
	 * Deal with trailer protocol: if type is trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	saveseg5(sav5);
	mapseg5(ifrw->ifrw_click, MAPBUFDESC);
	eh = (struct ether_header *) SEG5;

	type = eh->ether_type = ntohs((u_short)eh->ether_type);

#define	dedataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (type >= ETHERTYPE_TRAIL &&
		type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU) {
			restorseg5(sav5);
			return;		/* sanity */
		}
		type = ntohs(*dedataaddr(eh, off, u_short *));
		resid = ntohs(*(dedataaddr(eh, off+2, u_short *)));

		if (off + resid > len) {
			restorseg5(sav5);
			return;		/* sanity */
		}
		len = off + resid;
	} else
		off = 0;

	if (len == 0) {
		restorseg5(sav5);
		return;
	}

	restorseg5(sav5);

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; deget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */

	m = deget(&ds->ds_deuba, ifrw, len, off, &ds->ds_if);

	if (m == 0)
		return;
	if (off) {
		struct ifnet *ifp;

		ifp = *(mtod(m, struct ifnet **));
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
		*(mtod(m, struct ifnet **)) = ifp;
	}

	switch (type) {

#ifdef INET
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		arpinput(&ds->ds_ac, m);
		return;
#endif		/* INET */
	default:
#ifdef not_on_pdp
		/*
		 * see if other protocol families defined
		 * and call protocol specific routines.
		 * If no other protocols defined then dump message.
		 */

		if (pr=iftype_to_proto(type))  {
			if ((m = (struct mbuf *)(*pr->pr_ifinput)(m, &ds->ds_if, &inq)) == 0)
				return;
		} else {
#endif		/* not_on_pdp */
#ifdef DE_DEBUG
		if(dedebug >= 1)
			printf("de%d: Unknow Packet Type 0%o(%d)\n",
				ds->ds_if.if_unit, type, type);
#endif		/* DE_DEBUG */
#ifdef	DE_DO_BCTRS
			if (ds->ds_unrecog != 0xffff)
				ds->ds_unrecog++;
#endif		/* DE_DO_BCTRS */
			m_freem(m);
			return;
#ifdef not_on_pdp
		}
#endif		/* not_on_pdp */
	}

	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		splx(s);
#ifdef DE_DEBUG
		if (dedebug >= 1)
			printf("de%d: Packet Dropped(deread)\n",
				ds->ds_if.if_unit);
#endif		/* DE_DEBUG */
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
	splx(s);
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
deoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int type, s, error;
	u_char edst[6];
	struct in_addr idst;
	struct protosw *pr;
	register struct de_softc *ds = &de_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct ether_header *eh;
	register int off;
	int usetrailers;

	if((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		error = ENETDOWN;
		goto bad;
	}

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		idst = ((struct sockaddr_in *)dst)->sin_addr;
		if (!arpresolve(&ds->ds_ac, m, &idst, edst, &usetrailers))
			return (0);	/* if not yet resolved */

		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;

		/* need per host negotiation */
		if (usetrailers && off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ETHERTYPE_TRAIL + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)ETHERTYPE_IP);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottrailertype;
		}
		type = ETHERTYPE_IP;
		off = 0;
		goto gottype;
#endif		/* INET */
#ifdef	NS
	case AF_NS:
		type = ETHERTYPE_NS;
		bcopy((caddr_t)&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
		(caddr_t)edst, sizeof (edst));
		off = 0;
		goto gottype;
#endif		/* NS */

	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
		bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		type = eh->ether_type;
		goto gottype;

	default:
#ifdef	DE_INT_LOOPBACK
		/*
		 * If we are in loopback mode check the length and
		 * device type.  DELUA can only loopback frames of 36 bytes
		 * or less including crc.
		 */
		if ((ifp->if_flags & IFF_LOOPBACK) &&
		    (m->m_len > DELUA_LOOP_LEN) && (ds->ds_devid == DELUA))
			return(EINVAL);
#endif		/* DE_INT_LOOPBACK */
#ifdef not_on_pdp
		/*
		 * try to find other address families and call protocol
		 * specific output routine.
		 */
		if (pr=iffamily_to_proto(dst->sa_family)) {
			(*pr->pr_ifoutput)(ifp, m0, dst, &type, (char *)edst);
			goto gottype;
		} else {
#endif		/* not_on_pdp */
			printf("de%d: can't handle af%d", ifp->if_unit, dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
#ifdef not_on_pdp
		}
#endif		/* not_on_pdp */
	}

gottrailertype:
	/*
	 * Packet to be sent as trailer: move first packet
	 * (control information) to end of chain.
	 */
	while (m->m_next)
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
	/* DEUNA fills in source address */
	bcopy((caddr_t)ds->ds_addr,
	      (caddr_t)eh->ether_shost, sizeof(ds->ds_addr));

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
#ifdef DE_DEBUG
		if (dedebug >= 1)
			printf("de%d: Packet Dropped(deoutput)\n",
				ifp->if_unit);
#endif		/* DE_DEBUG */
		m_freem(m);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	destart(ifp->if_unit);
	splx(s);
	return (0);

bad:
	m_freem(m0);
	return (error);
}

/*
 * Routines supporting UNIBUS network interfaces.
 */
/*
 * Init UNIBUS for interface.  We map the i/o area
 * onto the UNIBUS. We then split up the i/o area among
 * all the receive and transmit buffers.
 */

static
de_ubainit(ifu, uban, hlen, nmr)
	register struct deuba *ifu;
	int uban, hlen, nmr;
{
	register caddr_t cp, dp;
	register struct ifrw *ifrw;
	int i, ncl;

	/*
	 * If the ring already has core allocated, quit now.
	 */
	if (ifu->difu_r[0].ifrw_click)
		return(1);

	nmr = ctob(nmr);
	nmr += hlen;
	for (i = 0; i < NRCV; i++) {
		ifu->difu_r[i].ifrw_click = m_ioget(nmr);
		if (ifu->difu_r[i].ifrw_click == 0) {
			ifu->difu_r[0].ifrw_click = 0;
			if (i)
				printf("de: lost some space\n");
			return(0);
		}
	}
	for (i = 0; i < NXMT; i++) {
		ifu->difu_w[i].ifrw_click = m_ioget(nmr);
		if (ifu->difu_w[i].ifrw_click == 0) {
			ifu->difu_w[0].ifrw_click = 0;
			ifu->difu_r[0].ifrw_click = 0;
			if (i)
				printf("de: lost some space\n");
			return(0);
		}
	}
	for (i = 0; i < NRCV; i++)
		ifu->difu_r[i].ifrw_info = 
			ubmalloc(ifu->difu_r[i].ifrw_click);
	for (i = 0; i < NXMT; i++)
		ifu->difu_w[i].ifrw_info = 
			ubmalloc(ifu->difu_w[i].ifrw_click);
	ifu->ifu_hlen = hlen;
	return (1);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.
 */
static struct mbuf *
deget(ifu, ifrw, totlen, off0, ifp)
	register struct deuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = (caddr_t) ifu->ifu_hlen;
	u_int click;

	top = 0;
	mp = &top;
	click = ifrw->ifrw_click;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = (caddr_t)(ifu->ifu_hlen + off);
		} else
			len = totlen;
		m->m_off = MMINOFF;
		if (ifp) {
			/*
			 * Leave Room for the ifp
			 */
			m->m_len = MIN(MLEN - sizeof(ifp), len);
			m->m_off += sizeof(ifp);
		} else
			m->m_len = MIN(MLEN, len);

		mbcopyin(click, cp, mtod(m, char *), (u_int) m->m_len);
		cp += m->m_len;
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
			 * Prepend interface pointer to first mbuf
			 */
			m->m_len += sizeof(ifp);
			m->m_off -= sizeof(ifp);
			*(mtod(m, struct ifnet **)) = ifp;
			ifp = (struct ifnet *) 0;
		}
	}
	return (top);
bad:
	m_freem(top);
	return (0);
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header which is copied to be in the mapped, aligned
 * i/o space.
 */
static
deput(ifu, n, m)
	struct deuba *ifu;
	int n;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register u_short off, click;

	click = ifu->difu_w[n].ifrw_click;
	off = 0;
	while (m) {
		mbcopyout(mtod(m, char *), click, off, (u_int) m->m_len);
		off += m->m_len;
		MFREE(m, mp);
		m = mp;
	}

	return (off);
}

/*
 * Process an ioctl request.
 */
deioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct de_softc *ds = &de_softc[ifp->if_unit];
	register struct uba_device *ui = deinfo[ifp->if_unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	struct protosw *pr;
	struct sockaddr *sa;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifdevea *ifd = (struct ifdevea *)data;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	int s = splimp(), error = 0;

	switch (cmd) {

#ifdef DE_INT_LOOPBACK
        case SIOCENABLBACK:
                printf("de%d: internal loopback enable requested\n", ifp->if_unit);
                if ( (error = deloopback(ifp, ds, addr, YES)) != NULL )
                        break;
                ifp->if_flags |= IFF_LOOPBACK;
                break;
 
        case SIOCDISABLBACK:
                printf("de%d: internal loopback disable requested\n", ifp->if_unit);
                if ( (error = deloopback(ifp, ds, addr, NO)) != NULL )
                        break;
                ifp->if_flags &= ~IFF_LOOPBACK;
                deinit(ifp->if_unit);
                break;
#endif		/* DE_INT_LOOPBACK */
 
#ifdef	DE_DO_PHYSADDR
        case SIOCRPHYSADDR: 
                /*
                 * read default hardware address.
                 */
                ds->ds_pcbb.pcbb0 = FC_RDDEFAULT;
                addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		if (dewait(ui, "read default hardware address", NO, YES)) {
                        error = EIO;
                        break;
                }
                /*
                 * copy current physical address and default hardware address
                 * for requestor.
                 */
                bcopy(&ds->ds_pcbb.pcbb2, ifd->default_pa, 6);
                bcopy(ds->ds_addr, ifd->current_pa, 6);
                break;
 

	case SIOCSPHYSADDR: 
		/* Set the DNA address as the de's physical address */
		ds->ds_pcbb.pcbb0 = FC_WTPHYAD;
		bcopy (ifr->ifr_addr.sa_data, &ds->ds_pcbb.pcbb2, 6);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		(void) dewait(ui, "write physical address", NO, YES);
		bcopy((caddr_t)&ds->ds_pcbb.pcbb2,(caddr_t)ds->ds_addr,
	    		sizeof (ds->ds_addr));
		deinit(ifp->if_unit);
		break;
#endif		/* DE_DO_PHYSADDR */

#ifdef DE_DO_MULTI
	case SIOCDELMULTI: 
	case SIOCADDMULTI: 
		{
		int i,j = -1,incaddr = ds->ds_ubaddr + MULTI_OFFSET;

		if (cmd==SIOCDELMULTI) {
		   for (i = 0; i < NMULTI; i++)
		       if (bcmp(&ds->ds_multicast[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    if (--ds->ds_muse[i] == 0)
				bcopy(unused_multi,&ds->ds_multicast[i],MULTISIZE);
		       }
		} else {
		    for (i = 0; i < NMULTI; i++) {
			if (bcmp(&ds->ds_multicast[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
			    ds->ds_muse[i]++;
			    goto done;
			}
		        if (bcmp(&ds->ds_multicast[i],unused_multi,MULTISIZE) == 0)
			    j = i;
		    }
		    if (j == -1) {
			printf("de%d: mtmulti failed, multicast list full: %d\n",
				ui->ui_unit, NMULTI);
			error = ENOBUFS;
			goto done;
		    }
		    bcopy(ifr->ifr_addr.sa_data, &ds->ds_multicast[j], MULTISIZE);
		    ds->ds_muse[j]++;
		}

		ds->ds_pcbb.pcbb0 = FC_WTMULTI;
		ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
		ds->ds_pcbb.pcbb4 = (NMULTI << 8) | ((incaddr >> 16) & 03);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		(void) dewait(ui, "set multicast address", NO, YES);
		break;

		}

#endif		/* DE_DO_MULTI */

#ifdef	DE_DO_BCTRS
	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		{
		register struct ctrreq *ctr = (struct ctrreq *)data;
		ubadr_t incaddr;

		ds->ds_pcbb.pcbb0 = cmd == SIOCRDCTRS ? FC_RDCNTS : FC_RCCNTS;
		incaddr = ds->ds_ubaddr + COUNTER_OFFSET;
		ds->ds_pcbb.pcbb2 = incaddr & 0xffff;
		ds->ds_pcbb.pcbb4 = (incaddr >> 16) & 0x3;
		ds->ds_pcbb.pcbb6 = sizeof(struct de_counters) / sizeof (short);
		addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
		if (dewait(ui, "read counts", NO, YES)) {
			error = ENOBUFS;
			break;
		}
		bzero(&ctr->ctr_ether, sizeof(struct estat));
		ctr->ctr_type = CTR_ETHER;
		cpfromkern(&time, &atv, sizeof(struct timeval));
		ctr->ctr_ether.est_seconds = (atv.tv_sec - ds->ds_ztime) > 0xfffe ? 0xffff : (atv.tv_sec - ds->ds_ztime);
		ctr->ctr_ether.est_byrcvd = *(long *)ds->ds_counters.c_brcvd;
		ctr->ctr_ether.est_bysent = *(long *)ds->ds_counters.c_bsent;
		ctr->ctr_ether.est_mbyrcvd = *(long *)ds->ds_counters.c_mbrcvd;
		ctr->ctr_ether.est_blrcvd = *(long *)ds->ds_counters.c_prcvd;
		ctr->ctr_ether.est_blsent = *(long *)ds->ds_counters.c_psent;
		ctr->ctr_ether.est_mblrcvd = *(long *)ds->ds_counters.c_mprcvd;
		ctr->ctr_ether.est_deferred = *(long *)ds->ds_counters.c_defer;
		ctr->ctr_ether.est_single = *(long *)ds->ds_counters.c_single;
		ctr->ctr_ether.est_multiple = *(long *)ds->ds_counters.c_multiple;
		ctr->ctr_ether.est_sf = ds->ds_counters.c_snderr;
		ctr->ctr_ether.est_sfbm = ds->ds_counters.c_sbm & 0xff;
		ctr->ctr_ether.est_collis = ds->ds_counters.c_collis;
		ctr->ctr_ether.est_rf = ds->ds_counters.c_rcverr;
		ctr->ctr_ether.est_rfbm = ds->ds_counters.c_rbm & 0xff;
		ctr->ctr_ether.est_unrecog = ds->ds_unrecog;
		ctr->ctr_ether.est_sysbuf = ds->ds_counters.c_ibuferr;
		ctr->ctr_ether.est_userbuf = ds->ds_counters.c_lbuferr;
		if (cmd == SIOCRDZCTRS) {
			cpfromkern(&time, &atv, sizeof(struct timeval));
			ds->ds_ztime = atv.tv_sec;
			ds->ds_unrecog = 0;
		}
		break;
		}
#endif		/* DE_DO_BCTRS */

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		deinit(ifp->if_unit);
		switch (ifa->ifa_addr.sa_family) {
#ifdef	INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif		/* INET */
#ifdef	NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina)) {
				ina->x_host = * (union ns_host *)
					(de_softc[ifp->if_unit].ds_addr);
			} else {
				de_setaddr(ina->x_host.c_host,ifp->if_unit);
			}
			break;
		    }
#endif		/* NS */
		default:
#ifdef not_on_pdp
			if (pr=iffamily_to_proto(ifa->ifa_addr.sa_family)) {
				error = (*pr->pr_ifioctl)(ifp, cmd, data);
			}
#endif		/* not_on_pdp */
#ifdef DE_DEBUG
			if (dedebug >= 1)
				printf("de%d: SIOCSIFADDR Unknown address family\n", ifp->if_unit);
#endif		/* DE_DEBUG */
			error = EAFNOSUPPORT;
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    ds->ds_flags & DSF_RUNNING) {
			((struct dedevice *)
			   (deinfo[ifp->if_unit]->ui_addr))->pclow = PCSR0_RSET;
			(void) dewait(deinfo[ifp->if_unit], "board reset(deioctl)", YES, NO);
			ds->ds_flags &= ~(DSF_LOCK | DSF_RUNNING);
#ifdef	DE_DEBUG
			if (dedebug >= 1)
				printf("de%d: reset and marked down\n",ifp->if_unit);
#endif		/* DE_DEBUG */
		} else if (ifp->if_flags & IFF_UP &&
		    (ds->ds_flags & DSF_RUNNING) == 0) {
			deinit(ifp->if_unit);
#ifdef	DE_DEBUG
			if (dedebug >= 1)
				printf("de%d: reinitialized and marked up\n",ifp->if_unit);
#endif		/* DE_DEBUG */
		}
#ifdef	DE_DEBUG
		if ((ifp->if_flags & IFF_DEBUG) == 0) {
			if (dedebug != 0) {
				dedebug = 0;
				printf("de%d: DEBUG disabled\n",ifp->if_unit);
			}
		} else {
			if (dedebug == 0) {
				dedebug = DE_DEBUG_LEVEL;
				printf("de%d: DEBUG enabled level=%d\n",ifp->if_unit,dedebug);
			}
		}
#endif		/* DE_DEBUG */
		break;

	default:
#ifdef	not_on_pdp
		if (pr=iffamily_to_proto(ifa->ifa_addr.sa_family))
			error = (*pr->pr_ifioctl)(ifp, cmd, data);
		else
#endif		/* not_on_pdp */
		error = EINVAL;
	}
done:	splx(s);
	return (error);
}

#ifdef DE_INT_LOOPBACK
/*
 * enable or disable internal loopback
 */
static
deloopback(ifp, ds, addr, lb_ctl )
register struct ifnet *ifp;
register struct de_softc *ds;
register struct dedevice *addr;
u_char lb_ctl;
{
	register struct uba_device *ui = deinfo[ifp->if_unit];
        /*
         * read current mode register.
         */
        ds->ds_pcbb.pcbb0 = FC_RDMODE;
        addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
	if (dewait(ui, "read mode register", NO, YES))
                return(EIO);

        /*
         * set or clear the loopback bit as a function of lb_ctl and
         * return mode register to driver.
         */
        if ( lb_ctl == YES ) {
                ds->ds_pcbb.pcbb2 |= MOD_LOOP;
		if (ds->ds_devid == DELUA)
			ds->ds_pcbb.pcbb2 |= MOD_INTL;
		else
                	ds->ds_pcbb.pcbb2 &= ~MOD_HDX;
        } else {
                ds->ds_pcbb.pcbb2 &= ~MOD_LOOP;
		if (ds->ds_devid == DELUA)
			ds->ds_pcbb.pcbb2 &= ~MOD_INTL;
		else
			ds->ds_pcbb.pcbb2 |= MOD_HDX;
        }
        ds->ds_pcbb.pcbb0 = FC_WTMODE;
        addr->pclow = CMD_GETCMD|((ds->ds_flags & DSF_RUNNING) ? PCSR0_INTE : 0);
	if(dewait(ui, "write mode register", NO, YES))
                return(EIO);

        return(NULL);
}
#endif		/* DE_INT_LOOPBACK */

dewait(ui, fn, no_port, only_dni)
register struct uba_device *ui;
char *fn;
int no_port;
int only_dni;
{
	register struct de_softc *ds = &de_softc[ui->ui_unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;
	register csr0;
	
	if (only_dni)
	    while ((addr->pcsr0 & PCSR0_DNI) == 0)
		    ;
	else
	    while ((addr->pcsr0 & PCSR0_INTR) == 0)
		    ;
	csr0 = addr->pcsr0;
	addr->pchigh = csr0 >> 8;
	if ((csr0 & PCSR0_PCEI) || (no_port && (csr0 & PCSR0_DNI)==0)) {
		if (ds->ds_devid == DEUNA)
			printf("de%d: %s failed, csr0=%b, csr1=%b\n",
				ui->ui_unit, fn, csr0, PCSR0_BITS,
				addr->pcsr1, PCSR1_BITS);
		else
			printf("de%d: %s failed, csr0=%b, csr1=%b\n",
				ui->ui_unit, fn, csr0, PCSR0_BITS,
				addr->pcsr1, PCSR1_BITS_DELUA);
	}
	return(csr0 & PCSR0_PCEI);
}

#ifdef	NS
/*
 * Set the ethernet address for the unit, this sets the board's
 * hardware address to physaddr.  This is only used if you have
 * NS defined.
 */
de_setaddr(physaddr, unit)
u_char *physaddr;
int unit;
{
	register struct de_softc *ds = &de_softc[unit];
	struct uba_device *ui = deinfo[unit];
	register struct dedevice *addr = (struct dedevice *)ui->ui_addr;

	if (! (ds->ds_flags & DSF_RUNNING))
		return;
	bcopy(physaddr, &ds->ds_pcbb.pcbb2, 6);
	ds->ds_pcbb.pcbb0 = FC_WTPHYAD;
	addr->pclow = PCSR0_INTE | CMD_GETCMD;
	if (dewait(ui, "address changed", NO, NO) == 0) {
		ds->ds_flags |= DSF_SETADDR;
		bcopy(physaddr, ds->ds_addr, 6);
	}
}
#endif		/* NS */
#endif		/* NDE > 0 */
