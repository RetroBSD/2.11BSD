/*
 * Xebec disk driver for bootstrap
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/seg.h>
#include <sys/xereg.h>
#include "../saio.h"

#define	XEADDR	((struct xedevice *)0177460)
#define	NXE	2

/*
 *	memory management registers 
 */

# define	UISA0 ((unsigned short *) 0177640)
# define	UDSA0 ((unsigned short *) 0177660)
# define	KISA0 ((unsigned short *) 0172340)
# define	KDSA0 ((unsigned short *) 0172360)

# define	PSW   ((unsigned short *) 0177776)
# define	KERNEL	00
# define	SUPER	01
# define	USER	03
# define	mode(p)	(((p) >> 14) & 03)

struct xeinit	xei;
struct xecommand xec;
u_short		xeiaddr[2];
u_short		xecaddr[2];
struct xep	xeparam[NXE] = {
	17,	306,	4,	306,	128,	7,	0,
	17,	1024,	3,	1024,	1024,	7,	0,
};

xestrategy(io, func)
register struct iob *io;
{
	register unsigned short i;
	register struct xedevice *xeaddr = XEADDR;
	daddr_t bn;
	int	dn;

	xeset(io->i_unit);
	map (&xec, xecaddr);
	bn = io->i_bn;
	dn = io->i_unit;
	/*
	 *	select disk block
	 */
	xec.xelblk = bn & 0377;
	i = bn >> 8;
	xec.xehblk = i & 0377;
	i >>= 8;
	xec.xeunit = (dn << 5) | (i & 0037);
	xec.xecount = io->i_cc >> 9;
	xec.xecntl = xeparam[dn].xe_seek;
	if (func == READ)
		xec.xeop = XEREAD;
	else
		xec.xeop = XEWRITE;
	xeaddr->xexcar = (xecaddr[1] >> 4) & 03;
	xeaddr->xexdar = (segflag >> 4) & 03;
	xeaddr->xecar = xecaddr[0];
	xeaddr->xedar = io->i_ma;
	/*
	 *	warning: i is being used as temporary
	 */
	i = ((xecaddr[1] & 017) << 8) | ((segflag & 017) << 2) | CSRGO;
	/*
	 *	run it
	 */
	xeaddr->xecsr = i;
	while (!((i = xeaddr->xecsr) & (CSRDONE | CSRERROR)))
		;
	if (i & CSRERROR) {
		printf("disk error: block=%D, er=%o\n",
		    bn, xeaddr->xecsr);
		return(-1);
	}
	return(io->i_cc);
}

map (virtual, real)
u_short	virtual;
u_short	real[2];
{
	u_short	nb, ts;
	u_short	psw;

	nb = (virtual >> 6) & 01777;
	psw = *(PSW);
	switch (mode (psw)) {
	case USER:
		ts = UDSA0[nb >> 7] + (nb & 0177);
		break;
	case KERNEL:
		ts = KDSA0[nb >> 7] + (nb & 0177);
		break;
	default:
		printf ("unknown memory management mode: 0%o\n", psw);
		exit ();
	}
	real[0] = ((ts << 6) + (virtual & 077));
	real[1] = (ts >> 10) & 077;
}

char	xeerr[4];
short xeeaddr[2];
int	ccsr;

xeset (unit)
{
	register struct xedevice *xe = XEADDR;
	unsigned		s;

	/*
	 *	init the drive
	 */
	map (&xec, xecaddr);
	map (&xei, xeiaddr);
	xec.xeop = XEINIT;
	xec.xeunit = 0;
	xec.xehblk = 0;
	xec.xelblk = 0;
	xec.xecount = 0;
	xei.xeihtrac = xeparam[unit].xe_ntrac >> 8;
	xei.xeiltrac = xeparam[unit].xe_ntrac & 0377;
	xei.xeinhead = xeparam[unit].xe_nhead;
	xei.xeihrwcur = xeparam[unit].xe_rwcur >> 8;
	xei.xeilrwcur = xeparam[unit].xe_rwcur & 0377;
	xei.xeihwpc = xeparam[unit].xe_wpc >> 8;
	xei.xeilwpc = xeparam[unit].xe_wpc & 0377;
	xei.xeiecc = 11;
	/*
	 *	initialize this unit
	 */
	xe->xecsr = CSRRESET;
	if ((s = xe->xecsr) & CSRERROR) {
		printf ("disk error: csr: 0%o, ccsr: 0%o\n", s, xe->xeccsr);
		return;
	}
	xe->xecar = xecaddr[0];
	xe->xedar = xeiaddr[0];
	xe->xexcar = (xecaddr[1] >> 4) & 03;
	xe->xexdar = (xeiaddr[1] >> 4) & 03;
	s = CSRGO | ((xecaddr[1] & 017) << 8) | ((xeiaddr[1] & 017) << 2);
	xe->xecsr = s;
	while (!((s = xe->xecsr) & (CSRDONE | CSRERROR)))
		;
	if (s & CSRERROR) {
		for (ccsr = 0; ccsr < 4; ccsr++)
			xeerr[ccsr] = 0;
		map (&xeerr, xeeaddr);
		printf ("error initing drive");
		ccsr = xe->xeccsr;
		printf ("csr=%o ccsr=%o\n", s, ccsr);
		xec.xeop = XERSS;
		xec.xeunit = (ccsr >> 5) & 7;
		xe->xecar = xecaddr[0];
		xe->xedar = xeeaddr[0];
		xe->xexcar = xecaddr[1] >> 4;
		xe->xexdar = xeeaddr[1] >> 4;
		s = CSRGO | ((xecaddr[1] & 017) << 8) | ((xeeaddr[1] & 017) << 2);
		xe->xecsr = s;
		while (!((s = xe->xecsr) & (CSRDONE | CSRERROR)))
			;
		for (ccsr = 0; ccsr < 4; ccsr++)
			printf ("status: 0%o\n", xeerr [ccsr]);
		return 0;
	}
	return 1;
}
