/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)exec.h	1.2 (2.11BSD GTE) 10/31/93
 */

#ifndef _EXEC_
#define _EXEC_
/*
 * Header prepended to each a.out file.
 */
struct	exec {
	int	a_magic;	/* magic number */
unsigned int	a_text;		/* size of text segment */
unsigned int	a_data;		/* size of initialized data */
unsigned int	a_bss;		/* size of uninitialized data */
unsigned int	a_syms;		/* size of symbol table */
unsigned int	a_entry; 	/* entry point */
unsigned int	a_unused;	/* not used */
unsigned int	a_flag; 	/* relocation info stripped */
};

#define	NOVL	15		/* number of overlays */
struct	ovlhdr {
	int	max_ovl;	/* maximum overlay size */
unsigned int	ov_siz[NOVL];	/* size of i'th overlay */
};

/*
 * eXtended header definition for use with the new macros in a.out.h
*/
struct	xexec {
	struct	exec	e;
	struct	ovlhdr	o;
	};

#define	A_MAGIC1	0407	/* normal */
#define	A_MAGIC2	0410	/* read-only text */
#define	A_MAGIC3	0411	/* separated I&D */
#define	A_MAGIC4	0405	/* overlay */
#define	A_MAGIC5	0430	/* auto-overlay (nonseparate) */
#define	A_MAGIC6	0431	/* auto-overlay (separate) */

#endif
