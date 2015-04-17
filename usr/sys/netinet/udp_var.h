/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)udp_var.h	7.3.2 (2.11BSD GTE) 1995/10/09
 */

/*
 * UDP kernel structures and variables.
 */
struct	udpiphdr {
	struct 	ipovly ui_i;		/* overlaid ip structure */
	struct	udphdr ui_u;		/* udp header */
};
#define	ui_next		ui_i.ih_next
#define	ui_prev		ui_i.ih_prev
#define	ui_pad		ui_i.ih_pad
#define	ui_x1		ui_i.ih_x1
#define	ui_pr		ui_i.ih_pr
#define	ui_len		ui_i.ih_len
#define	ui_src		ui_i.ih_src
#define	ui_dst		ui_i.ih_dst
#define	ui_sport	ui_u.uh_sport
#define	ui_dport	ui_u.uh_dport
#define	ui_ulen		ui_u.uh_ulen
#define	ui_sum		ui_u.uh_sum

struct	udpstat {
				/* input statistics: */
	long	udps_ipackets;		/* total input packets */
	long	udps_hdrops;		/* packet shorter than header */
	long	udps_badsum;		/* checksum error */
	long	udps_badlen;		/* data length larger than packet */
	long	udps_noport;		/* no socket on port */
	long	udps_noportbcast;	/* of above, arrived as broadcast */
	long	udps_fullsock;		/* not delivered, input socket full */
	long	udpps_pcbcachemiss;	/* input packets missing pcb cache */
				/* output statistics: */
	long	udps_opackets;		/* total output packets */
};

/*
 * Names for UDP sysctl objects
 */
#define	UDPCTL_CHECKSUM		1	/* checksum UDP packets */
#define UDPCTL_MAXID		2

#ifndef	KERNEL
#define UDPCTL_NAMES { \
	{ 0, 0 }, \
	{ "checksum", CTLTYPE_INT }, \
}
#endif

#ifdef SUPERVISOR
struct	inpcb udb;
struct	udpstat udpstat;
#endif
