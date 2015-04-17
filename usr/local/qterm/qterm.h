/*
 * Copyright (c) 1990 Michael A. Cooper.
 * This software may be freely distributed provided it is not sold for 
 * profit and the author is credited appropriately.
 */

/*
 * $Header: /src/common/usc/bin/qterm/RCS/qterm.h,v 5.1 1991/03/12 00:46:24 mcooper Exp $
 *------------------------------------------------------------------
 *
 * $Source: /src/common/usc/bin/qterm/RCS/qterm.h,v $
 * $Revision: 5.1 $
 * $Date: 1991/03/12 00:46:24 $
 * $State: Exp $
 * $Author: mcooper $
 * $Locker:  $
 *
 *------------------------------------------------------------------
 *
 * Michael A. Cooper
 * Research and Development Group
 * University Computing Services 
 * University of Southern California
 * (mcooper@usc.edu)
 *
 *------------------------------------------------------------------
 * $Log: qterm.h,v $
 * Revision 5.1  1991/03/12  00:46:24  mcooper
 * - Changed CMASK to CHAR_CMASK to avoid conflict
 *   under AIX 3.1.
 * - Expand tabs.
 *
 * Revision 5.0	 1990/12/15  18:30:45  mcooper
 * Version 5.
 *
 * Revision 4.1	 90/12/15  18:14:27  mcooper
 * Add copywrite.
 * 
 * Revision 4.0	 88/03/08  19:31:23  mcooper
 * Version 4.
 * 
 * Revision 3.2	 88/03/08  19:28:52  mcooper
 * Major rewrite.
 * 
 * Revision 3.1	 88/03/08  15:32:16  mcooper
 * Changed around user's qtermtab
 * file names.
 * 
 * Revision 3.0	 87/06/30  19:09:04  mcooper
 * Release of version 3.
 * 
 * Revision 2.4	 87/06/30  19:02:28  mcooper
 * WAIT changed to 2 for slow systems.
 * 
 *------------------------------------------------------------------
 */



#ifndef TABFILE
# define TABFILE	"/usr/local/lib/qtermtab" /* Default qtermtab file */
#endif
#define USRFILE		".qtermtab"		/* User's qtermtab file */
#define OLDUSRFILE	".qterm"		/* Old user qtermtab file */
#define ALTSEND		"\033[c"		/* Alternate query string */
#define WAIT		2			/* Timeout (in seconds) */
#define SIZE		512			/* Receive buffer size */
#define CHAR_MASK	0377			/* Character mask */
#define ESC		'\033'			/* ESCAPE */

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define TRUE		1
#define FALSE		0

#ifdef USG5
# define crmode()	(_ntty.c_lflag &= ~ICANON,\
			_ntty.c_cc[VMIN] = 1, _ntty.c_cc[VTIME] = 0,\
			ioctl(_tty_ch, TCSETAF, &_ntty))
# define nocrmode()	(_ntty.c_lflag |= ICANON,\
			_ntty.c_cc[VMIN] = _otty.c_cc[VMIN],\
			_ntty.c_cc[VTIME] = _otty.c_cc[VTIME],\
			ioctl(_tty_ch, TCSETAF, &_ntty))
# define echo()		(_ntty.c_lflag |= ECHO,\
			ioctl(_tty_ch, TCSETAF, &_ntty))
# define noecho()	(_ntty.c_lflag &= ~ECHO,\
			ioctl(_tty_ch, TCSETAF, &_ntty))
#else /* !USG5 */
# define crmode()	(_tty.sg_flags |= CBREAK,\
			ioctl(_tty_ch, TIOCSETP, &_tty))
# define nocrmode()	(_tty.sg_flags &= ~CBREAK,\
			ioctl(_tty_ch, TIOCSETP, &_tty))
# define echo()		(_tty.sg_flags |= ECHO,	  \
			ioctl(_tty_ch, TIOCSETP, &_tty))
# define noecho()	(_tty.sg_flags &= ~ECHO,  \
			ioctl(_tty_ch, TIOCSETP, &_tty))
#endif /* USG5 */

/*
 * Terminal table structure
 */
struct termtable {
    char		*qt_sendstr;	/* String to send to terminal */
    char		*qt_recvstr;	/* String expected in response */
    char		*qt_termname;	/* Terminal name */
    char		*qt_fullname;	/* Full terminal name & description */
    struct termtable    *nxt;		/* Next structure */
};
struct termtable *termtab = NULL;
