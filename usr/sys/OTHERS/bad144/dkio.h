struct drv_info {
	short	type;	/* RMDT register */
	short	model;	/* model byte */
	short	trk;	/* tracks/cyl */
	short	sec;	/* sec/trk */
	short	nspc;	/* sec/cyl */
	short	ncyl;	/* no. of cyls */
	char	name[48];
	struct sizes {
		daddr_t nblocks;
		int cyloff;
	} fs[8];
};

/* disk i/o controls */
#define	DKIOCHDR	_IO(d, 1)	/* next i/o includes header */
#define	DKREINIT	_IO(d, 6)	/* reread bad block forwarding table */
#define	DKINFO		_IOR(d, 7, struct drv_info)	/* get drive info */
