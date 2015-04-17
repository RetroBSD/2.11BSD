/*
 *	SCCS id	@(#)du.c	2.2 (2.11BSD)	11/26/94
 */

/*
 * DU-11 and DUP-11 synchronous line interface driver
 */


#include "du.h"
#if	NDU > 0
#include "param.h"
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/dureg.h>

extern	struct	dudevice *DUADDR;

/*
 * receiver interrupt code state definitions
 */
#define	RIDLE	0		/* idle, waiting for a message */
#define	RRUN	1		/* running, message in progress */
#define	RQUIT1	2		/* quit, trashing all until carrier drop */
#define	RQUIT2	3		/* same as above, but return error */

/*
 * transmitter interrupt code state definitions
 */
#define	TIDLE	0		/* idle, waiting for a message */
#define	TRUN	1		/* running, message in progress */
#define	TLAST	2		/* sending last character */
#define	TCARDRP	3		/* waiting for carrier hold-down delay */

/*
 * miscellaneous defintions
 */
#define	SYN	026		/* sync character */
#define	MSGN	5		/* number of message buffers each direction */
#define	MSGLEN	102		/* length of each message buffer */
#define	TIMEOUT	3		/* number of seconds before receive timeout */
#define	TDELAY	2		/* number ticks to hold down xmit carrier */
#define	DUPRI	5		/* du11 sleep priority */

/*
 * message buffer descriptor
 */
struct	msgbuf	{
	caddr_t	msgbufp;	/* pointer to message buffer */
	char	msgbc;		/* message byte count */
	char	msgerr;		/* nonzero if error in message */
};

/*
 * du11 control info
 */
struct	{
	struct	proc	*duproc;	/* process adr of caller, 0 if closed */
	/*
	 * Transmit info
	 */
	struct	buf	*dutbufp;	/* pointer to transmit buffer */
	struct	msgbuf	dutbuff[MSGN]; /* message buffer descriptors */
	struct	msgbuf	*dutbufi; /* in pointer */
	struct	msgbuf	*dutbufo; /* out pointer */
	short	dutbufn;	/* number of message buffers that are full */
	short	dutstate;	/* state of transmitter interrupt code */
	caddr_t	dutbufc;	/* char ptr for message in progress */
	short	dutbc;		/* byte count of message in progress */
	short	dutdna;		/* number of times data not available */
	/*
	 * Receive info
	 */
	struct	msgbuf	durbuff[MSGN]; /* message buffer descriptors */
	struct	msgbuf	*durbufi; /* in pointer */
	struct	msgbuf	*durbufo; /* out pointer */
	short	durbufn;	/* number of message buffers that are full */
	short	durstate;	/* state of receiver interrupt code */
	caddr_t	durbufc;	/* char ptr for message in progress */
	short	durbc;		/* byte count of message in progress */
	short	durtimer;	/* seconds left until timeout */
	short	durover;	/* number of receiver data overruns */
	short	durfull;	/* number of times receive buffer full */
	short	durlong;	/* number of messages > 255 bytes */
} du11;

#define	MAINTDEV	1	/* minor device for maintenance mode */
int     maint;			/* nonzero if open for maintenance */
 
/*ARGSUSED*/
duopen(dev, flag)
dev_t	dev;
{
	register struct dudevice *duaddr = DUADDR;
	register n;
	extern	durtimeout();

	if (du11.duproc == u.u_procp)
		return;
	if (du11.duproc) {
		u.u_error = ENXIO;
		return;
	}

	/*
	 * Check whether opening for maintenance mode
	 */
	if (minor(dev) == MAINTDEV)
		maint = 1;
	else
		maint = 0;

	/*
	 * reset device
	 */
	duaddr->dutcsr = DUTCSR_MR;

	/*
	 * Allocate transmit buffer
	 */
	du11.dutbufp = geteblk();
	du11.dutbufi = du11.dutbufo = du11.dutbuff;
	du11.dutbufn = 0;
	for(n = 0; n < MSGN; n++)
		du11.dutbuff[n].msgbufp = du11.dutbufp->b_un.b_addr + MSGLEN*n;
	du11.dutstate = TIDLE;

	/*
	 * Allocate receive buffer
	 */
	for(n = 0; n < MSGN; n++)
		du11.durbuff[n].msgbufp = (du11.dutbufp->b_un.b_addr+(BSIZE/2))
						+ (MSGLEN * n);
	du11.durbufi = du11.durbufo = du11.durbuff;
	du11.durbufn = 0;
	du11.durstate = RIDLE;

	/*
	 * Start 1 second receiver timeout clock
	 */
	du11.durtimer = 0;
	timeout(durtimeout, 0, hz);
	du11.duproc = u.u_procp;

	/*
	 * Set parameters according to whether du11 or dup11
	 */
#ifdef	DU11
	duaddr->durdbuf = DUPARCSR_8BITS | DUPARCSR_ISYNC | SYN;
#else
	duaddr->durdbuf = DUPARCSR_DEC | DUPARCSR_CRCINH | SYN;
	if (maint)
		duaddr->dutcsr = DUTCSR_MPSYSTST;
#endif
	/*
	 * Start receiver
	 */
	duaddr->durcsr = DU_STSYNC | DU_DTR | DU_SSYNC;
}

duclose()
{
	register struct	dudevice *duaddr = DUADDR;

	/*
	 * Stop device and disable interrupts
	 */
	duaddr->durcsr = 0;
	duaddr->dutcsr = 0;

	/*
	 * Stop timer
	 */
	du11.durtimer = -1;
	while (du11.durtimer)
		sleep(&du11.durstate, DUPRI);

	/*
	 * Stop interrupt code for sure
	 */
	du11.duproc = 0;

	/*
	 * Release buffers
	 */
#ifdef	UCB_BUFOUT
#define	brelse	abrelse
#endif
	brelse(du11.dutbufp);

	/*
	 * clear maintenance mode flag
	 */
	maint = 0;
}

duread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register nbytes;

	/*
	 * wait for a message to appear in the receive buffer or for TIMEOUT
	 * seconds to elapse
	 */
	du11.durtimer = TIMEOUT;
	(void) _spl6();
	while (du11.durbufn == 0) {
		if (du11.durtimer == 0) {
			(void) _spl0();
			return;
		}
		sleep(&du11.durstate, DUPRI);
	}
	(void) _spl0();
	du11.durtimer = 0;

	/*
	 * if an error was detected on this message, throw it away and return
	 * an error to the caller
	 */
	if (du11.durbufo->msgerr) {
		if (++du11.durbufo == du11.durbuff + MSGN)
			du11.durbufo = du11.durbuff;
		du11.durbufn--;
		u.u_error = EIO;
		return;
	}

	/*
	 * Copy the message to the caller's buffer
	 */
	nbytes = min(uio->uio_resid, du11.durbufo->msgbc);
	uiomove(du11.durbufo->msgbufp, nbytes, uio);
	if (++du11.durbufo == du11.durbuff + MSGN)
		du11.durbufo = du11.durbuff;
	du11.durbufn--;
}

duwrite(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
	register nbytes;

	/*
	 * Wait for there to be room for the message in the buffer
	 */
	while (du11.dutbufn == MSGN)
		sleep(&du11.dutstate, DUPRI);

	/*
	 * Transfer  the message from the caller's buffer to ours
	 */
	nbytes = min(uio->uio_resid, MSGLEN);
	du11.dutbufi->msgbc = nbytes;
	uiomove(du11.dutbufi->msgbufp, nbytes, uio);
	if (++du11.dutbufi == du11.dutbuff + MSGN)
		du11.dutbufi = du11.dutbuff;
	du11.dutbufn++;

	/*
	 * If the interrupt code is not running, start it up
	 */
	if (du11.dutstate == TIDLE)
		duxstart();
}

durint()
{
	register c, dustat;
	register struct dudevice *duaddr = DUADDR;

	dustat = duaddr->durcsr;
	if (du11.duproc == 0) {
		duaddr->dutcsr = DUTCSR_MR;
		if (maint)
#ifdef	DU11
			duaddr->dutcsr |= DUTCSR_MSYSTST;
#ifdef	UCB_DEVERR
		printf("durint: er=%b\n", dustat, DU_BITS);
#else
		printf("durint err %o\n", dustat);
#endif
#else
			duaddr->dutcsr |= DUTCSR_MPSYSTST;
#ifdef	UCB_DEVERR
		printf("durint: er=%b\n", dustat, DU_PBITS);
#else
		printf("durint err %o\n", dustat);
#endif
#endif	DU11
		return;
	}
	switch(du11.durstate) {

	/*
	 * Wait for the first char to be rcvd, ignoring carrier changes
	 */
	case RIDLE:
		if ((dustat & DU_RDONE) == 0)
			return;
		if (du11.durbufn == MSGN)
			goto rfull;
		du11.durbufc = du11.durbufi->msgbufp; /* set up char ptr */
		du11.durbufi->msgerr = 0;	/* set no error in msg */
		du11.durbc = 0;			/* set byte count */
		du11.durstate = RRUN;		/* set message in progress */

	/*
	 * A message is in progress
	 */
	case RRUN:
		if (dustat & DU_RDONE) {	/* a character has arrived */
			c = duaddr->durdbuf;

			/*
			 * End message if lost character or message too long
			 */
			if (c & DURDBUF_OVERRUN) {
				du11.durover++;
				goto rerror;
			}
			if (du11.durbc++ == MSGLEN) {
				du11.durlong++;
				goto rerror;
			}
			*du11.durbufc++ = c;
		}
		if ((dustat & DU_CAR) == 0)	/* carrier drop means */
			break;			/* end of message */
		return;

	/*
	 * We have used up all the message buffers. Throw the
	 * message away.
	 */
rfull:
		du11.durfull++;
		du11.durstate = RQUIT1;
	case RQUIT1:
		if ((dustat & DU_CAR) == 0) {
			du11.durstate = RIDLE;
			duaddr->durcsr &= ~DU_SSYNC;	/* desync receiver */
			duaddr->durcsr |= DU_SSYNC;
		}
		c = duaddr->durdbuf;
		return;

	/*
	 * Flag the message no good and junk the rest of it
	 */
rerror:
		du11.durbufi->msgerr++;
		du11.durstate = RQUIT2;

	case RQUIT2:
		c = duaddr->durdbuf;
		if (dustat & DU_CAR)
			return;
	}

	/*
	 * The message is finished. Set up the byte count and advance
	 * the in pointer to pass the message to the upper-level code.
	 */
	du11.durbufi->msgbc = du11.durbc;
	if (++du11.durbufi == du11.durbuff + MSGN)
		du11.durbufi = du11.durbuff;
	du11.durbufn++;
	du11.durstate = RIDLE;
	duaddr->durcsr &= ~DU_SSYNC;		/* desync receiver */
	duaddr->durcsr |= DU_SSYNC;
	c = duaddr->durdbuf;
	wakeup(&du11.durstate);
}

duxint()
{
	register dustat;
	register struct dudevice *duaddr = DUADDR;
	extern duxfin();

	dustat = duaddr->dutcsr;
	if (du11.duproc == 0) {
		duaddr->dutcsr = DUTCSR_MR;
		if (maint)
#ifdef	DU11
			duaddr->dutcsr |= DUTCSR_MSYSTST;
#ifdef	UCB_DEVERR
		printf("duxint:  er=%b\n", dustat, DUTCSR_BITS);
#else
		printf("duxint err %o\n", dustat);
#endif
#else
			duaddr->dutcsr |= DUTCSR_MPSYSTST;
#ifdef	UCB_DEVERR
		printf("duxint:  er=%b\n", dustat, DUTCSR_PBITS);
#else
		printf("duxint err %o\n", dustat);
#endif
#endif
		return;
	}
	switch(du11.dutstate) {

	/*
	 * A message is in progress
	 */
	case TRUN:
		/*
		 * Count number of times data not available
		 */
		if (dustat & DUTCSR_NODATA)
			du11.dutdna++;
		if (dustat & DUTCSR_TDONE) {
			if (--du11.dutbc) {	/* if there are more chars */
				duaddr->dutdbuf = *du11.dutbufc++ & 0377;
#ifndef	DU11
				if (du11.dutbc == 1) {
					duaddr->dutdbuf = DURDBUF_REOM;
					duaddr->dutcsr &= ~DUTCSR_SEND;
					du11.dutstate = TLAST;
				}
#endif
			}
#ifdef	DU11
			else 			/* wait for the last one */
				{
				duaddr->dutcsr = DUTCSR_SEND | DUTCSR_DNAIE;
				if (maint)
					duaddr->dutcsr |= DUTCSR_MSYSTST;
				du11.dutstate = TLAST;
			}
#endif
		}
		return;

	/*
	 * Wait for the data not available flag, signifying that the
	 * last char has been completely sent
	 */
	case TLAST:
#ifdef	DU11
		if (dustat & DUTCSR_NODATA)
#endif
			{
			du11.dutstate = TCARDRP;
			duaddr->dutcsr = 0;	/* drop carrier */
			if (maint)
#ifdef	DU11
				duaddr->dutcsr |= DUTCSR_MSYSTST;
#else
				duaddr->dutcsr |= DUTCSR_MPSYSTST;
#endif
			duaddr->durcsr &= ~DU_RTS;
			timeout(duxfin, 0, TDELAY);
		}
	}
}

/*
 * Carrier hold-down delay completed
 */
duxfin()
{
	if (++du11.dutbufo == du11.dutbuff + MSGN)
		du11.dutbufo = du11.dutbuff;
	du11.dutbufn--;
	du11.dutstate = TIDLE;

	/*
	 * If there is another message in the buffer, start sending it
	 */
	if (du11.dutbufi != du11.dutbufo)
		duxstart();
	wakeup(&du11.dutstate);
}

/*
 * Start transmitting a new message
 */
duxstart()
{
	register struct dudevice *duaddr = DUADDR;

	du11.dutbc = du11.dutbufo->msgbc;
	du11.dutbufc = du11.dutbufo->msgbufp;
	du11.dutstate = TRUN;
	duaddr->durcsr |= DU_RTS;

	duaddr->dutcsr = DUTCSR_SEND;
	if (maint)
#ifdef	DU11
		duaddr->dutcsr |= DUTCSR_MSYSTST;
#else
		duaddr->dutcsr |= DUTCSR_MPSYSTST;
#endif
	duaddr->dutdbuf = DURDBUF_RSOM | (*du11.dutbufc++ & 0377);

	/*
	 * Turn on interrupts
	 */
	duaddr->durcsr |= DU_RIE;
	duaddr->dutcsr |= DUTCSR_TIE;
}

/*
 * Exit if being closed
 */
durtimeout()
{
	if (du11.durtimer < 0) {
		du11.durtimer = 0;
		wakeup(&du11.durstate);
		return;
	}

	/*
	 * If duread is sleeping, decrement timer and wakeup if time's up
	 */
	if (du11.durtimer > 0)
		if (--du11.durtimer == 0)
			wakeup(&du11.durstate);
	timeout(durtimeout, 0, hz);
}
#endif	NDU
