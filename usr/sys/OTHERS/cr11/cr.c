/*
 *	SCCS id	@(#)cr.c	2.1 (Berkeley)	8/5/83
 */

/*
Module Name:
	cr.c -- card reader driver for Unix

Installation:

Function:
	Provide the interface necessary to handle the CR-11 card reader.

Globals contained:
	cr11 control info and wait channel address

Routines contained:
	cropen	process-level open request
	crclose	process-level close request
	crread	process-level read request
	crint	interrupt handler

Modules referenced:
	param.h	miscellaneous parameters
	conf.h	configuration options
	user.h	definition of the user table

Modules referencing:
	c.c and l.s

Compile time parameters and effects:
	CRPRI	Waiting priority for the card reader.  Should be positive.
		Since the card reader is a delayable device, this needn't
		be particularly high (i.e., a low number).
	CRLOWAT	The low water mark.  If the card reader has been stopped
		because the process code has not been taking the data, this
		is the point at which to restart it.
	CRHIWAT	The high water mark.  If the queued data exceeds this limit
		at a card boundry, the reader will be temporarily stopped.

Module History:
	Created 30Jan78 by Greg Noel, based upon the version from the
	Children's Museum, which didn't work on our system (the original
	was Copyright 1974; reproduced by permission).
*/
/*!	Rehacked for v7 by Bill Jolitz 8/79			*/
#include "param.h"
#include "cr.h"
#if	NCR > 0
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/crreg.h>

#define	CRPRI	(PZERO+10)
#define	CRLOWAT	50
#define	CRHIWAT	160

#define	NLCHAR	0140	/* internal value used for end-of-line */
#define	EOFCHAR	07417	/* end-of-file character (not currently used) */

/* card reader status */
struct	{
	int	crstate;	/* see below */
	struct	{
		int	cc;
		int	cf;
		int	cl;
	} crin;
} cr11;
/* bits in crstate */
#define	CLOSED	0
#define	WAITING	1
#define	READING	2
#define	EOF	3

int	nospl	=	{ 0 };
extern	struct	crdevice	*CRADDR;

char asctab[]
{
	' ', '1', '2', '3', '4', '5', '6', '7',
	'8', ' ', ':', '#', '@', '`', '=', '"',
	'9', '0', '/', 's', 't', 'u', 'v', 'w',
	'x', 'y', ' ', ']', ',', '%', '_', '>',
	'?', 'z', '-', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', ' ', '!', '$', '*', ')',
	';', '\\', 'r', '&', 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', ' ', '[', '.', '<',
	'(', '+', '^', 'i', ' ', ' ', ' ', ' '
};
#define	BADCHAR	'~'	/* was blank */

/*ARGSUSED*/
cropen(dev, flag)
dev_t	dev;
{
	extern	lbolt;

	if (flag!=0 || cr11.crstate!=CLOSED) {
	err:
		u.u_error = ENXIO;
		return;
	}
	while(CRADDR->crcs & (CR_HARDERR|CR_RRS|CR_BUSY)) {
		CRADDR->crcs = 0;
		sleep(&lbolt, CRPRI);
		if(cr11.crstate != CLOSED)
			goto err; /* somebody else got it */
	}
	CRADDR->crcs = CR_IE|CR_READ;
	cr11.crstate = WAITING;
}

/*ARGSUSED*/
crclose(dev, flag)
dev_t	dev;
{
	if(!nospl)
		(void) _spl6();
	CRADDR->crcs = 0;
	while (getc(&cr11.crin) >= 0)
		;
	cr11.crstate = CLOSED;
	(void) _spl0();
}

crread()
{
	register int c;

	do {
		if(!nospl)
		(void) _spl6();
		while((c = getc(&cr11.crin))<0) {
			if(cr11.crstate == EOF)
				goto out;
			if((CRADDR->crcs & CR_CARDDONE) && (cr11.crin.cc<CRHIWAT))
				CRADDR->crcs |= CR_IE|CR_READ;
			sleep(&cr11.crin,CRPRI);
		}
		(void) _spl0();
	} while (passc(asciicon(c)) >= 0);

out:
	(void) _spl0();
}

crint()
{
	if (cr11.crstate == WAITING) {
		if(CRADDR->crcs & CR_ERR) {
			CRADDR->crcs = CR_IE|CR_READ;
			return;
		}
		cr11.crstate = READING;
	}
	if(cr11.crstate == READING) {
		if (CRADDR->crcs & CR_ERR)
			/*
			This code is not really smart enough. The actual
			EOF condition is indicated by a simultaneous
			CR_HCHK and CR_CARDDONE; anything else is a real
			error.	 In the event of a real error we should
			discard the current card image, wait for the
			operator to fix it, and continue.  This is very
			hard to do.....
			*/
			cr11.crstate = EOF;
		else
			{
			if ((CRADDR->crcs&CR_CARDDONE) == 0) {
				putc(CRADDR->crb2,&cr11.crin);
				return;
			} else
				{
				cr11.crstate = WAITING;
				if (cr11.crin.cc < CRHIWAT)
					CRADDR->crcs = CR_IE|CR_READ;
				}
		}
		putc(NLCHAR,&cr11.crin);   /* card end or EOF -- insert CR */
		wakeup(&cr11.crin);
	}
}

asciicon(c)
{
	register c1, c2;

	c1 = c&0377;
	if (c1 == NLCHAR)
		return('\n');
	if (c1>=0200)
		c1 -= 040;
	c2 = c1;
	while ((c2 -= 040) >= 0)
		c1 -= 017;
	if(c1 > 0107)
		c1 = BADCHAR;
	else
		c1 = asctab[c1];
	return(c1);
}
#endif	NCR
