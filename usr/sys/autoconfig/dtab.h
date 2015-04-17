/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dtab.h	2.0 (2.11BSD GTE) 11/20/92
 */

/*
 * Internal structure used to store info from the device table
 */
typedef struct nlist	NLIST;

typedef struct handler_s {
	struct handler_s	*s_next;
	char	*s_str;
	NLIST	*s_nl;
} HAND;

typedef struct dtab_s {
	struct dtab_s	*dt_next;	/* Next device in list */
	char	*dt_name;		/* Two character device name */
	int	dt_unit;		/* Unit number, -1 is wildcard */
	int	dt_addr;		/* CSR address */
	int	dt_vector;		/* Interrupt vector */
	HAND	*dt_handlers;		/* Interrupt handlers */
	int	dt_br;			/* Priority */
	int	(*dt_uprobe)();		/* User-level (internal) probe */
	NLIST	*dt_probe,		/* Address of probe function */
		*dt_setvec,		/* Address of vector set function */
		*dt_attach;		/* Address of attach function */
} DTAB;

DTAB	*devs;				/* Head of device list */

#define EOS	'\0'			/* end of string */
#define ERR	-1			/* general error */
extern char	*myname,		/* program name */
		*dtab_name;		/* dtab file name */
