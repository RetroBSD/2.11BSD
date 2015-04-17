#define NIOMBLKS	31212	/* Total number of blocks on device */
#define XIMI_NCYL	306	/* # of cylinders per drive, imi/xebec */
#define XIMI_NSECT 	17	/* # of sectors per track, imi/xebec */
#define XIMI_NSURF 	6	/* # of surfaces (heads) per drive, imi/xebec */
#define NPART		3	/* Number of Bits used to hold partition no. */
#define PARTMSK		07	/* 3 bit mask for above */
#define DRIVPOS		3	/* position of drive bits */ 
#define DRIVMSK		03	/* 2 bit mask for above */
#define NTAB		3	/* Number of bits for table number */
#define TABPOS		5	/* position of table bits */
#define TABMSK		07	/* 3 bit mask for above */

#define PARTSIZ		PARTMSK + 1	/* Number of partitions in drive
					table entries */
#define TABSIZ		TABMSK + 1	/* Number of tables in drive
					table entries */
	/* Characteristics For IOM Disks */
#define IOMBLKFAC	1		/* blk adr. multiply x2 in command
					 block for 256b blocks on iom */
#define IOMSHIFT	8		/* Divide by 2**8=256 (instead of 512)
					 for number of blocks to xfer at once */

	/* Interface Bits */

	/* CCSR: Command Completion Status Register */
#define	PERR	1		/* Parity error, bit 0 */
#define	CONTROL	2		/* Control or drive error bit 1 */
#define	LUN	0340
#define	SEQ	0177400

	/* Control/Status Register */
#define GO	1
#define FORRIOM	2
#define DXMEM	074
#define INTR	0100
#define DONE	0200
#define CXMEM	07400
#define PERR2	020000
#define NONXMEM	040000
#define ERROR	0100000

	/* Disk Controller Commands Class 0 */
#define	TEIOMRDY	0
#define	REIOMORE	1
#define	RQSENSE	3
#define	FORMAT	4
#define	CKTRK	5
#define	FMTRK	6
#define	FBDTRK	7
#define	READ	010
#define	WRITE	012
#define	SEEK	013
#define	INITSZ	014
	/* Class 7 commands */

#define	TEIOMRAM	0340
#define	RETRY	0
#define DISRTRY	0100000

	/* Error codes */
#define TYPE0	0
#define TYPE1	020
#define	TYPE2	040
#define TYPE3	060

	/*  The only interesting one */
#define CDFE	010	/* Correctable data field error */

	/* MISC */
#define DTC1_ADDR	0177460
#define DTC1_VECTOR	0134
#define DTCIOM_ADDR	0177420
#define DTCIOM_VECTOR	0130
#define	EXTADDR		017		/* Extended address bits */

struct dtc
{
	int	ccsr;
	int	csr;
	caddr_t	bufaddr;
	caddr_t	cmdaddr;
	int	xbufaddr;
	int	xcmdaddr;
	int	dum3;	/* unused */
	int	dum4;	/* unused */
};

struct sc_sizes
{
	long	boffset;
	long	nblks;
};
