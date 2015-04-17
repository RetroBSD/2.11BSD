/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rlreg.h	1.2 (2.11BSD) 1997/7/20
 */

struct	rldevice
{
	short	rlcs;
	caddr_t	rlba;
	short	rlda;
	short	rlmp;
	short	rlbae;
};

/* bits in rlcs */
#define	RL_CERR		0100000		/* composite error */
#define	RL_DRE		0040000		/* drive error */
#define	RL_NXM		0020000		/* nonexistent memory */
#define	RL_DLHNF	0010000		/* data late or header not found */
#define	RL_CRC		0004000		/* data crc or header check or write check */
#define	RL_OPI		0002000		/* operation incomplete */
/* bits 9-8 are drive select */
#define	RL_CRDY		0000200		/* controller ready */
#define	RL_IE		0000100		/* interrupt enable */
/* bits 5-4 are the UNIBUS address extension bits */
/* bits 3-0 is the function code */
#define	RL_HARDERR	(RL_NXM|RL_DLHNF|RL_CRC|RL_OPI)

/* commands */
#define	RL_NOP		0000000		/* no operation */
#define	RL_WCHK		0000002		/* write check data */
#define	RL_GETSTATUS	0000004		/* get status */
#define	RL_SEEK		0000006		/* seek */
#define	RL_RHDR		0000010		/* read header */
#define	RL_WCOM		0000012		/* write data */
#define	RL_RCOM		0000014		/* read data */
#define	RL_RWHCHK	0000016		/* read data without header check */

#define	RL_BITS		\
"\10\20CERR\17DRE\16NXM\15DLHNF\14CRC\13OPI\10CRDY\7IE"

/* bits in rlda:  just be thankful there's a one to one correspondence */
#define	RLDA_RW_HSEL	0000100		/* head select during read or write */
#define	RLDA_SEEK_HSEL	0000020		/* head select during seek*/
#define	RLDA_RESET	0000011		/* reset during get status */
#define	RLDA_SEEKHI	0000005		/* seek to higher address */
#define	RLDA_SEEKLO	0000001		/* seek to lower address */
#define	RLDA_GS		0000003		/* get status */

#define	RLDA_BITS	\
"\10\7RW_HSEL\5SEEK_HSEL\4GS_RESET\3SEEK_DIR"

/* bits in rlmp */
#define	RLMP_WDE	0100000		/* write data error */
#define	RLMP_HCE	0040000		/* head current error */
#define	RLMP_WL		0020000		/* write lock */
#define	RLMP_STIMO	0010000		/* seek timeout */
#define	RLMP_SPE	0004000		/* spin error */
#define	RLMP_WGE	0002000		/* write gate error */
#define	RLMP_VCHK	0001000		/* volume check */
#define	RLMP_DSE	0000400		/* drive select error */
#define	RLMP_DTYP	0000200		/* drive type:  0 == RL01, 1 == RL02 */
#define	RLMP_HSEL	0000100		/* head select */
#define	RLMP_CO		0000040		/* cover open */
#define	RLMP_HO		0000020		/* heads out */
#define	RLMP_BH		0000010		/* brush home */
/* bits 2-0 are the state */

/* status bits */
#define	RLMP_LOAD	0000000		/* load cartridge */
#define	RLMP_SU		0000001		/* spin up */
#define	RLMP_BC		0000002		/* brush cycle */
#define	RLMP_LH		0000003		/* load heads */
#define	RLMP_SEEK	0000004		/* seek */
#define	RLMP_LCKON	0000005		/* lock on */
#define	RLMP_UH		0000006		/* unload heads */
#define	RLMP_SD		0000007		/* spin down */

#define	RLMP_BITS	\
"\10\20WDE\17HCE\16WLE\15STIMO\14SPE\13WGE\12VCHK\11DSE\10DTYP\
\7HSEL\6CO\5HO\4BH"
