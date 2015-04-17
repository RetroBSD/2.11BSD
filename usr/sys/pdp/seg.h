/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)seg.h	1.2 (2.11BSD GTE) 1/1/93
 */

#ifndef _SEG_
#define _SEG_

/*
 * Access abilities
 */
#define RO	02		/* Read only */
#define RW	06		/* Read and write */
#define NOACC	0		/* Abort all accesses */
#define ACCESS	07		/* Mask for access field */
#define ED	010		/* Extension direction */
#define TX	020		/* Software: text segment */
#define ABS	040		/* Software: absolute address */

#ifndef SUPERVISOR
u_short	*ka6;			/* nonseparate:  KISA6; separate:  KDSA6 */
u_int	kdsa6;			/* saved KDSA6, if any */
#endif

/*
 * Addresses and bits for DEC's KT-11
 * and macros for remapping kernel data space.
 */
#define	UISD	((u_short *) 0177600)	/* first user I-space descriptor */
#define	UDSD	((u_short *) 0177620)	/* first user D-space descriptor */

#ifdef	ENABLE34
#	define DEC_UISA7	((u_short *) 0177656)
	extern u_short *UISA, *UDSA, *KISA0, *KDSA1, *KDSA2, *KDSA5, *KDSA6;
#else
#	define UISA	((u_short *) 0177640)	/* first user I-space address */
#	define UDSA	((u_short *) 0177660)	/* first user D-space address */
#endif

#ifndef ENABLE34
#	define KISA0	((u_short *) 0172340)
#	define KISA1	((u_short *) 0172342)
#	define KISA5	((u_short *) 0172352)
#	define KISA6	((u_short *) 0172354)
#	define SSR3	((u_short *) 0172516)
#endif

#define	SISD0	((u_short *) 0172200)
#define	SDSD0	((u_short *) 0172220)
#define	SISA0	((u_short *) 0172240)
#define	SDSA0	((u_short *) 0172260)
#define	KISD1	((u_short *) 0172302)
#define	KISD5	((u_short *) 0172312)
#define	KISD6	((u_short *) 0172314)

#ifdef KERN_NONSEP
#	ifndef ENABLE34
#		define KDSA5	KISA5
#		define KDSA6	KISA6
#	endif !ENABLE34
#	define KDSD5	KISD5
#	define KDSD6	KISD6
#else
#	ifndef ENABLE34
#		define	SDSA5	((u_short *) 0172272)
#		define	SDSA6	((u_short *) 0172274)
#		define  KDSA5	((u_short *) 0172372)
#		define  KDSA6	((u_short *) 0172374)
#	endif !ENABLE34
#	define SDSD5	((u_short *) 0172232)
#	define SDSD6	((u_short *) 0172234)
#	define KDSD5	((u_short *) 0172332)
#	define KDSD6	((u_short *) 0172334)
#endif

/* start of possibly mapped area; see machdep.c */
#ifndef SUPERVISOR
#define	REMAP_AREA	((caddr_t)&proc[0])
#endif
#define	SEG5		((caddr_t) 0120000)
#define	SEG6		((caddr_t) 0140000)
#define	SEG7		((caddr_t) 0160000)

#ifdef COMMENT
 * Macros for resetting the kernel segmentation registers to map in
 * out-of-address space data.  If KDSA5 is used for kernel data space
 * only proc, file and text tables are allowed in that range.  Routines
 * can repoint KDSA5 to map in data such as buffers or clists without
 * raising processor priority by calling these macros.  Copy (in mch.s) uses
 * two registers, KDSA5 and 6. If KDSA6 is in use, the prototype register
 * kdsa6 will be non-zero, and the kernel will be running on a temporary stack
 * in bss.  Interrupt routines that access any of the structures in this range
 * or the u. must call savemap (in machdep.c) to save the current mapping
 * information in a local structure and restore it before returning.
 *
 * USAGE:
 *	To repoint KDSA5 from the top level,
 *		mapseg5(addr, desc);	/* KDSA5 now points at addr */
 *		...
 *		normalseg5();		/* normal mapping */
 *
 *	To repoint KDSA5 from interrupt or top level,
 *		segm saveregs;
 *		saveseg5(saveregs);	/* save previous mapping of segment 5 */
 *		mapseg5(addr, desc);	/* KDSA5 now points at addr */
 *		...
 *		restorseg5(saveregs);	/* restore previous mapping */
 *
 *	To access proc, text, file or user structures from interrupt level,
 *		mapinfo map;		/* save ALL mapping information, */
 *		savemap(map);		/* restore normal mapping of KA5 */
 *		...			/* and 6 */
 *		restormap(map);		/* restore previous mapping */
#endif /* COMMENT */

/*
 * Structure to hold a saved PAR/PDR pair.
 */
struct segm_reg {
	u_int	se_desc;
	u_int	se_addr;
};
typedef struct segm_reg segm;
typedef struct segm_reg mapinfo[2];	/* KA5, KA6 */

#ifndef DIAGNOSTIC
#	define	mapout(bp)	normalseg5()
#endif

/* use segment 5 to access the given address. */
#ifdef SUPERVISOR
#define mapseg5(addr,desc) { \
	*SDSD5 = desc; \
	*SDSA5 = addr; \
}
#else
#define mapseg5(addr,desc) { \
	*KDSD5 = desc; \
	*KDSA5 = addr; \
}
#endif

/* save the previous contents of KDSA5/KDSD5. */
#ifdef SUPERVISOR
#define saveseg5(save) { \
	save.se_addr = *SDSA5; \
	save.se_desc = *SDSD5; \
}
#else
#define saveseg5(save) { \
	save.se_addr = *KDSA5; \
	save.se_desc = *KDSD5; \
}
#endif

/* restore normal kernel map for seg5. */
	extern segm	seg5;		/* prototype KDSA5, KDSD5 */
#define normalseg5()	restorseg5(seg5)

/* restore the previous contents of KDSA5/KDSD5. */
#ifdef SUPERVISOR
#define restorseg5(save) { \
	*SDSD5 = save.se_desc; \
	*SDSA5 = save.se_addr; \
}
#else
#define restorseg5(save) { \
	*KDSD5 = save.se_desc; \
	*KDSA5 = save.se_addr; \
}
caddr_t	mapin();
#endif
#endif /* _SEG_ */
