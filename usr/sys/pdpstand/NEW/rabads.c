/*
 * SCCSID: @(#)rabads.c	3.3	12/30/87
 */
/*
 *		LICENSED FROM DIGITAL EQUIPMENT CORPORATION
 *			       COPYRIGHT (c)
 *		       DIGITAL EQUIPMENT CORPORATION
 *			  MAYNARD, MASSACHUSETTS
 *			      1985, 1986, 1987
 *			   ALL RIGHTS RESERVED
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
 *	NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
 *	EQUIPMENT CORPORATION.  DIGITAL MAKES NO REPRESENTATIONS ABOUT
 *	THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  IT IS
 *	SUPPLIED "AS IS" WITHOUT EXPRESSED OR IMPLIED WARRANTY.
 *
 *	IF THE REGENTS OF THE UNIVERSITY OF CALIFORNIA OR ITS LICENSEES
 *	MODIFY THE SOFTWARE IN A MANNER CREATING DERIVATIVE COPYRIGHT
 *	RIGHTS, APPROPRIATE COPYRIGHT LEGENDS MAY BE PLACED ON THE
 *	DERIVATIVE WORK IN ADDITION TO THAT SET FORTH ABOVE.
 */

/*
 * ULTRIX-11 stand-alone MSCP Disk Initialization
 *
 * Fred Canter
 *	For RX33/RX50, reads all blocks and flags any bad ones.
 *	For RD31/RD32/RD51/RD52/RD53/RD54 reads all blocks and rewrites any bad
 *	blocks to force controller initiated replacement.
 *	For RA60/RA80/RA81, reads all blocks and uses host initiated
 *	replacement to revector any bad blocks.
 *
 * SECRET STUFF:
 *
 *	This program has the following undocumented commands/features,
 *	used mainly for debugging:
 *
 * 1.	The "u" command is commented out. This was an unsuccessful
 *	attempt to unreplace a block. I soon discovered that could
 *	not be done.
 *
 * 2.	The "t all" feature of the table command causes all entries
 *	in the RCT to be printed, not just the ones that represent
 *	replaced blocks. Also allows RCT copy # selection.
 *
 * 3.	The "f" command (requires a simple password) causes the
 *	specified block to be written with the "force error"
 *	modifier. This is used for debugging.
 *
 * NOTE:
 *	This program consists, for the most part, of straight-line
 *	unoptimized code! Its only saving grace is that it works, at
 *	least as far as I can tell so far! -- Fred Canter 8/5/84
 *
 */

#include "sa_defs.h"
#include <sys/param.h>
#include <sys/inode.h>
#include "saio.h"
#include "ra_saio.h"

/*#define	DEBUG	1	/* Include conditional code for debugging */

#define	RD	0
#define	WRT	1
#define	RW	2
#define	ERR	0
#define	NOERR	1
#define	YES	1
#define	NO	0
#define	HELP	2

/*
 * Bad block replacement strategy flags
 */
#define	BB_COOP	0	/* Use MSCP cooperative replacement */
#define	BB_RWRT	1	/* (rd51/rd52) - rewriting a block replaces it */
#define	BB_NONE	2	/* RX33/RX50 - no bad block replacement */

struct dkinfo {
	char	*di_type;	/* type name of disk */
	char	*di_name;	/* ULTRIX-11 disk name */
	daddr_t	di_size;	/* size of entire volume in blocks */
	int	di_chunk;	/* number of sectors per read */
	int	di_flag;	/* replacement strategy to use */
				/* if set, just rewrite block to replace it. */
} dkinfo[] = {
	"ra60",	"ra",	400176L,	32, BB_COOP,
	"ra80",	"ra",	236964L,	31, BB_COOP,
	"ra81",	"ra",	891072L,	32, BB_COOP,
	"rx33",	"rx",	2400L,		10, BB_NONE,
	"rx50",	"rx",	800L,		10, BB_NONE,
	"rd31",	"rd",	41560L,		20, BB_RWRT,
	"rd32",	"rd",	83204L,		20, BB_RWRT,
	"rd51",	"rd",	21600L,		18, BB_RWRT,
	"rd52",	"rd",	60480L,		18, BB_RWRT,
	"rd53",	"rd",	138672L,	18, BB_RWRT, /* RQDX3 NOT OPTIMUM */
	"rd54",	"rd",	311200L,	20, BB_RWRT, /* RQDX3 NOT OPTIMUM */
	"rc25",	"rc",	50902L,		31, BB_COOP,
	0,
};

char	buf[512*32];	/* disk read and replacement process buffer */

/*
 * Bad block replacement process definitions.
 */

				/* Command Modifiers */
#define	MD_CMP	040000		/* Compare */
#define	MD_ERR	010000		/* Force Error */
#define	MD_SEC	01000		/* Suppress Error Correction */
#define MD_SREC	0400		/* Suppress Error Recovery */
#define	MD_PRI	01		/* Primary replacement block */

				/* End Message Flags */
#define	EF_BBR	0200		/* Bad block Reported */
#define	EF_BBU	0100		/* Bad Block Unreported */

				/* Status or Event Codes */
#define	ST_MSK	037		/* Status/event code mask */
#define	ST_SUC	0		/* Success (includes subcode "normal") */
#define	ST_DAT	010		/* Data Error */
#define MD_HCE	0110		/* Header Compare Error */

				/* Revector Control Info flags */
#define	RCI_VP	02		/* Volume write protect flag */
#define	RCI_FE	0200		/* Forced error flag */
#define	RCI_BR	020000		/* Bad RBN flag */
#define	RCI_P2	040000		/* Phase 2 flag */
#define	RCI_P1	0100000		/* Phase 1 flag */

				/* Block read check return values */
#define	BR_NBBR	0		/* no bad block reported */
#define	BR_BBR	1		/* bad block reported */
#define	BR_CECC	2		/* correctable ECC error */
#define	BR_UECC	3		/* uncorrectable ECC error */
#define	BR_NDE	4		/* non data error */
#define	BR_FEM	5		/* block written with "force error" modifier */
#define	BR_DEB	6		/* data error with bad block reported */

/*
 * Variables used by the replacement process.
 */

#ifdef	DEBUG
int	rp_bps;			/* interrupt replacement process at step ? */
#endif DEBUG

daddr_t	rp_lbn;			/* LBN of logical block being replaced */
daddr_t	rp_rbn;			/* RBN (Replacement Block Number) */
int	rp_didit;		/* Says block actually replaced */
int	rp_hblk;		/* RCT block of primary RBN descriptor */
int	rp_hoff;		/* RCT block offset of primary RBN descriptor */
int	rp_blk;			/* Current RCT block being searched */
int	rp_off;			/* Current RCT block offset */
int	rp_oblk;		/* RCT block number of OLD RBN */
int	rp_ooff;		/* RCT block offset of OLD RBN */
int	rp_delta;		/* RCT block search offset delta (ping-pong) */

int	rp_irs;			/* Initial read of bad LBN successful */
int	rp_wcs7;		/* Step 7, write-compare sucessful */
int	rp_wcs8;		/* Step 8, write-compare sucessful */
int	rp_prev;		/* LBN was previously replaced */
daddr_t	rp_orbn;		/* If previously replaced, OLD RBN */
daddr_t rp_orbd;		/* old RBN descriptor, in case replace fsils */
int	rp_pri;			/* RBN is primary replacement block */
int	rp_sec;			/* RBN is non-primary replacement block */
int	rp_l;			/* Number of LBNs per track */
int	rp_r;			/* Number of RBNs per track */
int	rp_dirty;		/* Phase (P1 | P2) of interrupted replacement */

/*
 * Replacement process buffer pointers,
 * segment the read/write buffer for use
 * by the replacement process.
 */

struct rct_rci *bp_rci = &buf[0]; /* RCT blk 0 - replacement cntrl info */
char *bp_ir = &buf[512];	  /* RCT blk 1 - image of bad LBN */
union rctb *bp_rct1 = &buf[1024]; /* RCT blk F - first block being searched */
union rctb *bp_rct2 = &buf[1536]; /* RCT blk S - second block (if needed) */
int *bp_tp = &buf[2048];	  /* test data pattern ( 0165555 0133333 ) */
char *bp_tmp = &buf[2560];	  /* scratch buffer */

/*
 * Structure of RCT (Revector Control Table) block 0.
 * Used during bad block replacement.
 */

struct	rct_rci {
	short	rt_vsn[4];	/* volume serial number */
	short	rt_stat;	/* replacement process status word */
	short	rt_rsvd;	/* reserved ??? */
	union {			/* LBN being replaced */
		short	ri_lbnw[2];
		daddr_t	ri_lbnl;
	} rt_lbn;
	union {			/* RBN - replacement block number */
		short	ri_rbnw[2];
		daddr_t	ri_rbnl;
	} rt_rbn;
	union {			/* bad RBN - RBN being replaced */
		short	ri_brbw[2];
		daddr_t	ri_brbl;
	} rt_brb;
	short	rt_pad[244];	/* bad to 512 byte block */
};

/*
 * Structure used to access an RCT block
 * containing 128 RBN descriptors. The
 * order (swap hi & lo words) must be reversed
 * because of the way the V7 C compiler stores longs.
 */

union	rctb {
	daddr_t	rbnd_l;		/* Access descriptor as a long */
	short	rbnd_w[2];	/* Access descriptor as two shorts */
};

/*
 * External variables found in the MSCP disk driver (ra.c).
 * Used to modifier driver operation for bad block
 * replacement and get error status info from the driver.
 */

#define	RP_WRT	1
#define	RP_RD	2
#define	RP_REP	3
#define RP_AC	4

extern	union {			/* RABADS RBN for ra.c */
	daddr_t	ra_rbnl;
	short	ra_rbnw[2];
} ra_rbn;
extern int	ra_badc;	/* RABADS command flag in ra.c */
extern int	ra_badm;	/* RABADS command modifier in ra.c */
extern int	ra_ctid[];	/* MSCP controller type ID */
extern char	*ra_dct[];	/* Controller type name string */
extern int	ra_stat[];	/* MSCP status/event code */
extern int	ra_ecode[];	/* MSCP end code */
extern int	ra_eflags[];	/* MSCP end message flags */
extern struct	ra_drv	ra_drv[3][4];	/* Drive type info */

struct dkinfo *dip;
daddr_t	sbn, us, nblk, nbc, rbn, nb;
daddr_t	bn;
int	repcnt, badcnt, rsize;
int fd, rcnt;
int	ask;		/* flag so table doesn't ask for a return */
int	allflag;
int	rwfem;
int	force;
long	atol();
int	cn;
int	unit;
char	line[50];
char	fn[30];		/* file spec i.e., hp(0,0) */
char	cmd[20];	/* command name */
char	dt[20];		/* disk type rm03, etc */
char	dn[2];		/* drive number */
int	firstblk;	/* flag for linear rct searching */
int	off_bottom;	/* descriptor in RCT greater that 127 */
int	off_top;	/* descriptor in RCT less than zero */
int	first_level;	/* have completed first level search of the RCT block */
int	nondataerr;	/* got a non-data error reading the block in step 7 */
int	bbr_recur;	/* BBR recursion counter */

char	*help[] =
{
	"",
	"To correct typing mistakes, press <DELETE> to erase one character",
	"or <CTRL/U> to erase the entire line.",
	"",
	"To execute a command, type the first letter of the command then",
	"press <RETURN>. The program may prompt for additional information.",
	"",
	"The valid RABADS commands are:",
	"",
	"help    - Print this help message.",
	"",
	"exit    - Exit from the RABADS program.",
	"",
	"drives  - List the disks that can be initialized with RABADS.",
	"",
	"status  - Print the status and geometry of the specified disk.",
	"",
	"table   - Print the RCT (Revector Control Table) for a disk.",
	"",
	"init    - Do a read scan of the disk and replace any bad blocks.",
	"",
	"replace - Force replacement of a disk block.",
	"",
	0
};

char	*fewarn[] =
{
	"",
	"The block was written with the \"force error\" modifier, that is,",
	"when the block is read report an error even if the read succeeds.",
	"This indicates that the block has already been replaced but that",
	"valid data could not be recovered from the bad block before it",
	"was replaced. This may also indicate that a previous attempt to",
	"replace the block failed for some reason. In either case, the",
	"recommended procedure is to rewrite the block then reread it and",
	"only replace the block if the reread fails.",
	"",
	"		    ****** CAUTION ******",
	"",
	"Rewriting the block will clear the \"forced error\", however, the",
	"data in the block must be considered to have been corrupted. This",
	"is because valid data could not be read from the block before it",
	"was replaced.",
	"",
	0
};

char	*bdwarn =
  "\n\n\7\7\7DISK SHOULD NOT BE USED: \
  \n    (has unreplaced bad blocks or uncleared \"forced error\" blocks!)\n";
char	*pwstr = "qwerty";
char	*feb_nc1 =
  "was written with \"forced error\" - NOT CLEARED!\n";
char	*feb_nc2 =
  "(Data in this block must considered invalid!)\n";
char	*ndemsg =
  "(NON DATA ERROR: suspect faulty disk hardware or bad media!)\n";
char	*rb_uecc =
  "(Uncorrectable ECC error in replacement block!)\n";
char	*rb_bad =
  "(Hardware says replacement block bad, retry disk initialization!)\n";

extern int argflag;	/* 0=interactive, 1=called by sdload */
int	rctcopy;

main()
{
	struct rct_rci *rtp;
	register int i, j;
	register char *p;
	int s, cnt, k;
	int found, rctend, rctrw;
	long	*lp;

	printf("\n\nULTRIX-11 MSCP Disk Initialization Program\n");
	if(argflag) { 	/* called by SDLOAD, can only do init command */
		cmd[0] = 'i';
		cmd[1] = '\0';
		goto do_i;
	}
retry:
	ask = 1;
	force = 0;
	allflag = 0;
	rctcopy = 0;
	printf("\nrabads <help exit drives status table init replace>: ");
	gets(cmd);
	switch(cmd[0]) {
	case 'f':
		goto do_f;
	case 'h':
		for(i=0; help[i]; i++)
			printf("\n%s", help[i]);
		goto retry;
	case 'e':
		exit(NORMAL);
	case 'd':
		prtdsk();
		goto retry;
	case 's':
		goto do_s;
	case 't':
		if(strcmp("t all", cmd) == 0)
			allflag++;	/* print entire RCT */
		if(strcmp("t go", cmd) == 0){
			ask = 0;	/* don't stop to ask */
			allflag++;	/* print entire RCT */
		}
		goto do_t;
	case 'r':
		goto do_r;
	case 'i':
		goto do_i;
	case 0:
		goto retry;
	default:
		printf("\n(%s) - not a valid command!\n", cmd);
		goto retry;
	}
do_s:
	if(dipset() < 0)
		goto retry;
	dskopen(NOERR, RD);
	if(fd >= 0)
		close(fd);
	printf("\nMSCP controller: %s at micro-code revision %d",
		ra_dct[cn], ra_ctid[cn] & 017);
	printf("\nUnit %d: ", unit);
	switch(ra_drv[cn][unit].ra_dt) {
	case RC25:
		printf("RC25");
		break;
	case RX33:
		printf("RX33");
		break;
	case RX50:
		printf("RX50");
		break;
	case RD31:
		printf("RD31");
		break;
	case RD32:
		printf("RD32");
		break;
	case RD51:
		printf("RD51");
		break;
	case RD52:
		printf("RD52");
		break;
	case RD53:
		printf("RD53");
		break;
	case RD54:
		printf("RD54");
		break;
	case RA60:
		printf("RA60");
		break;
	case RA80:
		printf("RA80");	
		break;
	case RA81:
		printf("RA81");
		break;
	default:
		printf("UKNOWN or NONEXISTENT");
		break;
	}
	printf(" status =");
	if(ra_drv[cn][unit].ra_online)
		printf(" ONLINE");
	else
		printf(" OFFLINE");
	printf("\nLogical blocks per track\t= %d",
		ra_drv[cn][unit].ra_trksz);
	printf("\nReplacement blocks per track\t= %d",
		ra_drv[cn][unit].ra_rbns);
	printf("\nNumber of RCT copies\t\t= %d", ra_drv[cn][unit].ra_ncopy);
	printf("\nSize of each RCT copy in blocks\t= %d",
		ra_drv[cn][unit].ra_rctsz);
	printf("\nHost area size in blocks\t= %D\n",
		ra_drv[cn][unit].d_un.ra_dsize);
	goto retry;
do_u:		/* semi-secret stuff (unreplace block(s)) */
/*
 *******************************************************************************
	printf("\nPassword: ");
	gets(line);
	if(strcmp("farkle", line) != 0) {
		printf("\nReserved for Digital!\n");
		goto retry;
	}
	if(dipset() < 0)
		goto retry;
	dskopen(NOERR, RW);
	if(fd < 0) {
		printf("\nCan't open %s!\n", fn);
		goto retry;
	}
	if(rp_dirty) {
		printf("\nUnreplace aborted!\n");
		goto do_u_x;
	}
	s = ra_drv[cn][unit].ra_rctsz;
	if(s == 0) {
		printf("\nDisk does not have a Revector Control Table!");
		goto do_u_x;
	}
do_u_bn:
	if(allflag == 0) {
		printf("\nBlock number: ");
		gets(line);
		if(line[0] == '\0')
			goto do_u_bn;
		bn = atol(line);
		if((bn < 0) || (bn >= ra_drv[cn][unit].d_un.ra_dsize))
			goto do_u_bn;
	}
	rctend = 0;
	rctrw = 0;
	for(i=2; i<s; i++) {
		if(rctmcr(i, bp_rct1) < 0) {
			printf("\nRCT multicopy read failed!\n");
			goto do_u_x;
		}
		found = 0;
		for(j=0; j<128; j++) {
			k = (bp_rct1[j].rbnd_w[0] >> 12) & 017;
			switch(k) {
			case 02:
			case 03:
				if(allflag) {
					found++;
					bp_rct1[j].rbnd_l = 0L;
					break;
				}
				if(bn == (bp_rct1[j].rbnd_l & ~036000000000)) {
					found++;
					bp_rct1[j].rbnd_l = 0L;
				}
				break;
			case 07:
				if(((ra_ctid[cn] >> 4) & 017) == RQDX1) {
					rctend++;
					break;
				} else
					continue;
			case 010:
				rctend++;
				break;
			default:
				continue;
			}
		}
		if(found) {
			if(rctmcw(i, bp_rct1) < 0) {
				printf("\nPANIC: RCT multicopy write failed!\n");
				exit(FATAL);
			}
			rctrw++;
		}
		if(rctend) {
			if(rctrw == 0)
				printf("\nBlock %D was not revectored!\n", bn);
			break;
		}
	}
 *******************************************************************************
 */
do_u_x:
	close(fd);
	printf("\n");
	goto retry;
do_r:
	if(dipset() < 0)
		goto retry;
	if(dip->di_flag == BB_NONE) {
		printf("\nCannot replace blocks on an %s disk!\n",
			dip->di_type);
		goto retry;
	}
	dskopen(NOERR, RW);
	if(fd < 0) {
		printf("\nCan't open %s!\n", fn);
		goto retry;
	}
	if(rp_dirty)
		goto rp_s1;	/* finish up interrupted replacement */
	if(ra_drv[cn][unit].ra_rctsz == 0) {
		printf("\nDisk does not have a Revector Control Table!");
		goto do_u_x;
	}
do_r_bn:
	printf("\nBlock number: ");
	gets(line);
	if(line[0] == '\0')
		goto do_r_bn;
	bn = atol(line);
	if((bn < 0) || (bn >= ra_drv[cn][unit].d_un.ra_dsize))
		goto do_r_bn;
	if(dip->di_flag == BB_RWRT) {	/* cntlr will replace block if bad */
		lseek(fd, (long)(bn*512), 0);
		ra_badc = RP_RD;
		read(fd, (char *)&buf, 512);
	}
	printf("\nBlock %D read check: ", bn);
	s = brchk(bn);	/* read check block - see if really bad */
	if((s == BR_BBR) || (s == BR_NBBR))
		printf("SUCCEEDED - ");
	else
		printf("FAILED - ");
	switch(s) {
	case BR_BBR:
	case BR_DEB:
		printf("bad block reported\n");
		break;
	case BR_NBBR:
		printf("no bad block reported\n");
		if(dip->di_flag == BB_RWRT) {
			printf("\n\7\7\7Disk controller replaces bad blocks,");
			printf(" no way to force replacement!\n");
			goto do_u_x;
		}
		printf("\nReally replace this block <y or n> ? ");
		if(yes(0) == YES)
			break;
		else
			goto do_u_x;
	case BR_CECC:
	case BR_UECC:
		printf("data error\n");
		break;
	case BR_NDE:
		printf("non data error\n");
		printf("\n** - block connot be replaced!\n");
		goto do_u_x;
	case BR_FEM:
		printf("block written with \"force error\" modifier\n");
		for(i=0; fewarn[i]; i++)
			printf("\n%s", fewarn[i]);
		printf("\nRewrite the block <y or n> ? ");
		if(yes(0) == NO)
			goto do_u_x;
		lseek(fd, (long)(bn*512), 0);
		ra_badc = RP_WRT;
		if(write(fd, (char *)bp_tmp, 512) != 512) {
			printf("\n\7\7\7BLOCK REWRITE FAILED!\n");
			goto do_u_x;
		}
		lseek(fd, (long)(bn*512), 0);
		ra_badc = RP_RD;
		if(dip->di_flag == BB_RWRT)
			goto do_r_rq;
		if(read(fd, (char *)bp_tmp, 512) == 512)
			printf("\nREREAD SUCCEEDED: block not replaced!\n");
		else
			printf("\nREREAD FAILED: retry the replace command!\n");
		goto do_u_x;
do_r_rq:
		if(read(fd, (char *)bp_tmp, 512) != 512) {
			printf("\n\7\7\7Disk controller failed ");
			printf("to replace bad block!\n");
		} else
			printf("\nDisk controller replaced block!\n");
		goto do_u_x;
	}
	if(dip->di_flag == BB_COOP) {
		printf("\nBLOCK: %D - replacement ", bn);
		force++;	/* force replacement (see rp_s8:) */
		rp_lbn = bn;
		goto rp_s1;
	}
	goto do_u_x;
do_f:
	printf("\nPassword: ");
	gets(line);
	if(strcmp(line, pwstr) != 0) {
		printf("\nSorry - reserved for Digital!\n");
		goto retry;
	}
	if(dipset() < 0)
		goto retry;
	if(dip->di_flag == BB_NONE) {
		printf("\nCannot replace blocks on an %s disk!\n",
			dip->di_type);
		goto retry;
	}
	dskopen(NOERR, RW);
	if(fd < 0) {
		printf("\nCan't open %s!\n", fn);
		goto retry;
	}
	if(rp_dirty) {
		printf("\nForce aborted!\n");
		goto do_u_x;
	}
	if(ra_drv[cn][unit].ra_rctsz == 0) {
		printf("\nDisk does not have a Revector Control Table!");
		goto do_u_x;
	}
do_f_bn:
	printf("\nBlock number: ");
	gets(line);
	if(line[0] == '\0')
		goto do_f_bn;
	bn = atol(line);
	if((bn < 0) || (bn >= ra_drv[cn][unit].d_un.ra_dsize))
		goto do_f_bn;
	lseek(fd, (long)(bn*512), 0);
	read(fd, buf, 512);
	lseek(fd, (long)(bn*512), 0);
	ra_badc = RP_WRT;
	ra_badm = 010000;	/* MD_ERR (force error) */
	write(fd, buf, 512);
	goto do_u_x;
do_t:
	if(dipset() < 0)
		goto retry;
	dskopen(NOERR, RD);
	if(fd < 0) {
		printf("\nCan't open %s!\n", fn);
		goto retry;
	}
	s = ra_drv[cn][unit].ra_rctsz;
	if(s == 0) {
		printf("\nDisk does not have a Revector Control Table!");
		goto do_t_x;
	}
	if(allflag == 0)	/* if "t all" ask which copy */
		goto do_t_n;
	printf("\nRCT copy < ");
	k = ra_drv[cn][unit].ra_ncopy;
	for(i=0; i<k; i++)
		printf("%d ", i+1);
	printf(">: ");
	gets(line);
	rctcopy = atoi(line);
	if((rctcopy < 0) || (rctcopy > k))
		rctcopy = 0;
	if(rctmcr(0, bp_rci) < 0)
		goto do_t_err;
	rtp = bp_rci;
	i = rtp->rt_lbn.ri_lbnw[0];	/* reverse order */
	rtp->rt_lbn.ri_lbnw[0] = rtp->rt_lbn.ri_lbnw[1];
	rtp->rt_lbn.ri_lbnw[1] = i;
	i = rtp->rt_rbn.ri_rbnw[0];	/* reverse order */
	rtp->rt_rbn.ri_rbnw[0] = rtp->rt_rbn.ri_rbnw[1];
	rtp->rt_rbn.ri_rbnw[1] = i;
	i = rtp->rt_brb.ri_brbw[0];	/* reverse order */
	rtp->rt_brb.ri_brbw[0] = rtp->rt_brb.ri_brbw[1];
	rtp->rt_brb.ri_brbw[1] = i;
	printf("\nRCT block 0: revector control information.\n");
	printf("\nStatus bits: P1=%d ", rtp->rt_stat&RCI_P1 ? 1 : 0);
	printf("P2=%d ", rtp->rt_stat&RCI_P2 ? 1 : 0);
	printf("BR=%d ", rtp->rt_stat&RCI_BR ? 1 : 0);
	printf("FE=%d ", rtp->rt_stat&RCI_FE ? 1 : 0);
	printf("VP=%d ", rtp->rt_stat&RCI_VP ? 1 : 0);
	printf("\nBad LBN\t= %D", rtp->rt_lbn.ri_lbnl);
	printf("\nRBN\t= %D", rtp->rt_rbn.ri_rbnl);
	printf("\nOld RBN\t= %D\n", rtp->rt_brb.ri_brbl);
	printf("\nOctal dump RCT block 1 <y or n> ? ");
	gets(line);
	if(line[0] != 'y')
		goto do_t_n;
	if(rctmcr(1, bp_ir) < 0)
		goto do_t_err;
	printf("\nRCT block 1 contents:\n");
	for(i=0; i<256; i++) {
		if((i%8) == 0)
			printf("\n    ");
		printf("%o\t", bp_ir[i]);
		if(i == 127)
			if (ask)
				if(prfm())
					goto do_t_x;
	}
	if (ask)
		if(prfm())
			goto do_t_x;
do_t_n:
	cnt = -1;
	for(i=2; i<s; i++) {
		if(rctmcr(i, bp_rct1) < 0) {
do_t_err:
			printf("\nCANNOT READ REVECTOR CONTROL TABLE: ");
			printf("RCT multicopy read failed!\n");
			goto do_t_x;
		}
		for(j=0; j<128; j++) {
			if((cnt == 16) || (cnt == -1)) {
				if(cnt == 16)
					if (ask)
						if(prfm())
							goto do_t_x;
				cnt = 0;
				printf("\nBlock\tOffset\tLBN\tCODE");
				printf("\n-----\t------\t---\t----");
			}
			k = (bp_rct1[j].rbnd_w[0] >> 12) & 017;
			switch(k) {
			case 02:
			case 03:
			case 04:
			case 05:
				break;
			case 07:
				if(((ra_ctid[cn] >> 4) & 017) == RQDX1) {
					if(allflag)
						break;
					else
						goto do_t_x;
				} else
					continue;
			case 010:
				goto do_t_x;
			default:
				if(allflag)
					break;
				else
					continue;
			}
			cnt++;
			printf("\n%d\t%d\t%D\t", i, j,
				bp_rct1[j].rbnd_l & ~036000000000);
			switch(k) {
			case 00:
				printf("Unallocated replacement block");
				break;
			case 02:
				printf("Allocated - primary RBN");
				break;
			case 03:
				printf("Allocated - non-primary RBN");
				break;
			case 04:
			case 05:
				printf("Unusable replacement block");
				break;
			default:
				printf("Unknown code (%o)", k);
				break;
			}
		}
	}
do_t_x:
	close(fd);
	printf("\n");
	goto retry;
do_i:
	if(dipset() < 0)
		goto retry;
	dskopen(ERR, RW);
	if(rp_dirty) {
		printf("\nInitialization aborted!\n");
		goto do_u_x;
	}
	us = ra_drv[cn][unit].d_un.ra_dsize;
	printf("\nStarting block number < 0 >: ");
	gets(line);
	if(line[0] == '\0')
		sbn = 0L;
	else {
		sbn = atol(line);
		if((sbn < 0) || (sbn >= us)) {
			printf("\nBad starting block!\n");
	do_i_ex:
			if(argflag)
				exit(FATAL);
			else
				goto retry;
		}
	}
	printf("\nNumber of blocks to check < %D >: ", us - sbn);
	gets(line);
	if(line[0] == '\0')
		nblk = us - sbn;
	else
		nblk = atol(line);
	if(nblk <= 0) {
		printf("\nBad # of blocks!\n");
		goto do_i_ex;
	}
	if(nblk > (us - sbn)) {
		nblk = (us - sbn);
		printf("\nToo many blocks, truncating to %D blocks!\n", nblk);
	}
do_i_rw:
	if(dip->di_flag != BB_NONE) {
	    if(argflag == 0) {
		printf("\nRewrite blocks written with \"forced error\" ");
		printf("(? for help) <y or n> ? ");
		rwfem = yes(HELP);
		if(rwfem == HELP) {
			for(i=0; fewarn[i]; i++)
				printf("\n%s", fewarn[i]);
			goto do_i_rw;
		}
	    } else
		rwfem = YES;
	}
	printf("\nREADING...\n");
	badcnt = 0;
	repcnt = 0;
	bn = sbn;
	nbc = 0;
do_i_lp:
	nb = dip->di_chunk;
	if((bn + nb) > us)
		nb = (us - bn);
	if((nb + nbc) > nblk)
		nb = nblk - nbc;
	rsize = nb * 512;
	lseek(fd, (long)(bn*512), 0);
	ra_badc = RP_AC;
	rcnt = read(fd, (char *)&buf, rsize);
	if(rcnt == rsize) {
do_i_lp1:
		bn += nb;
		nbc += nb;
		if(nbc >= nblk) {
bg_loop:
			close(fd);
			printf("\n\n%D blocks checked", nbc);
			printf("\n%d bad blocks found", badcnt);
			printf("\n%d bad blocks replaced\n", repcnt);
			if(argflag == 0) {
				if(repcnt != badcnt)
					printf("%s", bdwarn);
				else
					printf("\n");
				goto retry;
			}
			if(repcnt != badcnt)
				exit(HASBADS);
			while(getchar() != '\n') ;	/* wait for <RETURN> */
			exit(NORMAL);
		}
		goto do_i_lp;
	}
	for(rbn=bn; rbn<(bn+nb); rbn++) {
		lseek(fd, (long)(rbn*512), 0);
		ra_badc = RP_RD;
		rcnt = read(fd, (char *)&buf, 512);
		if(rcnt == 512)
			continue;
		badcnt++;
		printf("\nBLOCK: %D - ", rbn);
		switch(dip->di_flag) {	/* replacement strategy to use */
		case BB_COOP:
			if(ra_eflags[cn] & EF_BBR) {
				printf("replacement ");
				rp_lbn = rbn;
				goto rp_s1;	/* go replace block */
			} else if(ra_stat[cn] == ST_DAT) {	/* FEM */
				/* data error with force error modifier */
do_i_rs:
				if(!rwfem) { /* NOT REWRITING FORCED ERRORS */
					printf("%s", feb_nc1); /* warn bad data */
					printf("%s", feb_nc2);
				} else {
					if(fembrw(0, rbn) == YES)
						badcnt--;
				}
				break;
			} else {
				printf("(no bad block reported flag) ");
				printf("cannot be replaced!\n");
				deprnt();
				break;
			}
do_i_rg:			/* return here if replacement succeeded */
			repcnt++;
			printf("SUCCEEDED!\n");
			s = brchk(rbn);	/* read check replacement block */
			if(s == BR_NBBR)
				break;	/* every thing is cool! */
			if(s == BR_FEM) { /* replacement block - forced error */
				if(!rwfem) {
					printf("BLOCK: %D - ", rbn);
					printf("%s", feb_nc1);
					printf("%s", feb_nc2);
					repcnt--;
					break;
				}
				if(fembrw(1, rbn) == NO)
					repcnt--;
				break;
			}
			repcnt--;
			printf("BLOCK: %D - ", rbn);
			printf("replacement block read check FAILED!\n");
			switch(s) {
			case BR_UECC:
				printf("%s", rb_uecc);
				deprnt();
				break;
			case BR_BBR:
				printf("%s", rb_bad);
				/* BAD REPLACEMENT BLOCK ?????? */
				/* should go back and rescan */
				break;
			case BR_NDE:
			default:
				printf("%s", ndemsg);
				deprnt();
				break;
			}
			break;
do_i_rb:			/* return here if replacement failed */
			printf("\nBLOCK: %D - replacement FAILED!\n", rbn);
			break;
do_i_ra:			/* return here if replacement aborted */
				/* i.e., block was not really bad */
			printf("ABORTED (block not really bad)!\n");
			lseek(fd, (long)(rbn*512), 0);
			ra_badc = RP_RD;
			read(fd, (char *)&buf, 512);
			if(ra_stat[cn] != ST_DAT) {
				badcnt--;
				break;	/* NOT FORCED ERROR */
			}
			if(!rwfem) {
				printf("BLOCK: %D - ", rbn);
				printf("%s", feb_nc1);
				printf("%s", feb_nc2);
				break;
			}
			if(fembrw(1, rbn) == YES)
				badcnt--;
			break;
		case BB_NONE:
			printf("%s: bad blocks cannot be replaced!\n",
				dip->di_type);
			deprnt();
			break;
		case BB_RWRT:
			if(ra_eflags[cn] & EF_BBR) {
				printf("replacement ");
				switch(brchk(rbn)) {	/* read check */
				case BR_NBBR:
					repcnt++;
					printf("SUCCEEDED!\n");
					break;
				case BR_FEM:
					printf("SUCCEEDED!\n");
					repcnt++;
					if(!rwfem) {
						printf("BLOCK: %D - ", rbn);
						printf("%s", feb_nc1);
						printf("%s", feb_nc2);
						repcnt--;
						break;
					}
					if(fembrw(1, rbn) == NO)
						repcnt--;
					break;
				case BR_UECC:
					printf("FAILED!\n");
					printf("%s", rb_uecc);
					deprnt();
					break;
				case BR_BBR:
					printf("FAILED!\n");
					printf("%s", rb_bad);
					/* BAD REPLACEMENT BLOCK ?????? */
					/* should go back and rescan */
					break;
				case BR_NDE:
				default:
					printf("FAILED!\n");
					printf("%s", ndemsg);
					deprnt();
					break;
				}
			} else if(ra_stat[cn] == ST_DAT) { /* FEM */
				if(!rwfem) { /* NOT REWRITING FORCED ERRORS */
					printf("%s", feb_nc1); /* warn bad data */
					printf("%s", feb_nc2);
				} else {
					if(fembrw(0, rbn) == YES)
						badcnt--;
				}
			} else {
				printf("cannot be replaced!\n");
				printf("%s", ndemsg);
				deprnt();
			}
			break;
		}
	}
	goto do_i_lp1;

/*
 * MSCP: host initiated bad block replacement algorithm
 *
 * I allow myself one editorial comment before proceeding:
 *
 *	Fred Canter "Good GOD Gertrude, you must be kidding!"
 *
 * Each step of the bad block replacement procedure is labeled
 * so as to correspond to the steps in section 7.5 of the "DSA
 * DISK FORMAT SPECIFICATION" Version 1.4.0.
 */

rp_s1:
#ifdef	DEBUG
	if(rp_bps == 1)
		goto do_u_x;
#endif	DEBUG
/*
 * Step 1 is performed elsewhere in the program. Either the replacement
 * process is initilated by an error with the BAD BLOCK REPORTED flag set,
 * or the process is restarted because it was interrupted by a failure of
 * some type. The latter case is detected when the unit is prought online.
 */
	rp_didit = 0;	/* clear block actually replaced indicator */
			/* in case of interrupted replacement */
	rp_prev = rp_pri = rp_sec = 0;

rp_s2:
#ifdef	DEBUG
	if(rp_bps == 2)
		goto do_u_x;
#endif	DEBUG
/*
 * The DSA spec calls for a soft lock of the unit, but because replacement
 * is only done by this stand-alone program a lock is not necessary.
 */

bbr_recur = 0;		/* initialize the BBR recursion counter to 0 */

rp_s3:
#ifdef	DEBUG
	if(rp_bps == 3)
		goto do_u_x;
#endif	DEBUG
/*
 * The first two steps were easy, but I don't expect this trend to continue.
 */

	if(rp_dirty == 0)
		goto rp_s4;	/* NOT an interrupted replacement! */
/*
 * Interrupted replacement,
 * Re-establish software context for Phase 1
 * and Phase 2, if necessary.
 * Yes, I know that RCT block 0 was read in
 * during the dskopen()!
 */
	printf("\n\7\7\7COMPLETING INTERRUPTED BAD BLOCK REPLACEMENT");
	printf(" (Phase %d)...\n", rp_dirty);
	if(rctmcr(0, bp_rci) < 0) {	/* revector control info */
		rp_rcte(3, RD, 0);
		goto rp_s18;
	}
	if(rctmcr(1, bp_ir) < 0) {	/* bad LBN image */
		rp_rcte(3, RD, 1);
		goto rp_s18;
	}
	rtp = (struct rct_rci *)bp_rci;
	if(rtp->rt_stat & RCI_FE)	/* initial read sucessful or not */
		rp_irs = NO;
	else
		rp_irs = YES;
/*
 * The DSA spec says that the LBN is only valid
 * if the P1 bit is set, but I don't buy it!
 * I never clear the LBN until after phase 2 is
 * complete or a fatal error.
 */
	i = rtp->rt_lbn.ri_lbnw[0];	/* reverse order */
	rtp->rt_lbn.ri_lbnw[0] = rtp->rt_lbn.ri_lbnw[1];
	rtp->rt_lbn.ri_lbnw[1] = i;
	rp_lbn = rtp->rt_lbn.ri_lbnl;	/* bad block's LBN */
	printf("\nBLOCK: %D - replacement ", rp_lbn);
	if(rp_dirty == 1)
		goto rp_s7;	/* Interrupted during phase one */
	i = rtp->rt_rbn.ri_rbnw[0];	/* new RBN - reverse order */
	rtp->rt_rbn.ri_rbnw[0] = rtp->rt_rbn.ri_rbnw[1];
	rtp->rt_rbn.ri_rbnw[1] = i;
	rp_rbn = rtp->rt_rbn.ri_rbnl;
	i = rtp->rt_brb.ri_brbw[0];	/* old RBN - reverse order */
	rtp->rt_brb.ri_brbw[0] = rtp->rt_brb.ri_brbw[1];
	rtp->rt_brb.ri_brbw[1] = i;
	if(rtp->rt_stat & RCI_BR) {	/* previously replaced */
		rp_prev = 1;
		rp_orbn = rtp->rt_brb.ri_brbl;
	}
	rp_l = ra_drv[cn][unit].ra_trksz;	/* LBNs per track */
	rp_r = ra_drv[cn][unit].ra_rbns;	/* RBNs per track */
						/* Primary RBN descriptor */
	rp_hblk = (((rp_lbn / rp_l) * rp_r) / 128) + 2;
	rp_hoff = ((rp_lbn / rp_l) * rp_r) % 128;
	rp_blk = (rp_rbn / 128) + 2;		/* Actual RBN descriptor */
	rp_off = rp_rbn % 128;
	if((rp_blk == rp_hblk) && (rp_off == rp_hoff))	/* Primary RBN ? */
		rp_pri = 1;
	else
		rp_sec = 1;	/* YES, I know rp_sec not really used */
	if(rp_prev) {		/* set saved old RBN desc, as best we can */
		rp_orbd = rp_lbn;	/* LBN being replaced */
		rp_oblk = (rp_orbn / 128) + 2;
		rp_ooff = rp_orbn % 128;
		if((rp_oblk == rp_hblk) && (rp_ooff == rp_hoff))
			rp_orbd |= 04000000000;	/* primary RBN */
		else
			rp_orbd |= 06000000000;	/* non-primary RBN */
	}
	if(rctmcr(rp_blk, bp_rct1) < 0) {	/* First RCT block */
		rp_rcte(3, RD, rp_blk);
		goto rp_s18;
	}
	goto rp_s11;		/* finish up phase 2 */

rp_s4:
#ifdef	DEBUG
	if(rp_bps == 4)
		goto do_u_x;
#endif	DEBUG
printf("4 ");
/*
 * Read the suspected bad block, remember whether or not the read succeeded.
 * Also, remember if the forced error indicator was set. 
 */
	rp_irs = NO;
	p = bp_ir;
	for(i=0; i<512; i++)
		*p++ = 0;
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_RD;
	for (i=0; i < 4; i++)
		if(read(fd, bp_ir, 512) == 512) {
			rp_irs = YES;
			break;
		}
		else {
			if(ra_stat[cn] & ST_DAT)
				break;
			rp_irs = NO;
		}


rp_s5:
#ifdef	DEBUG
	if(rp_bps == 5)
		goto do_u_x;
#endif	DEBUG
printf("5 ");
/*
 * Save data read in step 4 in RCT block 1.
 */
	if(rctmcw(1, bp_ir) < 0) {
		rp_rcte(4, WRT, 1);
		goto rp_s18;
	}

rp_s6:
#ifdef	DEBUG
	if(rp_bps == 6)
		goto do_u_x;
#endif	DEBUG
printf("6 ");
/*
 * Begin phase 1, save info in RCT block 0.
 */
	if(rctmcr(0, bp_rci) < 0) {
		rp_rcte(6, RD, 0);
		goto rp_s18;
	}
	rtp = (struct rct_rci *)bp_rci;
	rtp->rt_lbn.ri_lbnl = rp_lbn;
	i = rtp->rt_lbn.ri_lbnw[0];	/* reverse order */
	rtp->rt_lbn.ri_lbnw[0] = rtp->rt_lbn.ri_lbnw[1];
	rtp->rt_lbn.ri_lbnw[1] = i;
	rtp->rt_stat &= ~RCI_FE;
	if(rp_irs == NO)
		rtp->rt_stat |= RCI_FE;
	rtp->rt_stat |= RCI_P1;
	if(rctmcw(0, bp_rci) < 0) {
		rp_rcte(6, WRT, 0);
		goto rp_s17;
	}

rp_s7:
#ifdef	DEBUG
	if(rp_bps == 7)
		goto do_u_x;
#endif	DEBUG
printf("7 ");
/*
 * Check to see if the block is really bad.
 *
 * Read the block, then write customer data, then write inverted
 * customer data.
 */

	nondataerr = NO;
	rp_wcs7 = YES;
	if(rctmcr(1, bp_tmp) < 0) {	/* Get best guess data from RCT blk 1 */
		rp_rcte(7, RD, 1);
		goto rp_s18;
	}
	for (i = 0; i < 4; i++) {
		/* check for any data error */
		lseek(fd, (long)(rp_lbn*512), 0);
		ra_badc = RP_AC;
		ra_badm = MD_SEC|MD_SREC;
		if ((read(fd, buf, 512) != 512)
			&& ((ra_eflags[cn] & EF_BBR) || (ra_eflags[cn] & EF_BBU))
			|| (ra_stat[cn]  & 0xe8)) {
			rp_wcs7 = NO;
			break;
		}
	}
	if (rp_wcs7 == NO && ((ra_stat[cn]  & 0xe8) != 0xe8)) {
		/* printf(" nondata read failure status = 0x%x - aborting\n",
			ra_stat); GMM */
		rp_wcs7 = YES;
		nondataerr = YES;
	}
	if (rp_wcs7 == YES && ~nondataerr) {
		bp_tp = &buf[2048];
		bp_tmp = &buf[512];
		for (j= 0; j < 512; j++)
			*bp_tp++ = ~*bp_ir++;
		bp_tp = &buf[2048];
		bp_ir = &buf[512];
		for (i = 0; i < 8; i++) {
			if ((rp_wcs7 = blocktest(fd, !rp_irs, bp_ir)) == YES)
				rp_wcs7 = blocktest(fd, 0, bp_tp);
			if (rp_wcs7 == NO)
				break;
		}
	}


rp_s8:
#ifdef	DEBUG
	if(rp_bps == 8)
		goto do_u_x;
#endif	DEBUG
printf("8 ");
/*
 * Bad block may be good, write saved bad block contents back
 * out to bad block. May be a transient bad block error????
 *
 * NOTE: the "force" variable causes replacement regardless!
 */
	rp_wcs8 = YES;
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_WRT;
	ra_badm = MD_CMP;
	if(rp_irs == NO)
		ra_badm |= MD_ERR;
	if(write(fd, (char *)bp_ir, 512) == 512) {
		if(((ra_eflags[cn] & EF_BBR) == 0) &&
		    (rp_irs == YES)) {
			if(force || (rp_wcs7 == NO))
				goto rp_s9;
			else 
				goto rp_s13;
		}
	}
	if((ra_stat[cn] & 0xe8) && 	/* DATA ERROR with FORCED ERROR */
	   (rp_irs == NO) &&
	   ((ra_eflags[cn] & EF_BBR) == 0))
			if(force)
				goto rp_s9;
			else 
				goto rp_s13;

rp_s9:
#ifdef	DEBUG
	if(rp_bps == 9)
		goto do_u_x;
#endif	DEBUG
printf("9 ");
/*
 * Seach the RCT for the replacement block number (RBN) and
 * whether or not the bad LBN was previously replaced, if so
 * also get the old RBN.
 */
/* Set initial (hash) RCT block # and offset, init variables */

	if (bbr_recur >= 2) {
		printf("Replacement Command Failure at LBN %d\n", rp_lbn);
		goto rp_s16;
	}
	else
		bbr_recur++;
	firstblk = 0;
	rp_prev = rp_pri = rp_sec = 0;
	rp_l = ra_drv[cn][unit].ra_trksz;	/* LBNs per track */
	rp_r = ra_drv[cn][unit].ra_rbns;	/* RBNs per track */
	rp_hblk = (((rp_lbn / rp_l) * rp_r) / 128) + 2;
	rp_hoff = ((rp_lbn / rp_l) * rp_r) % 128;
	rp_blk = rp_hblk;
	rp_off = rp_hoff;
/* Check for primary RBN descriptor empty or matches this LBN */

	if(rctmcr(rp_blk, bp_rct1) < 0) {
		rp_rcte(9, RD, rp_blk);
		goto rp_s16;
	}
	if(rbempty()) {
		rp_pri++;
		goto rp_s9_f;	/* found RBN */
	}
	if(rbmatch()) {
		rp_prev++;
		rp_orbn = ((rp_blk - 2) * 128) + rp_off;
/* OLDCODE	rp_orbn = bp_rct1[rp_off].rbnd_l & ~036000000000;	*/
		rp_orbd = bp_rct1[rp_off].rbnd_l;
	}
/* Start ping-pong search */


	first_level = YES;
	off_bottom = off_top = NO;
	rp_delta = 1;
	do {
		rp_off = rp_hoff + rp_delta;
		if((rp_off >= 0) && (rp_off <= 127) && !rbnull()) {
			if(rbempty()) {
				rp_sec++;
				goto rp_s9_f;		/* found RBN */
			}
			if(rbmatch()) {
				rp_prev++;
				rp_orbn = ((rp_blk - 2) * 128) + rp_off;
				rp_orbd = bp_rct1[rp_off].rbnd_l;
			}
		}
		else {
			if(rp_off > 127)
				off_bottom = YES;
			if(rp_off < 0)
				off_top = YES;
			if((off_top = YES) && (off_top == off_bottom))
				first_level == NO;
		}
		rp_delta = -rp_delta;
		if(rp_delta >= 0)
			rp_delta++;
	}
	while ((first_level) && (!rbnull()));
	if(!rbnull())
		rp_blk++;
	else
		rp_blk = 2;
	firstblk++;
	rp_off = rp_hoff = 0;

/* Start of linear search */

rp_s9_l:
	if(rblast()) {	/* If primary in last RCT block, go to first block */
		rp_blk = 2;
		rp_off = 0;
	}
rp_s9_l1:
	if(rctmcr(rp_blk, bp_rct1) < 0) {
		rp_rcte(9, RD, rp_blk);
		goto rp_s16;
	}
	rp_off = 0;
rp_s9_l2:
	if((rp_blk == rp_hblk) && (!firstblk )) {  /* search failed */
		rp_rcte(9, -1, 0);	/* RP step 9 FAILED! - message */
		printf("\nRCT search failed: no replacement block available!\n");
		goto rp_s16;
	}
	if(rbempty()) {
		rp_sec++;
		goto rp_s9_f;		/* found RBN */
	}
	if(rbmatch()) {
		rp_prev++;
		rp_orbn = ((rp_blk - 2) * 128) + rp_off;
/* OLDCODE	rp_orbn = bp_rct1[rp_off].rbnd_l & ~036000000000;	*/
		rp_orbd = bp_rct1[rp_off].rbnd_l;
	}
	if(rbnull()) {
		rp_blk = 2;
		goto rp_s9_l1;
	}
	rp_off++;
	if(rp_off > 127) {
		rp_blk++;
		firstblk = 0;
		goto rp_s9_l1;
	}
	goto rp_s9_l2;
rp_s9_f:
	rp_rbn = ((rp_blk - 2) * 128) + rp_off;
rp_s10:
#ifdef	DEBUG
	if(rp_bps == 10)
		goto do_u_x;
#endif	DEBUG
/*
 * Update RCT block 0 with RBN, BR flag, and say in phase 2.
 */
printf("10 ");
	rtp->rt_rbn.ri_rbnl = rp_rbn;
	i = rtp->rt_rbn.ri_rbnw[0];	/* reverse order */
	rtp->rt_rbn.ri_rbnw[0] = rtp->rt_rbn.ri_rbnw[1];
	rtp->rt_rbn.ri_rbnw[1] = i;
	if(rp_prev) {
		rtp->rt_brb.ri_brbl = rp_orbn;
		i = rtp->rt_brb.ri_brbw[0];	/* reverse order */
		rtp->rt_brb.ri_brbw[0] = rtp->rt_brb.ri_brbw[1];
		rtp->rt_brb.ri_brbw[1] = i;
		rtp->rt_stat |= RCI_BR;
	}
	rtp->rt_stat &= ~RCI_P1;
	rtp->rt_stat |= RCI_P2;
	if(rctmcw(0, bp_rci) < 0) {
		rp_rcte(10, WRT, 0);
		goto rp_s16;
	}

rp_s11:
#ifdef	DEBUG
	if(rp_bps == 11)
		goto do_u_x;
#endif	DEBUG
/*
 * Update RCT to say block has been replaced.
 */
printf("11 ");
	bp_rct1[rp_off].rbnd_l = rp_lbn;
	if(rp_pri)
		bp_rct1[rp_off].rbnd_w[0] |= 020000;
	else
		bp_rct1[rp_off].rbnd_w[0] |= 030000;
	rp_oblk = (rp_orbn / 128) + 2;
	rp_ooff = rp_orbn % 128;
	if(rp_prev) {
		if(rp_blk != rp_oblk) {	/* rbn & old rbn in diff blks */
			if(rctmcr(rp_oblk, bp_rct2) < 0) {
				rp_rcte(11, RD, rp_oblk);
				goto rp_s16;
			}
			bp_rct2[rp_ooff].rbnd_l = 010000000000L;
		} else
			bp_rct1[rp_ooff].rbnd_l = 010000000000L;
	}
	if(rctmcw(rp_blk, bp_rct1) < 0) {
		rp_rcte(11, WRT, rp_blk);
		goto rp_s15;
	}
	if(rp_prev && (rp_oblk != rp_blk)) {
		if(rctmcw(rp_blk, bp_rct2) < 0) {
			rp_rcte(11, WRT, rp_blk);
			goto rp_s15;
		}
	}

rp_s12:
#ifdef	DEBUG
	if(rp_bps == 12)
		goto do_u_x;
#endif	DEBUG
/*
 * Use the REPLACE command to cause the controller
 * to replace the bad block.
 */
printf("12 ");
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_REP;
	if(rp_pri)
		ra_badm = MD_PRI;
	ra_rbn.ra_rbnl = rp_rbn;	/* tell driver replacement block # */
	write(fd, (char *)bp_ir, 512);	/* really a REPLACE command */
	if((ra_stat[cn] & ST_MSK) != ST_SUC) {
		rp_rcte(12, -1, 0);	/* RP step 12 FAILED! - message */
		printf("\nMSCP replace command failed!\n");
		goto rp_s17;
	}
	if(rp_dirty == 2) goto rp_s120;  
	/*
	 * First, read the block and check for the Forced Error flag. 
	 * All unused RBNs are written with the forced error flag, so
	 * it should be set if the replace command worked.
	 */
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_RD;
	read(fd, buf, 512);
	if ((ra_stat[cn] & ST_DAT) != ST_DAT) {
		rp_rcte(12, -1, 0);	/* RP step 12 FAILED! - message */
		printf("\nMSCP replace command failed!\n");
		printf("RBN used for LBN %d did not contain Forced Error indicator.\n", rp_lbn);
		/*printd2("ra_stat = 0x%x, ra_ecode = 0x%x, ra_eflags = 0x%x\n",
			ra_stat, ra_ecode, ra_eflags); */
		goto rp_s17;
	}
	/* 
	 * Read the date to be written to the replacement block
	 * from RCT block 1 -- just to be safe.
	 */
	/* if (rctmcr(1, bp_ir) < 0) {
		rp_rcte(12, RD, 0);
		goto rp_s18;
	} */
rp_s120:
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_WRT;
	ra_badm = MD_CMP;
	if(rp_irs == NO)
		ra_badm |= MD_ERR;
	write(fd, (char *)bp_ir, 512);	/* write saved -> replacement block */
	if((ra_stat[cn] & ST_MSK) == ST_SUC) 
		goto rp_s121;
	if((ra_eflags[cn] & EF_BBR) != EF_BBR)
		if((ra_stat[cn] == ST_DAT) && (rp_irs == NO))
			goto rp_s121;
	/* write - compare failed */
	rp_rcte(12, -1, 0);
	printf("\nWRITE-COMPARE operation on replacement block failed!\n");
	/* 
	 *  If a header compare error or a bad block reported, go back
	 *  to step 9 and try to find another RBN.
	 */
	if((ra_stat[cn] != MD_HCE) || (ra_eflags[cn] & EF_BBR))
		goto rp_s9;
	else {
		printf("Check the RCT, it MAY be corrupt!!\n");
		if(cmd[0] = 'i')
			goto bg_loop;
		else {
			goto do_u_x;
		}
	}
rp_s121:	/* replacement succeeded */
	rp_didit++;

rp_s13:
#ifdef	DEBUG
	if(rp_bps == 13)
		goto do_u_x;
#endif	DEBUG
/*
 * Update RCT block 0 to indicate replacement
 * no longer in progress.
 */
printf("13 ");
	rtp->rt_stat &= RCI_VP;	/* clean out all but VP */
	rtp->rt_lbn.ri_lbnl = 0L;
	rtp->rt_rbn.ri_rbnl = 0L;
	rtp->rt_brb.ri_brbl = 0L;
	if(rctmcw(0, bp_rci) < 0) {
		rp_rcte(13, WRT, 0);
		goto rp_s17;
	}

rp_s14:
#ifdef	DEBUG
	if(rp_bps == 14)
		goto do_u_x;
#endif	DEBUG
/*
 * Soft lock not used.
 * Replacement completed, return!
 */
	if(cmd[0] == 'i') {
		if(rp_didit)
			goto do_i_rg;
		else
			goto do_i_ra;
	} else {
		if(fd > 0)
			close(fd);
		if(rp_dirty)
			rp_dirty = 0;
		if(rp_didit)
			printf("SUCCEEDED!\n");
		else
			if (nondataerr) {
				printf("ABORTED (nondata error encontered)!\n");
				nondataerr = NO;
			}
			else
				printf("ABORTED (block not really bad)!\n");
		goto retry;
	}

rp_s15:
#ifdef	DEBUG
	if(rp_bps == 15)
		goto do_u_x;
#endif	DEBUG
/*
 * Restore RCT to say new RBN unusable and
 * set bad LBN back to its original state.
 * ECO: not sure what to do yet!
 *	For now, say RBN unusable regardless!
 */
printf("15 ");
	bp_rct1[rp_off].rbnd_l = 010000000000L;
	if(rp_prev) {
		if(rp_blk != rp_oblk)
			bp_rct2[rp_ooff].rbnd_l = rp_orbd;
		else
			bp_rct1[rp_ooff].rbnd_l = rp_orbd;
	}
	if(rctmcw(rp_blk, bp_rct1) < 0)
		rp_rcte(15, WRT, rp_blk);
	if(rp_prev && (rp_blk != rp_oblk))
		if(rctmcw(rp_oblk, bp_rct2) < 0)
			rp_rcte(15, WRT, rp_oblk);

rp_s16:
#ifdef	DEBUG
	if(rp_bps == 16)
		goto do_u_x;
#endif	DEBUG
/*
 * Write saved date back to the bad LBN
 * with forced error modifier.
 */
printf("16 ");
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_WRT;
	if(rp_irs == NO)
		ra_badm = MD_ERR;
	if(write(fd, (char *)bp_ir, 512) != 512) {
		rp_rcte(16, -1, 0);	/* RP step 16 FAILED! - message */
		printf("\nRewrite of original data back to bad block failed!\n");
	}

rp_s17:
#ifdef	DEBUG
	if(rp_bps == 17)
		goto do_u_x;
#endif	DEBUG
/*
 * Update RCT block 0 to say replacement
 * no longer inprogress.
 */
printf("17 ");
	rtp->rt_stat &= RCI_VP;	/* clean out all but VP */
	rtp->rt_lbn.ri_lbnl = 0L;
	rtp->rt_rbn.ri_rbnl = 0L;
	rtp->rt_brb.ri_brbl = 0L;
	if(rctmcw(0, bp_rci) < 0)
		rp_rcte(17, WRT, 0);

rp_s18:
#ifdef	DEBUG
	if(rp_bps == 18)
		goto do_u_x;
#endif	DEBUG
/*
 * Soft lock not used.
 * Replacement failed, return!
 */
	if(cmd[0] == 'i')
		goto do_i_rb;
	else {
		if(rp_dirty)
			rp_dirty = 0;
		printf("\n\7\7\7Block %D replacement FAILED!\n", rp_lbn);
		if(fd > 0)
			close(fd);
		goto retry;
	}
}

/*
 *  Write data to a block, and then read it -- thus testing the block
 */

blocktest(fd, ucerr, bp)
int	fd;			/* file descriptor */
char 	*bp;
int	ucerr;			/* uncorrectable ECC error when reading
				 * the block the block to be replaced
				 */
{
	int i, count;
	int	block_good;	/* true if block is thought to be good */

	block_good = YES;
	lseek(fd, (long)(rp_lbn*512), 0);
	ra_badc = RP_WRT;
	ra_badm = (MD_CMP|MD_SEC|MD_SREC | (!ucerr ? 0 : MD_ERR));
	if((((count = write(fd, bp, 512)) != 512) && (ra_eflags[cn] & EF_BBR)) ||
		(ra_stat[cn] & 0xe8)) {
		block_good = NO;
	}
	if (block_good == YES) {
		for (i = 0; i < 4; i++) {
			lseek(fd, (long)(rp_lbn*512), 0);
			ra_badc = RP_AC;
			ra_badm = MD_SEC|MD_SREC;
			if(((read(fd, buf, 512) != 512) && (ra_eflags[cn] & EF_BBR)) ||
				(ra_stat[cn] & 0xe8)) {
				block_good = NO;
				break;
			}
		}
	}
	return (block_good);
}
/*
 * RCT read/write fatal error message
 *
 * s	= step in replacement process
 * rw	= failure was on read or write
 *	= -1, print first line then return
 * lbn	= block number
 */

rp_rcte(s, rw, lbn)
int	s;
int	rw;
int	lbn;
{
	printf("\n\7\7\7Bad block replacement process step %d FAILED!", s);
	if(rw == -1)
		return;
	printf("\nMulticopy %s", (rw == WRT) ? "write" : "read");
	printf(" of RCT block %d FAILED!\n", lbn);
	deprnt();	/* print disk error info */
}

/*
 * Open the disk and set file descriptor in fd.
 * mode = how to open, read, write, read/write.
 * err  = ignore errors (see if drive is there).
 *
 * If the disk has an RCT, read RCT block zero
 * and set rp_dirty to the phase that was in
 * progress if the replacement was interrupted.
 * Otherwise, set rp_dirty to zero.
 */
dskopen(err, mode)
int err;
int mode;
{
	register struct rct_rci *rtp;

	if((fd = open(fn, mode)) <= 0) {
		if(err == 0) {
			printf("\nCan't open %s!\n", fn);
			exit(FATAL);
		}
		return;
	}
	rp_dirty = 0;
	if((ra_drv[cn][unit].ra_rctsz == 0) || (dip->di_flag != BB_COOP))
		return;
	if(rctmcr(0, bp_rci) < 0) {
		printf("\n\7\7\7WARNING: cannot read RCT block zero for ");
		printf("%s unit %d!\n", ra_dct[cn], unit);
		return;
	}
	rtp = (struct rct_rci *)bp_rci;
	if(rtp->rt_stat & RCI_P1)
		rp_dirty = 1;
	if(rtp->rt_stat & RCI_P2)
		rp_dirty = 2;
	if(rp_dirty == 0)
		return;
	printf("\n\7\7\7\t\t****** WARNING ******\n");
	printf("\nA bad block replacement was interrupted during phase %d.\n",
		rp_dirty);
	if(cmd[0] == 'r')
		return;
	printf("\nAfter the current command completes, execute a replace");
	printf("\ncommand by responding to the rabads < ..... >: prompt");
	printf("\nwith the letter r followed by <RETURN>. This will");
	printf("\ncause the interrupted replacement of be completed.\n");
}

prtdsk()
{
	struct dkinfo *dp;

	printf("\nDisk\tULTRIX\tSize in");
	printf("\nName\tName\tBlocks");
	printf("\n----\t----\t------");
	for(dp=dkinfo; dp->di_type; dp++){
		printf("\n%s\t", dp->di_type);
		printf("%s\t", dp->di_name);
		printf("%D", dp->di_size);
	}
	printf("\n");
}
/*
 * Drive information pointer set,
 *	pointer into dkinfo table,
 *	unit number,
 *	MSCP controller number,
 *	file name - ??(#,0).
 */
dipset()
{
	register int i;

	printf("\nDisk type < ");
	for(i=0; dkinfo[i].di_type; i++) {
		printf("%s ", dkinfo[i].di_type);
	}
	printf(">: ");
	gets(dt);
	for(dip=dkinfo; dip->di_type; dip++)
		if(strcmp(dip->di_type, dt) == 0)
			break;
	if(dip->di_type == 0) {
		printf("\n(%s) - not supported by RABADS!\n", dt);
dip_x:
		if(argflag)
			exit(FATAL);
		else
			return(-1);
	}
	printf("\nUnit number < 0-3 >: ");
	gets(dn);
	if((strlen(dn) != 1) || (dn[0] < '0') || (dn[0] > '3')) {
		printf("\nUnits 0 -> 3 only!\n");
		goto dip_x;
	}
	unit = dn[0] - '0';
	sprintf(fn, "%s(%s,0)", dip->di_name, dn);
	for(i=0; devsw[i].dv_name; i++)
		if(strcmp(dip->di_name, devsw[i].dv_name) == 0)
			break;
	cn = devsw[i].dv_cn;
#ifdef	DEBUG

	printf("\nDEBUG: Break Point at replacement step ? ");
	gets(line);
	if(line[0] == '\0')
		rp_bps = 0;
	else
		rp_bps = atoi(line);

#endif	DEBUG
	return(0);
}

/*
 * RCT multicopy read,
 *	lbn = sector of RCT to read.
 *	bp = pointer to buffer.
 *	Host area size is added to lbn.
 * cn is MSCP cntlr number, unit is drive number,
 * fd is the previously opened file descriptor,
 * all are external to main().
 */

rctmcr(lbn, bp)
unsigned lbn;
union rctb *bp;
{
	register int i, n, s;
	daddr_t	bn, boff;

	n = ra_drv[cn][unit].ra_ncopy;
	s = ra_drv[cn][unit].ra_rctsz;
	bn = ra_drv[cn][unit].d_un.ra_dsize + lbn;
	for(i=0; i<n; i++) {
		if(rctcopy && (rctcopy != (i+1)))
			continue;
		boff = bn * 512;
		lseek(fd, (long)boff, 0);
		ra_badc = RP_RD;	/* tell driver doing BADS funny business */
		read(fd, (char *)bp, 512);
		if(ra_stat[cn] == ST_SUC) {
			if(lbn > 1)
				rctrev(bp);
			return(0);
		}
		bn += s;
	}
	return(-1);
}

/*
 * RCT multicopy write,
 *	lbn = sector of RCT to write.
 *	bp = pointer to buffer.
 *	Host area size is added to lbn.
 * cn is the MSCP cntlr number, unit is drive number,
 * fd is the previously opened file descriptor,
 * all are external to main().
 */

rctmcw(lbn, bp)
unsigned lbn;
union rctb *bp;
{
	register int i, n, s;
	int ec, sc, fatal;
	daddr_t bn, boff;

	n = ra_drv[cn][unit].ra_ncopy;
	s = ra_drv[cn][unit].ra_rctsz;
	bn = ra_drv[cn][unit].d_un.ra_dsize + lbn;
	ec = fatal = 0;
	if(lbn > 1)		/* only reverse blocks containing RBNs */
		rctrev(bp);
	for(i=0; i<n; i++, bn += s) {
		boff = bn * 512;
		lseek(fd, (long)boff, 0);
		ra_badc = RP_WRT;	/* tells driver doing BADS funny business */
		ra_badm = MD_CMP;
		write(fd, (char *)bp, 512);
		if(ra_stat[cn] == ST_SUC)
			continue;
		sc = (ra_stat[cn] >> 5) & 03777;
		if((ra_stat[cn] & ST_MSK) == ST_DAT) {
			if((sc != 2) && (sc != 3) && (sc != 7))
				continue; /* NOT - HCE, DATA SYNC, UNC ECC */
			ec++;
			lseek(fd, (long)boff, 0);
			ra_badc = RP_WRT;
			ra_badm = MD_ERR;
			write(fd, (char *)bp, 512);
			if((ra_stat[cn] & ST_MSK) == ST_SUC)
				continue;
			sc = (ra_stat[cn] >> 5) & 03777;
			if((ra_stat[cn] & ST_MSK) == ST_DAT) {
				if((sc != 2) && (sc != 3) && (sc != 7))
					continue;
			}
		}
		fatal = -1;
		break;
	}
	if(ec >= n)
		fatal = -1;
	return(fatal);
}

/*
 * Reverse the order of the RBN descriptors in bp_rct1[].
 * ULTRIX-11 C still need hi order word first!
 */

rctrev(bp)
union rctb *bp;
{
	register union rctb *rbp;
	register int i, j;

	rbp = bp;
	for(i=0; i<128; i++) {
		j = rbp[i].rbnd_w[0];
		rbp[i].rbnd_w[0] = rbp[i].rbnd_w[1];
		rbp[i].rbnd_w[1] = j;
	}
}

/*
 * Print the disk error information that would
 * have been printed by the driver if the ra_badc
 * flag was not set.
 */
deprnt()
{
	printf("%s unit %d disk error: ",
		ra_dct[cn], unit);
	printf("endcode=%o flags=%o status=%o\n",
		ra_ecode[cn], ra_eflags[cn], ra_stat[cn]);
	printf("(FATAL ERROR)\n");
}

rbempty()
{
	if((bp_rct1[rp_off].rbnd_l & 036000000000) == 0)
		return(1);
	else
		return(0);
}

rbmatch()
{
	if((bp_rct1[rp_off].rbnd_l & 04000000000) == 0)
		return(0);
	if(rp_lbn == (bp_rct1[rp_off].rbnd_l & ~036000000000))
		return(1);
	return(0);
}

rbnull()
{
	if((bp_rct1[rp_off].rbnd_l & 036000000000) == 020000000000)
		return(1);
	else
		return(0);
}
rblast()
{
	register int i;

	for(i=0; i<128; i++)
		if((bp_rct1[i].rbnd_l & 036000000000) == 020000000000)
			return(1);
	return(0);
}

prfm()
{
	register char c;

	printf("\n\nPress <RETURN> for more");
	printf(" (<CTRL/C> to abort): ");
	while((c=getchar()) != '\n') {
		if(c == 03) /* <CTRL/C> */
			return(1);
	}
	return(0);
}

char	ynline[40];

yes(hlp)
{
yorn:
	gets(ynline);
	if((strcmp(ynline, "y") == 0) || (strcmp(ynline, "yes") == 0))
		return(YES);
	if((strcmp(ynline, "n") == 0) || (strcmp(ynline, "no") == 0))
		return(NO);
	if(hlp)
	    if((strcmp(ynline, "?") == 0) || (strcmp(ynline, "help") == 0))
		return(HELP);
	printf("\nPlease answer yes or no!\n");
	goto yorn;
}

/*
 * Read a suspected bad block and
 * return its status.
 */

brchk(lbn)
daddr_t lbn;
{
	register int i, s;

	for(i=0; i<100; i++) {
		lseek(fd, (long)(lbn*512), 0);
		ra_badc = RP_AC;
		if(read(fd, (char *)&buf, 512) != 512)
			break;
	}
	s = ra_stat[cn] & ST_MSK;
	if(s == ST_SUC) {
		if(ra_eflags[cn] & EF_BBR)
			return(BR_BBR);
		else
			return(BR_NBBR);
	} else if(s == ST_DAT) {
		if((dip->di_flag != BB_RWRT) && (ra_eflags[cn] & EF_BBR))
			return(BR_DEB);
		switch((ra_stat[cn] >> 5) & 03777) {
		case 0:
			return(BR_FEM);
		case 4:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			/* can't happen - not allowing datagrams */
			return(BR_CECC);
		case 7:
			return(BR_UECC);
		default:
			return(BR_NDE);
		}
	} else
		return(BR_NDE);
}

/*
 * Conditionally print BLOCK # header message.
 * Rewite a block that was written with "forced error".
 * Return status of rewrite operation.
 */

fembrw(hm, lbn)
int	hm;
daddr_t	lbn;
{
	if(hm)
		printf("BLOCK: %D - ", lbn);
	printf("rewrite to clear \"forced error\" ");
	lseek(fd, (long)(lbn*512), 0);
	ra_badc = RP_WRT;
	write(fd, (char *)&buf, 512);
	lseek(fd, (long)(lbn*512), 0);
	if(read(fd, (char *)&buf, 512) == 512) {
		printf("SUCCEEDED!\n");
		return(YES);
	} else {
		printf("FAILED!\n");
		deprnt();	/* print disk error info */
		return(NO);
	}
}
