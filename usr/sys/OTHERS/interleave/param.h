#ifndef	INTRLVE
#define	dkblock(bp)	((bp)->b_blkno)
#define	dkunit(bp)	(minor ((bp)->b_dev) >> 3)
#endif	INTRLVE
