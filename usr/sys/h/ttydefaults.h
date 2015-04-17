/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ttydefaults.h	1.2 (2.11BSD) 1997/4/15
 */

/*
 * System wide defaults of terminal state.
 */
#ifndef _TTYDEFAULTS_
#define	_TTYDEFAULTS_

#define CTRL(x)	('x'&037)
/*
 * Control Character Defaults
 */
#define	CEOF		CTRL(d)
#define	CEOL		_POSIX_VDISABLE
#define	CERASE		CTRL(h)
#define	CINTR		CTRL(c)	
#define	CKILL		CTRL(u)
#define	CMIN		1
#define	CQUIT		034	/* FS, ^\ */
#define	CSUSP		CTRL(z)
#define	CTIME		1
#define	CDSUSP		CTRL(y)
#define	CSTART		CTRL(q)
#define	CSTOP		CTRL(s)
#define	CLNEXT		CTRL(v)
#define	CFLUSHO 	CTRL(o)
#define	CWERASE 	CTRL(w)
#define	CREPRINT 	CTRL(r)
#define CQUOTE		'\\'
#define	CEOT		CEOF

#define	CBRK		CEOL
#define CRPRNT		CREPRINT
#define CFLUSH		CFLUSHO

/*
 * Settings on first open of a tty.
 */
#define TTYDEF_SPEED	(B9600)

#endif /*_TTYDEFAULTS_*/

/*
 * Define TTYDEFCHARS to include an array of default control characters.
 */
#ifdef TTYDEFCHARS
u_char	ttydefchars[NCC] = {
	CEOF,	CEOL,	CEOL,	CERASE, CWERASE, CKILL, CREPRINT, CQUOTE,
	CINTR,	CQUIT,	CSUSP,	CDSUSP,	CSTART,	CSTOP,	CLNEXT,
	CFLUSHO, CMIN,	CTIME, _POSIX_VDISABLE, _POSIX_VDISABLE
};
#endif /*TTYDEFCHARS*/
