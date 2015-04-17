/*
 * SCCSID: @(#)if_de.h	1.1	(2.11BSD GTE)	12/31/93
 */

/* Header files and definitons to support multiple DEUNAs */

#include "socket.h"
#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef INET
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"
#endif		/* INET */

#ifdef NS
#include "../netns/ns.h"
#include "../netns/ns_if.h"
#endif		/* NS */

#include "../pdpif/if_dereg.h"
#include "../pdpuba/ubavar.h"
#include "../pdpif/if_uba.h"

/*
 * These numbers are based on the amount of space that is allocated
 * int netinit() to miobase, for 4 + 6, miosize must be 16384 instead
 * of the 8192 allocated originally.  m_ioget() gets a click address
 * within the allocated region.  NOTE: the UMR handling has been fixed
 * in 2.11BSD to allocate only the number of UMRs required by the size
 * of the m_ioget I/O region - a UMR per buffer is NO LONGER THE CASE!
 * Note that the size of a buffer is:
 * 1500 (ETHERMTU) + sizeof(ether_header) + some rounding from btoc() =
 * 1536 bytes or 24 clicks.
 */
 
#define	NXMT	4	/* number of transmit buffers */
#define	NRCV	6	/* number of receive buffers (must be > 1) */

#ifdef	DE_DO_MULTI
/*
 * Multicast address list structure
 */
struct de_m_add {
	u_char	dm_char[6];
};
#define	MULTISIZE	sizeof(struct de_m_add)
#define	NMULTI		10	/* # of multicast addrs on the DEUNA */
#endif		/* DE_DO_MULTI */

/*
 * The deuba structures generalizes the ifuba structure
 * to an arbitrary number of receive and transmit buffers.
 */
struct	deuba {
	u_short	ifu_hlen;		/* local net header length */
	struct	ifrw difu_r[NRCV];	/* receive information */
	struct	ifrw difu_w[NXMT];	/* transmit information */
	short	difu_flags;
};
/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct	de_softc {
	struct	arpcom ds_ac;		/* Ethernet common part */
#define	ds_if	ds_ac.ac_if		/* network-visible interface */
#define	ds_addr	ds_ac.ac_enaddr		/* hardware Ethernet address */
	char	ds_flags;		/* Has the board be initialized? */
#define	DSF_LOCK	1
#define	DSF_RUNNING	2
#define	DSF_SETADDR	4
	char	ds_devid;		/* device id DEUNA=0, DELUA=1 */
	ubadr_t	ds_ubaddr;		/* map info for incore structs */
	struct	deuba ds_deuba;		/* unibus resource structure */
	/* the following structures are always mapped in */
	struct	de_pcbb ds_pcbb;	/* port control block */
	struct	de_ring ds_xrent[NXMT];	/* transmit ring entrys */
	struct	de_ring ds_rrent[NRCV];	/* receive ring entrys */
	struct	de_udbbuf ds_udbbuf;	/* UNIBUS data buffer */
#ifdef	DE_DO_MULTI
	struct	de_m_add ds_multicast[NMULTI]; /* multicast addr list */
#endif		/* DE_DO_MULTI */

#ifdef	DE_DO_BCTRS
	struct	de_counters ds_counters;/* counter block */
#endif		/* DE_DO_BCTRS */

	/* end mapped area */
#define	INCORE_BASE(p)	((char *)&(p)->ds_pcbb)
#define	RVAL_OFF(n)	((char *)&de_softc[0].n - INCORE_BASE(&de_softc[0]))
#define	LVAL_OFF(n)	((char *)de_softc[0].n - INCORE_BASE(&de_softc[0]))
#define	PCBB_OFFSET	RVAL_OFF(ds_pcbb)
#define	XRENT_OFFSET	LVAL_OFF(ds_xrent)
#define	RRENT_OFFSET	LVAL_OFF(ds_rrent)
#define	UDBBUF_OFFSET	RVAL_OFF(ds_udbbuf)

#ifdef	DE_DO_MULTI
#define MULTI_OFFSET	RVAL_OFF(ds_multicast[0])
#endif		/* DE_DO_MULTI */

#ifdef	DE_DO_BCTRS
#define COUNTER_OFFSET	RVAL_OFF(ds_counters)
#endif		/* DE_DO_BCTRS */

#define	INCORE_SIZE	RVAL_OFF(ds_xindex)
	u_char	ds_xindex;		/* UNA index into transmit chain */
	u_char	ds_rindex;		/* UNA index into receive chain */
	u_char	ds_xfree;		/* index for next transmit buffer */
	u_char	ds_nxmit;		/* # of transmits in progress */

#ifdef	DE_DO_MULTI
	u_char	ds_muse[NMULTI];	/* multicast address use */
#endif		/* DE_DO_MULTI */

#ifdef	DE_DO_BCTRS
	long	ds_ztime;		/* time counters were last zeroed */
	u_short	ds_unrecog;		/* unrecognized frame destination */
#endif		/* DE_DO_BCTRS */
};

/*
 * These are the Ultrix ioctl's that are specific to this driver
 */

#ifdef	DE_DO_PHYSADDR
#define SIOCSPHYSADDR	_IOWR(i,23, struct ifreq)	/* Set phys. ad.*/
#define SIOCRPHYSADDR	_IOWR(i,28, struct ifdevea)	/* Read phy. ad.*/
#endif		/* DE_DO_PHYSADDR */

#ifdef	DE_DO_MULTI
#define SIOCADDMULTI	_IOWR(i,24, struct ifreq)	/* Add m.c. ad. */
#define SIOCDELMULTI	_IOWR(i,25, struct ifreq)	/* Dele. m.c.ad.*/
#endif		/* DE_DO_MULTI */

#ifdef	DE_DO_BCTRS
#define SIOCRDCTRS	_IOWR(i,26, struct ctrreq)	/* Read if cntr.*/
#define SIOCRDZCTRS	_IOWR(i,27, struct ctrreq)	/* Read/0 if c. */
#endif		/* DE_DO_BCTRS */

#ifdef DE_INT_LOOPBACK
#define SIOCDISABLBACK	_IOW(i,34, struct ifreq)	/* Cl.in.ex.lpb.*/
#define SIOCENABLBACK	_IOW(i,33, struct ifreq)	/* Set in.ex.lb.*/
#endif		/* DE_INT_LOOPBACK */
