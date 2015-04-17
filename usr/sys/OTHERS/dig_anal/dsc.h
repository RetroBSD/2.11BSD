/* dsc.h	1.2 (CARL) 1/14/84  09:56:11 */

#include	<sys/ioctl.h>

/*
 * these values are critical;
 * dsseq depends on da being 0
 * and ad being 1
 */
# define DA		(0)
# define AD		(1)

/*
 * Errors.
 * Returned in ds_err.errors.
 */
# define EDS_ARGS	01	/* missing arguments/parameters */
# define EDS_ACC	02	/* converters in use */
# define EDS_MOD	04	/* buffer size wasn't modulo MINDSB */
# define EDS_SIZE	010	/* buffer size was too small */
# define EDS_DISK	020	/* disk error */
# define EDS_CERR	040	/* converter error */
# define EDS_RST	0100	/* dsreset clobbered us */

/*
 * Ioctl commands.
 */
# define DSSEQ		_IOW(s,	0,	struct ds_seq)	/* sequence */
# define DSRATE		_IOW(s,	1,	struct ds_seq)	/* rate */
# define DS20KHZ	_IO(s,	2)			/* 20kHz filter */
# define DS10KHZ	_IO(s,	3)			/* 10kHz filter */
# define DS5KHZ		_IO(s,	4)			/* 5kHz filter */
# define DSBYPAS	_IO(s,	5)			/* bypass filter */
# define DSERRS		_IOR(s,	6,	struct ds_err)	/* get errors */
# define DSBNO		_IOW(s,	7,	struct ds_fs)	/* starting block */
# define DSTODO		_IO(s,	8)			/* # of bytes to do */
# define DSCOUNT	_IOW(s,	9,	struct ds_fs)	/* amnt. to convert */
# define DSBOFF		_IOW(s,	10,	struct ds_fs)	/* 1st buffer offset */
# define DSNODSK	_IO(s,	11)			/* no disking */
# define DSLAST		_IOW(s,	12,	struct ds_seq)	/* last seq ram */
# define DSDEV		_IOW(s,	13,	struct ds_seq)	/* disk to use */
# define DSDONE		_IOR(s,	14,	struct ds_fs)	/* amnt. done */
# define DSMON		_IO(s,	15)			/* set monitor mode */
# define DSBRD		_IO(s,	16)			/* set broadcast mode */
# define DSNBLKS	_IOW(s,	17,	struct ds_fs)	/* size of block list */

/*
 * this one is peculiar. instead of
 * letting the system do the copyin
 * for us we do it ourselves.
 */
# define DSBLKS		_IO(s, 18)			/* block list */

# define NDSB		2		/* number of buffers chaining with */

/*
 * THIS IS SITE-SPECIFIC
 * CONFIGURED FOR DSP-VAX
 * produces 49152 Hz sampling rate when ASCSRT is set to 0x80
 * 4 dacs, 2 adcs, 20KHz and 6.5KHz filters
 */
# define D0BLPT		32		/* number of blocks per CDC track */
# define D0MINDSB	(D0BLPT*512)
# define DSCMRCLK	4000000		/* clock frequency */
# define DSCMRTIC	(DSCMRCLK/2)
# define DSCSRATE	49152.0
# define MAXDACS	2
# define MAXADCS	2
# define DSCFLT0	15000.0		/* 15.0KHz */
# define DSCFLT1	7000.0		/*  7.0KHz */
# define DSCFLT2	0.0		/* bypass mode */
# define DSCFLT3	4000.0		/*  4.0KHz */
# define DSHASDAF0	1		/* has flt0 */
# define DSHASDAF1	1		/* has flt1 */
# define DSHASDAF2	1		/* has bypass */
# define DSHASDAF3	1		/* has flt2 */
# define DSHASADF0	1		/* has flt0 */
# define DSHASADF1	1		/* has flt1 */
# define DSHASADF2	1		/* has bypass */
# define DSHASADF3	1		/* has flt2 */

# ifdef CARL
/*
 * THIS IS SITE-SPECIFIC
 * CONFIGURED FOR CARL
 * produces 49152 Hz sampling rate when ASCSRT is set to 0x80
 * 4 dacs, 2 adcs, 20KHz and 6.5KHz filters
 */
# define D0BLPT		32		/* number of blocks per CDC track */
# define D1BLPT		51		/* number of blocks per ra81 track */
# define D0MINDSB	(D0BLPT*512)
# define D1MINDSB	(D0BLPT*512)
# define D2MINDSB	(D1BLPT*512)
# define DSCMRCLK	6291456		/* clock frequency */
# define DSCMRTIC	(DSCMRCLK/2)
# define DSCSRATE	49152.0
# define MAXDACS	4
# define MAXADCS	2
# define DSCFLT0	20000.0		/* 20.0KHz */
# define DSCFLT1	6.5		/*  6.5KHz */
# define DSCFLT2	0.0		/* bypass mode */
# define DSCFLT3	0.0		/* not installed */
# define DSHASDAF0	1		/* has flt0 */
# define DSHASDAF1	1		/* has flt1 */
# define DSHASDAF2	1		/* has bypass */
# define DSHASDAF3	0		/* not installed */
# define DSHASADF0	1		/* has flt0 */
# define DSHASADF1	1		/* has flt1 */
# define DSHASADF2	1		/* has bypass */
# define DSHASADF3	0		/* not installed */
# endif CARL

# ifdef IRCAM
/*
 * THIS IS SITE-SPECIFIC
 * CONFIGURED FOR IRCAM
 * produces 48000 Hz sampling rate when ASCSRT is set to 0x80
 * 2 dacs, 2 adcs, 12.8KHz and 6.4KHz filters
 */
# define D0BLPT		31		/* number of blocks per rm80 track */
# define D1BLPT		31		/* additional disks go here */
# define D0MINDSB	(D0BLPT*512)
# define D1MINDSB	(D1BLPT*512)	/* additional disks go here */
# define DSCMRCLK	7680000
# define DSCMRTIC	(DSCMRCLK/2)
# define DSCSRATE	48000.0
# define MAXDACS 	2
# define MAXADCS 	2
# define DSCFLT0	12800.0		/* 12.8KHz */
# define DSCFLT1	 6400.0		/*  6.4KHz */
# define DSCFLT2	    0.0		/* bypass mode */
# define DSCFLT3	    0.0		/* not installed */
# define DSHASDAF0	1		/* has flt0 */
# define DSHASDAF1	0		/* not installed */
# define DSHASDAF2	1		/* has bypass */
# define DSHASDAF3	0		/* not installed */
# define DSHASADF0	1		/* has flt0 */
# define DSHASADF1	0		/* not installed */
# define DSHASADF2	1		/* has bypass */
# define DSHASADF3	0		/* not installed */
# endif IRCAM

# ifdef AMOS
/*
 * THIS IS SITE-SPECIFIC
 * CONFIGURED FOR AMOS
 * produces 49152.0 Hz sampling rate when ASCSRT is set to 0x80
 * 2 dacs, 1 adc, no filters
 */
# define D0BLPT		32		/* number of blocks per FUJI track */
# define D1BLPT		32		/* additional disks go here */
# define D0MINDSB	(D0BLPT*512)
# define D1MINDSB	(D0BLPT*512)	/* additional disks go here */
# define DSCMRCLK	6291456		/* clock frequency */
# define DSCMRTIC	(DSCMRCLK/2)
# define DSCSRATE	49152
# define MAXDACS 	2
# define MAXADCS 	1
# define DSCFLT0	0.0		/* not installed*/
# define DSCFLT1	0.0		/* not installed */
# define DSCFLT2	0.0		/* no bypass mode */
# define DSCFLT3	0.0		/* not installed */
# define DSHASF0	0		/* no flt0 */
# define DSHASF1	0		/* disabled */
# define DSHASF2	0		/* no bypass */
# define DSHASF3	0		/* not installed */
# endif AMOS

/*
 * reg specifies a sequence register (0-15).
 * conv specifies a converter.
 * dirt specifies the direction when
 * setting up the sequence ram (DSSEQ) or the
 * sampling rate (DSRATE).
 */
struct ds_seq {
	short reg;
	short conv;
	short dirt;			/* shared by DSSEQ and DSRATE */
};

/*
 * Format of returned converter
 * errors.
 */
struct ds_err {
	short dma_csr;
	short asc_csr;
	short errors;
};

/*
 * File information.
 * The member bnosiz is used for
 * lots of things; starting block number
 * of the file, size of the file, amount
 * to convert, etc.
 * On the vax this structure
 * is the wrong size; 8 bytes instead
 * of the desired 6; fs_unused isn't
 * used so this shouldn't be a problem.
 */
struct ds_fs {
	daddr_t bnosiz;
	short fs_unused;
};
