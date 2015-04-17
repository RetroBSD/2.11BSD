/*	@(#)tmscp.h	1.2 (2.11BSD) 1998/2/23 */

/****************************************************************
 *                                                              *
 *        Licensed from Digital Equipment Corporation           *
 *                       Copyright (c)                          *
 *               Digital Equipment Corporation                  *
 *                   Maynard, Massachusetts                     *
 *                         1985, 1986                           *
 *                    All rights reserved.                      *
 *                                                              *
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
 ****************************************************************
 *
 * Modification history:
 *
 * 23-Feb-98 - sms
 *	The version number and revision history were accidentally omitted
 *	when update #401 was prepared (29-Jan-98).
 *
 *	Rearrange and clarify tmscp packet structure.  It was apparently
 *	possible for the 'mscp_dscptr' field to be overwritten with status
 *	information by the controller.  The 'm_filler' field was not used,
 *	no longer needed and in fact was just wasting 6 bytes of space.  It
 *	was removed.  The per controller structure is now 1864 bytes instead
 *	of 1896.
 *
 * 12-Dec-95 - sms
 *	Begin process of moving definitions common to MSCP and TMSCP into
 *	a different include file.  Define additional flags for the (heavily)
 *	revised TMSCP driver.
 *
 * 18-Oct-85 - afd
 *	Added: defines for tape format (density) flag values.
 *
 * 18-Jul-85 - afd
 *	Added: #define	M_UF_WBKNV	0000100
 *		for write back (which enables cache).
 ************************************************************************/
 
 
#include <machine/mscp_common.h>

/*
 * A TMSCP packet. 
 *
 * WARNING!  If this structure grows be sure to change the '1864' in the
 * file pdp/machdep2.c!!!
 */
 
struct mscp {
	struct	mscp_header mscp_header;/* device specific header */
	u_short	mscp_cmdref;		/* command reference number */
	u_short	m_xxx0;			/* filler */
	short	mscp_unit;		/* unit number */
	short	m_xxx1;			/* unused */
	u_char	mscp_opcode;		/* opcode */
	u_char	mscp_flags;		/* end message flags */
	short	mscp_modifier;		/* modifiers */
	union {
	char	FILLER[94];		/* sizeof (mslg) after 16 byte header */
	struct {
		u_short	bytecnt;	/* byte count (low order) */
		u_short	zzz2;		/* 64kb max for pdp-11 (high order) */
		u_short	buf_l;		/* buffer descriptor low word */
		u_short	buf_h;		/* buffer descriptor high word */
		long	xxx2[2];	/* unused */
		u_short	lbn_l;		/* logical block number low word */
		u_short	lbn_h;		/* logical block number high word */
	} gen;
	struct {
		short	version;	/* MSCP version */
		short	cntflgs;	/* controller flags */
		short	hsttmo;		/* host timeout */
		short	usefrac;	/* use fraction */
		u_long	time[2];	/* time and date */
		long	cntdep;		/* controller dependent parameters */
	} scc;
	struct {
		short	multunt;	/* multi-unit code */
		short	unitflgs;	/* unit flags */
		long	hostid;		/* host identifier */
		u_long	unitid[2];	/* unit identifier */
		long	mediaid;	/* media type identifier */
		short	format;		/* format (tape density) */
		short	speed;		/* tape speed = (ips * bpi) /1000 */
		short	fmtmenu;	/* format menu */
		u_short	maxwtrec;	/* max write byte count */
		u_short	noiserec;	/* max noise record size */
		u_short	pad;		/* reserved */
	} gtu;
/*
 * Reposition end message.  Note:  the shorts are not swapped in any
 * of the longs.
*/
	struct	{
		u_long	rcskiped;	/* records skipped */
		u_long	tmskiped;	/* tapemarks skipped */
		u_long	pad[2];		/* not used */
		u_long	position;	/* tape position */
	} rep_em;
	} un;
	long	*mscp_dscptr;	/* pointer to descriptor (software) */
};
 
/*
 * generic packet
 */
 
#define mscp_zzz2	un.gen.zzz2
#define	mscp_bytecnt	un.gen.bytecnt
#define	mscp_buffer_h	un.gen.buf_h
#define	mscp_buffer_l	un.gen.buf_l
#define	mscp_lbn_h	un.gen.lbn_h
#define	mscp_lbn_l	un.gen.lbn_l
#define	mscp_status	mscp_modifier
#define	mscp_endcode	mscp_opcode
#define	mscp_position	un.rep_em.position
 
/*
 * Abort / Get Command Status packet
 */
 
#define	mscp_outref	mscp_bytecnt
 
/*
 * Set Controller Characteristics packet
 */
 
#define	mscp_version	un.scc.version
#define	mscp_cntflgs	un.scc.cntflgs
#define	mscp_hsttmo	un.scc.hsttmo
#define	mscp_time	un.scc.time
#define	mscp_cntdep	un.scc.cntdep
 
/*
 * Reposition command packet fields
 */
 
#define mscp_reccnt mscp_bytecnt	/* record/object count */
#define mscp_tmkcnt mscp_buffer_l	/* tape mark count */
 
/*
 * Get Unit Status end packet
 */
 
#define	mscp_multunt	un.gtu.multunt
#define	mscp_unitflgs	un.gtu.unitflgs
#define	mscp_hostid	un.gtu.hostid
#define	mscp_unitid	un.gtu.unitid
#define	mscp_mediaid	un.gtu.mediaid
#define	mscp_format	un.gtu.format /* density:0=high */
#define	mscp_speed	un.gtu.speed  /* (ips*bpi)/1000 */
#define	mscp_fmtmenu	un.gtu.fmtmenu
 
/*
 * Set Controller Characteristics end packet
 */
 
#define	mscp_cnttmo	mscp_hsttmo	/* controller timeout */
#define	mscp_cntcmdl	mscp_usefrac	/* controller soft & hardware version */
#define	mscp_cntid	mscp_unitid	/* controller id */
 
/*
 * MSCP Error Log packet
 *
 *	NOTE: MSCP packet must be padded to this size.
 */
 
struct mslg {
	struct	mscp_header mslg_header;/* device specific header */
	long	mslg_cmdref;		/* command reference number */
	short	mslg_unit;		/* unit number */
	short	mslg_seqnum;		/* sequence number */
	u_char	mslg_format;		/* format */
	u_char	mslg_flags;		/* error log message flags */
	short	mslg_event;		/* event code */
	u_char	me_cntid[8];		/* controller id */
	u_char	me_cntsvr;		/* controller software version */
	u_char	me_cnthvr;		/* controller hardware version */
	short	mslg_multunt;		/* multi-unit code */
	u_long	me_unitid[2];		/* unit id */
	u_char	me_unitsvr;		/* unit software version */
	u_char	me_unithvr;		/* unit hardware version */
	short	mslg_group;		/* group; retry + level */
	long	mslg_position;		/* position (object count) */
	u_char	me_fmtsvr;		/* formatter software version */
	u_char	me_fmthvr;		/* formatter hardware version */
	short	mslg_xxx2;		/* unused */
	char	mslg_stiunsucc[62];	/* STI status information */
};
 
#define	mslg_busaddr	me_unitid.val[0]
#define	mslg_sdecyl	mslg_group

/*
 * These definitions were moved here where they could be included by
 * both the main driver and the tape crash dump module.
*/

/*
 * Per controller information structure.
 */
struct tmscp_softc {
	struct	tmscpdevice *sc_addr;	/* controller CSR address */
	short   sc_state;       /* state of controller */
	short	sc_ivec;        /* interrupt vector address */
	short	sc_unit;	/* CONTROLLER number - NOT drive unit # */
	short   sc_credits;     /* transfer credits */
	short   sc_lastcmd;     /* pointer into command ring */
	short   sc_lastrsp;     /* pointer into response ring */
	struct	buf sc_cmdbuf;	/* internal command buffer */
	struct	buf sc_ctab;	/* controller queue */
	struct	buf sc_wtab;	/* I/O wait queue for controller */
	struct	tmscp *sc_com;	/* communications area pointer */
	struct	tms_info *sc_drives[4];	/* pointers to per drive info */
};

/*
 * The TMSCP packet.  This is the same as MSCP except for the leading 't'
 * in the structure member names.  Eventually the two drivers will use a
 * single definition.
*/
struct tmscp {
	struct tmscpca	tmscp_ca;         /* communications area */
	struct mscp	tmscp_rsp[NRSP];  /* response packets */
	struct mscp	tmscp_cmd[NCMD];  /* command packets */
};					  /* 1864 bytes per controller! */

/*
 * Per drive information structure.
*/
struct tms_info {
	long		tms_type;	/* Drive type field  */
	int		tms_resid;	/* residual from last xfer */
	u_char		tms_endcode;	/* last command endcode */
	u_char		tms_flags;	/* flags visible to user programs */
	u_short		tms_status;	/* Command status from last command */
	u_short		Tflags;		/* Internal driver flags */
	short		tms_fmtmenu;	/* the unit's format (density) menu */
	short		tms_unitflgs;	/* unit flag parameters */
	short		tms_format;	/* unit's current format (density) */
	long		tms_position;	/* Drive position */
	struct	buf	tms_dtab;	/* I/O tape drive queues */
};

/* Bits in minor device */
#define	TMSUNIT(dev)	(minor(dev)&03)
#define	TMSCTLR(dev)	((minor(dev) >> 6) & 3)
#define	TMSDENS(dev)	((minor(dev) >> 3) & 3)
#define	FMTMASK		(M_TF_800|M_TF_PE|M_TF_GCR)	/* = 7 */

#define	T_NOREWIND	04
