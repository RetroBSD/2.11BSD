/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)prf.c	1.4 (2.11BSD) 1996/06/04
 */

#include "../machine/cons.h"

#define	KLADDR	((struct dldevice *)0177560)

#define	CTRL(x)	('x' & 037)

/*
 * Scaled down version of C Library printf.  Only %s %u %d (==%u) %o %x %D
 * are recognized.  Used to print diagnostic information directly on
 * console tty.  Since it is not interrupt driven, all system activities
 * are pretty much suspended.  Printf should not be used for chit-chat.
 */
printf(fmt, x1)
	register char *fmt;
	unsigned x1;
{
	register c;
	register unsigned int *adx;
	unsigned char *s;

	adx = &x1;
loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c);
	}
	c = *fmt++;
	if (c == 'd' || c == 'u' || c == 'o' || c == 'x')
		printn((long)*adx, c=='o'? 8: (c=='x'? 16:10));
	else if (c == 's') {
		s = (unsigned char *)*adx;
		while (c = *s++)
			putchar(c);
	} else if (c == 'D' || c == 'O') {
		printn(*(long *)adx, c == 'D' ?  10 : 8);
		adx += (sizeof(long) / sizeof(int)) - 1;
	} else if (c == 'c')
		putchar((char *)*adx);
	adx++;
	goto loop;
}

/*
 * Print an unsigned integer in base b.
 */
printn(n, b)
	long n;
	register int b;
{
	long a;

	if (n < 0) {	/* shouldn't happen */
		putchar('-');
		n = -n;
	}
	if (a = n/b)
		printn(a, b);
	putchar("0123456789ABCDEF"[(int)(n%b)]);
}

putchar(c)
	register c;
{
	register s;
	register unsigned timo;

#ifdef notdef
	/*
	 *  If last char was a break or null, don't print
	 */
	if ((KLADDR->dlrbuf & 0177) == 0)
		return;
#endif
	/*
         *  If we got a XOFF, wait for a XON
         */
	if ((KLADDR->dlrcsr & DL_RDONE) != 0)
		if ((KLADDR->dlrbuf & 0177) == 19)
			while ((KLADDR->dlrbuf & 0177) != 17) 
				continue;
	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	timo=60000;
	while ((KLADDR->dlxcsr & DLXCSR_TRDY) == 0)
		if (--timo == 0)
			break;
	if (c == 0)
		return(c);
	s = KLADDR->dlxcsr;
	KLADDR->dlxcsr = 0;
	KLADDR->dlxbuf = c;
	if (c == '\n') {
		putchar('\r');
		putchar(0177);
	}
	putchar(0);
	KLADDR->dlxcsr = s;
	return(c);
}

getchar()
{
	register c;

	KLADDR->dlrcsr = DL_RE;
	while ((KLADDR->dlrcsr & DL_RDONE) == 0)
		continue;
	c = KLADDR->dlrbuf & 0177;
	if (c=='\r')
		c = '\n';
	return(c);
}

gets(buf)
	char *buf;
{
	register char *lp, *cp;
	register int c;

	lp = buf;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
			default:
				if (c < ' ' || c >= 127)
					putchar(CTRL(G));
				else {
					*lp++ = c;
					putchar(c);
				}
				break;

			case '\n':
			case '\r':
				putchar('\n');
				c = '\n';
				*lp++ = '\0';
				return;

			case '\177':
			case '\b':
			case '#':
				if (lp <= buf)
					putchar(CTRL(G));
				else {
					lp--;
					putchar('\b');
					putchar(' ');
					putchar('\b');
				}
				break;

			case CTRL(U):
			case '@':
				while (lp > buf) {
					lp--;
					putchar('\b');
					putchar(' ');
					putchar('\b');
				}
				break;

			case CTRL(R):
				putchar('^');
				putchar('R');
				putchar('\n');
				for (cp = buf; cp < lp; cp++)
					putchar(*cp);
				break;
		}
	}
}
