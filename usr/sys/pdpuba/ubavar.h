/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ubavar.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Per ``device'' structure.
 *
 * This structure contains all the information necessary to run
 * a unibus device such as a dz or a dh.  It also contains information
 * for slaves of unibus controllers as to which device on the slave
 * this is.  A flags field here can also be given in the system specification
 * and is used to tell which dz lines are hard wired or other device
 * specific parameters.
 */
struct uba_device {
	struct	uba_driver *ui_driver;
	short	ui_unit;	/* unit number on the system */
	short	ui_ubanum;	/* the uba it is on */
	caddr_t	ui_addr;	/* address of device in i/o space */
	long	ui_flags;	/* parameter from system specification */
	short	ui_alive;	/* device exists */
};

/*
 * Per-driver structure.
 *
 * Each unibus driver defines entries for a set of routines
 * as well as an array of types which are acceptable to it.
 *
 * This is silly; clean it up.  KB
 */
struct uba_driver {
	int	(*ud_probe)();		/* see if a driver is really there */
	int	(*ud_slave)();		/* see if a slave is there */
	int	(*ud_attach)();		/* setup driver for a slave */
	int	(*ud_dgo)();		/* fill csr/ba to start transfer */
	u_short	*ud_addr;		/* device csr addresses */
	char	*ud_dname;		/* name of a device */
	struct	uba_device **ud_dinfo;	/* backpointers to ubdinit structs */
};

/*
 * Flags to UBA map/bdp allocation routines
 */
#define	UBA_NEEDBDP	0x01		/* transfer needs a bdp */
#define	UBA_CANTWAIT	0x02		/* don't block me */
#define	UBA_NEED16	0x04		/* need 16 bit addresses only */
#define	UBA_HAVEBDP	0x08		/* use bdp specified in high bits */

ubadr_t	uballoc();
#define	ubarelse(a,b)
ubadr_t ubmalloc();
