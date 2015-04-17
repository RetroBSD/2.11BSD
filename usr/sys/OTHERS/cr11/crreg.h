struct crdevice {
	short	crcs;
	short	crb1;
	short	crb2;
};

/* bits in crcs */
#define	CR_ERR		0100000		/* error */
#define	CR_CARDDONE	0040000		/* card done */
#define	CR_HCHK		0020000		/* hopper check */
#define	CR_MCHK		0010000		/* motion check */
#define	CR_TERR		0004000		/* timing error */
#define	CR_ONLINE	0002000		/* reader to on line */
#define	CR_BUSY		0001000		/* busy */
#define	CR_RRS		0000400		/* reader ready status */
#define	CR_COLDONE	0000200		/* column done */
#define	CR_IE		0000100		/* interrupt enable */
#define	CR_EJECT	0000002		/* eject */
#define	CR_READ		0000001		/* read */

#define	CR_HARDERR	(CR_ERR|CR_HCHK|CR_MCHK|CR_TERR)

#define	CR_BITS		\
"\10\20ERR\17CARDDONE\16HCHK\15MCHK\14TERR\13ONLINE\12BUSY\11RRS\10COLDONE\
\7IE\2EJECT\1READ"
