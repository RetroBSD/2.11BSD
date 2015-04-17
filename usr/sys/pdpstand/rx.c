/*
 * RX02 Standalone disk driver.
 * 96/3/8, Steve Schultz (sms@moe.2bsd.com)
 * 95/12/02, Tim Shoppa (shoppa@altair.krl.caltech.edu)
 *
 *	Layout of logical devices:
 *
 *	name	min dev		unit	density
 *	----	-------		----	-------
 *	rx0a	   0		  0	single
 *	rx1a	   1		  1	single
 *	rx0b	   2		  0	double
 *	rx1b	   3		  1	double
 *
 *	the following defines use some fundamental
 *	constants of the RX02.
 */

#define	NSPB	(4-2*(pn))		/* sectors per block */
#define	NRXBLKS	(1001-501*(pn))	/* blocks on device */
#define	NBPS	(128+128*(pn))	/* bytes per sector */
#define	DENSITY	((pn))	/* Density: 0 = single, 1 = double */
#define	UNIT	((dn))	/* Unit Number: 0 = left, 1 = right */
#define	RXGD	(RX_GO  | (DENSITY << 8))

#define	rxwait()	while (((rxaddr->rxcs) & RX_XREQ) == 0)
#define rxdone()	while (((rxaddr->rxcs) & RX_DONE) == 0)

#include "../h/param.h"
#include "../pdpuba/rxreg.h"
#include "saio.h"

#define	NRX	2

	struct	rxdevice *RXcsr[NRX+1]=
		{
		(struct rxdevice *)0177170,
		(struct	rxdevice *)0,
		(struct rxdevice *)-1
		};

rxstrategy(io, func)
	register struct iob *io;
{
	register struct rxdevice *rxaddr;
	daddr_t bn;
	unsigned int sectno,sector,track,dn,pn,bae,lo16,lotemp,cc;
	unsigned int bc,bs,retry;

	rxaddr = RXcsr[io->i_ctlr];
	bn = io->i_bn;
	dn = io->i_unit;
	pn = io->i_part;
	cc = io->i_cc;
	iomapadr(io->i_ma, &bae, &lo16);
	bc=0;

	for (sectno=0; bc<cc; sectno++) {
		rxfactr((int)bn*NSPB+sectno,&sector,&track);
		if (func == READ) {
	    		retry=0;
rxretry:    		rxaddr->rxcs=RX_RSECT|RXGD|(UNIT<<4);
	    		rxwait();
	    		rxaddr->rxsa=sector;
	    		rxwait();
	    		rxaddr->rxta=track;
	    		rxdone();
	    		if (rxaddr->rxcs & RX_ERR) {
				if ((retry++) < 10) goto rxretry;
	        		goto rxerr;
	    		}
	  	}
	  	bs = ((cc-bc<NBPS) ? (cc-bc) : (NBPS));
	  	rxaddr->rxcs=((func==READ)?RX_EMPTY:RX_FILL)|RXGD|(bae<<12);
	  	rxwait();
	  	rxaddr->rxwc=bs/2;
	  	rxwait();
	  	rxaddr->rxba=lo16;
	  	rxdone();
	  	if (rxaddr->rxcs & RX_ERR) goto rxerr;
	  	if (func==WRITE) {
	  		rxaddr->rxcs=RX_WSECT|RXGD|(UNIT<<4);
	  		rxwait();
	  		rxaddr->rxsa=sector;
	  		rxwait();
	  		rxaddr->rxta=track;
	  		rxdone();
	  		if (rxaddr->rxcs & RX_ERR) goto rxerr;
	  	}
	lotemp=lo16;
	lo16=lo16+NBPS;
	if (lo16 < lotemp)
		bae=bae+1;
	bc=bc+bs;
	}
	return(io->i_cc);

rxerr:	printf("%s rxcs %o rxes %o\n",devname(io), rxaddr->rxcs,rxaddr->rxes);
	return(-1);
}

	
rxopen(io)
	struct iob *io;
{
	return(genopen(NRX, io));
}

/*
 *	rxfactr -- calculates the physical sector and physical
 *	track on the disk for a given logical sector.
 *	call:
 *		rxfactr(logical_sector,&p_sector,&p_track);
 *	the logical sector number (0 - 2001) is converted
 *	to a physical sector number (1 - 26) and a physical
 *	track number (0 - 76).
 *	the logical sectors specify physical sectors that
 *	are interleaved with a factor of 2. thus the sectors
 *	are read in the following order for increasing
 *	logical sector numbers (1,3, ... 23,25,2,4, ... 24,26)
 *	There is also a 6 sector slew between tracks.
 *	Logical sectors start at track 1, sector 1; go to
 *	track 76 and then to track 0.  Thus, for example, unix block number
 *	498 starts at track 0, sector 25 and runs thru track 0, sector 2
 *	(or 6 depending on density).
 */
static
rxfactr(sectr, psectr, ptrck)
	register int sectr;
	int *psectr, *ptrck;
{
	register int p1, p2;

	p1 = sectr / 26;
	p2 = sectr % 26;
	/* 2 to 1 interleave */
	p2 = (2 * p2 + (p2 >= 13 ?  1 : 0)) % 26;
	/* 6 sector per track slew */
	*psectr = 1 + (p2 + 6 * p1) % 26;
	if (++p1 >= 77)
		p1 = 0;
	*ptrck = p1;
}
