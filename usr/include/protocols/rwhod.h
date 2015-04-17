/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rwhod.h	5.1 (Berkeley) 5/28/85
 */

/*
 * rwho protocol packet format.
 */
struct	outmp {
	char	out_line[8];		/* tty name */
	char	out_name[8];		/* user id */
	long	out_time;		/* time on */
};

struct	whod {
	char	wd_vers;		/* protocol version # */
	char	wd_type;		/* packet type, see below */
	char	wd_pad[2];
	long	wd_sendtime;		/* time stamp by sender */
	long	wd_recvtime;		/* time stamp applied by receiver */
	char	wd_hostname[32];	/* hosts's name */
	long	wd_loadav[3];		/* load average as in uptime */
	long	wd_boottime;		/* time system booted */
	struct	whoent {
		struct	outmp we_utmp;	/* active tty info */
		long	we_idle;	/* tty idle time */
	} wd_we[1024 / sizeof (struct whoent)];
};

#define	WHODVERSION	1
#define	WHODTYPE_STATUS	1		/* host status */
