/*
 *	xereg.h
 *
 *	defines for the DTC-XEBEC combination controller
 *	on a Q-bus
 */

struct xedevice {
	short	xeccsr;
	short	xecsr;
	short	xedar;
	short	xecar;
	short	xexdar;
	short	xexcar;
};

struct xep {
	int	xe_nsect;
	int	xe_ntrac;
	int	xe_nhead;
	int	xe_rwcur;
	int	xe_wpc;
	int	xe_seek;
	int	xe_isset;
};

struct xesize {
	daddr_t		nblocks;
	daddr_t		blkoff;
};

struct xecommand {
	char	xeop;		/* opcode and command class */
	char	xeunit;		/* drive number and bits 20-16 of blk no */
	char	xehblk;		/* bits 15-8 of block number */
	char	xelblk;		/* bits 7-0 of block number */
	char	xecount;	/* block count */
	char	xecntl;		/* control field (stepping option) */
};

struct xeinit {
	char	xeihtrac;	/* high order number of cylinders */
	char	xeiltrac;	/* low order number of cylinders */
	char	xeinhead;	/* number of heads */
	char	xeihrwcur;	/* high order reduced current cylinder number */
	char	xeilrwcur;	/* low order reduced current cylinder number */
	char	xeihwpc;	/* high order write precomp cylinder number */
	char	xeilwpc;	/* low order write precomp cylinder number */
	char	xeiecc		/* ecc error correction length, usually 11 */
};

struct xeiblock {
	short	xeimagic;	/* magic number to check validity */
	char	xeibnsect;	/* number of sectors/track */
	char	xeibnhead;	/* number of heads */
	short	xeibntrac;	/* number of cylinders */
	short	xeibrwcur;	/* reduced write current cylinder number */
	short	xeibwpc;	/* write precomp cylinder */
	char	xeibecc;	/* ecc error correction length */
	char	xeibseek;	/* seek parameter */
	struct xesize xeibsizes[8];	/* size of each logical device */
};

# define XEIMAGIC	0077000		/* magic number */

/*
 *	command completion status register bits
 */
# define CCSRSEQER	0177400	/* sequence error bits 15-8 */
# define CCSRLUN	0000340	/* logical unit number bits 7-5 */
# define CCSRCDE	0000002	/* controller/drive error bit 1 */
# define CCSRPE		0000001	/* parity error bit 0 */
# define XECCSR_BITS "\02CCSRCDE\01CCSRPE"

/*
 *	control/status register bits
 */
# define CSRERROR	0100000	/* any error detected bit 15 */
# define CSRNXM		0040000	/* non-existant memory bit 14 */
# define CSRPE		0020000	/* parity error bit 13 */
# define CSRCXMA	0007400	/* bits 19-16 of com block addr bits 11-8 */
# define CSRDONE	0000200	/* done bit 7 */
# define CSRIE		0000100	/* interrupt enable bit 6 */
# define CSRDXMA	0000074	/* bits 19-16 of data block addr bits 5-2 */
# define CSRRESET	0000002	/* force reset bit 1 */
# define CSRGO		0000001	/* go bit 0 */
# define XECSR_BITS "\20CSRERROR\17CSRNXM\16CSRPE\10CSRDONE\7CSRIE\2CSRRESET\1CSRGO"

/*
 *	data address register
 */
# define DARADDR	0177777	/* low order data address bits 15-0 */

/*
 *	command address register
 */
# define CARADDR	0177777	/* low order command address bits 15-0 */

/*
 *	extension data address register
 */
# define XDARADDR	0000003	/* bits 21-20 of data address bits 1-0 */

/*
 *	extension command address register
 */
# define XCARADDR	0000003	/* bits 21-20 of command address bits 1-0 */

/*
 *	commands to the xebec board
 */
# define XERDY		000	/* select and verify drive ready */
# define XEHOME		001	/* home to track 0 */
# define XERSS		003	/* read sense error information */
# define XEFMT		004	/* format drive */
# define XECFMT		005	/* check drive format */
# define XETFMT		006	/* format track */
# define XEBTRK		007	/* set bad track bit */
# define XEREAD		010	/* read N blocks */
# define XEWRITE	012	/* write N blocks */
# define XESEEK		013	/* seek to block N */
# define XEINIT		014	/* initialize drive charactistics */
# define XERECC		015	/* read ecc burst error length */
# define XEFAT		016	/* format alternate track */
# define XEWSB		017	/* write sector buffer */
# define XERSB		020	/* read sector buffer */
# define XERAMT		0340	/* ram test */
# define XEDRVT		0343	/* drive diagnostic */
# define XECID		0344	/* controller internal diagnostics */
# define XERL		0345	/* read long */
# define XEWL		0346	/* write long */
