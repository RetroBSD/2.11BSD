/*
 *	@(#)ioctl_compat.h	1.0 (2.11BSD SMS) 1997/3/28
 *
 * These are flags which have (finally) been removed from ioctl.h.  Several
 * of these have lacked any kernel support for quite a long time.  For others
 * (the delay flags) the kernel support (such as it was) had not been used 
 * that anyone could remember and that kernel support has been removed in the
 * interest of streamlining the tty subsystem (as well as making flag bits
 * available for use without expanding the tty structure).
 *
 * All values are 0.   Since there is no kernel support at all for any of
 * the flags the only reason to use this file is to avoid having to modify
 * the source to whatever application still references the symbols below.
*/

#ifndef	_TTY_COMPAT_OBSOLETE
#define	_TTY_COMPAT_OBSOLETE

#define		LCASE		0x0	/* (obsolete) place holder */
#define		NLDELAY		0x0	/* \n delay */
#define			NL0	0x0
#define			NL1	0x0	/* tty 37 */
#define			NL2	0x0	/* vt05 */
#define			NL3	0x0
#define		TBDELAY		0x0	/* horizontal tab delay */
#define			TAB0	0x0
#define			TAB1	0x0	/* tty 37 */
#define			TAB2	0x0
#define		CRDELAY		0x0	/* \r delay */
#define			CR0	0x0
#define			CR1	0x0	/* tn 300 */
#define			CR2	0x0	/* tty 37 */
#define			CR3	0x0	/* concept 100 */
#define		VTDELAY		0x0	/* vertical tab delay */
#define			FF0	0x0
#define			FF1	0x0	/* tty 37 */
#define		BSDELAY		0x0	/* \b delay */
#define			BS0	0x0
#define			BS1	0x0
#define		ALLDELAY	(NLDELAY|TBDELAY|CRDELAY|VTDELAY|BSDELAY)
#define		TILDE		0x0	/* (obsolete) place holder */
#define		LTILDE		((int)TILDE>>16)) /* (obsolete) place holder */

#define		L001000		0x0
#endif
