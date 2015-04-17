/*	@(#)tmscpreg.h	1.4	(2.11BSD) 1998/1/28 */

/****************************************************************
 *								*
 *        Licensed from Digital Equipment Corporation 		*
 *                       Copyright (c) 				*
 *               Digital Equipment Corporation			*
 *                   Maynard, Massachusetts 			*
 *                         1985, 1986 				*
 *                    All rights reserved. 			*
 *								*
 *        The Information in this software is subject to change *
 *   without notice and should not be construed as a commitment *
 *   by  Digital  Equipment  Corporation.   Digital   makes  no *
 *   representations about the suitability of this software for *
 *   any purpose.  It is supplied "As Is" without expressed  or *
 *   implied  warranty. 					*
 *								*
 *        If the Regents of the University of California or its *
 *   licensees modify the software in a manner creating  	*
 *   diriviative copyright rights, appropriate copyright  	*
 *   legends may be placed on  the drivative work in addition   *
 *   to that set forth above. 					*
 *								*
 ****************************************************************/

/*
 * TMSCP registers and structures
 */
 
struct tmscpdevice {
	short	tmscpip;	/* initialization and polling */
	short	tmscpsa;	/* status and address */
};
 
#define	TMSCP_ERR	0100000	/* error bit */
#define	TMSCP_STEP4	0040000	/* step 4 has started */
#define	TMSCP_STEP3	0020000	/* step 3 has started */
#define	TMSCP_STEP2	0010000	/* step 2 has started */
#define	TMSCP_STEP1	0004000	/* step 1 has started */
#define	TMSCP_NV	0002000	/* no host settable interrupt vector */
#define	TMSCP_QB	0001000	/* controller supports Q22 bus */
#define	TMSCP_DI	0000400	/* controller implements diagnostics */
#define	TMSCP_OD	0000200	/* port allows odd host addr's in the buffer descriptor */
#define	TMSCP_IE	0000200	/* interrupt enable */
#define	TMSCP_MP	0000100	/* port supports address mapping */
#define	TMSCP_LF	0000002	/* host requests last fail response packet */
#define	TMSCP_PI	0000001	/* host requests adapter purge interrupts */
#define	TMSCP_GO	0000001	/* start operation, after init */
 
typedef struct {		/* swap shorts for TMSCP controller */
	short	lsh;
	short	hsh;
	} Trl;
 
/*
 * TMSCP Communications Area
 */

/* 
 * These defines were moved here so they could be shared between the
 * driver and the crash dump module.
*/
#ifndef	NRSPL2
#define	NRSPL2	3	/* log2 number of response packets */
#endif
#ifndef	NCMDL2
#define	NCMDL2	3	/* log2 number of command packets */
#endif

#define	NRSP	(1<<NRSPL2)
#define	NCMD	(1<<NCMDL2)
#define	RINGBASE	(4 * sizeof (short))
/* Size to map in when mapping a controller's command packet area */
#define	MAPBUFDESC	(((btoc(sizeof (struct tmscp)) - 1) << 8) | RW)
 
struct tmscpca {
	short	ca_xxx1;	/* unused */
	char	ca_xxx2;	/* unused */
	char	ca_bdp;		/* BDP to purge */
	short	ca_cmdint;	/* command queue transition interrupt flag */
	short	ca_rspint;	/* response queue transition interrupt flag */
	Trl	ca_rspdsc[NRSP];/* response descriptors */
	Trl	ca_cmddsc[NCMD];/* command descriptors */
};
 
#define	ca_ringbase	ca_rspdsc[0]
 
#define	TMSCP_OWN	0x8000	/* port owns descriptor (else host owns it) */
#define	TMSCP_INT	0x4000	/* allow interrupt on ring transition */
