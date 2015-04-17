/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* def.monst.h - version 1.0.2 */

struct monst {
	struct monst *nmon;
	struct permonst *data;
	unsigned m_id;
	xchar mx,my;
	xchar mdx,mdy;		/* if mdispl then pos where last displayed */
#define	MTSZ	4
	coord mtrack[MTSZ];	/* monster track */
	schar mhp,mhpmax;
	char mappearance;	/* nonzero for undetected 'M's and for '1's */
	EasyBitfield(mimic,1);	/* undetected mimic */
	EasyBitfield(mdispl,1);	/* mdx,mdy valid */
	EasyBitfield(minvis,1);	/* invisible */
	EasyBitfield(cham,1);	/* shape-changer */
	EasyBitfield(mhide,1);	/* hides beneath objects */
	EasyBitfield(mundetected,1);	/* not seen in present hiding place */
	EasyBitfield(mspeed,2);
	EasyBitfield(msleep,1);
	EasyBitfield(mfleetim,7);	/* timeout for mflee */
	EasyBitfield(mfroz,1);
	EasyBitfield(mconf,1);
	EasyBitfield(mflee,1);	/* fleeing */
	EasyBitfield(mcan,1);	/* has been cancelled */
	EasyBitfield(mtame,1);		/* implies peaceful */
	EasyBitfield(mpeaceful,1);	/* does not attack unprovoked */
	EasyBitfield(isshk,1);	/* is shopkeeper */
	EasyBitfield(isgd,1);	/* is guard */
	EasyBitfield(mcansee,1);	/* cansee 1, temp.blinded 0, blind 0 */
	EasyBitfield(mblinded,7);	/* cansee 0, temp.blinded n, blind 0 */
	EasyBitfield(mtrapped,1);	/* trapped in a pit or bear trap */
	EasyBitfield(mnamelth,6);	/* length of name (following mxlth) */
#ifndef NOWORM
	EasyBitfield(wormno,5);	/* at most 31 worms on any level */
#endif NOWORM
	unsigned mtrapseen;	/* bitmap of traps we've been trapped in */
	long mlstmv;	/* prevent two moves at once */
	struct obj *minvent;
	long mgold;
	unsigned mxlth;		/* length of following data */
	/* in order to prevent alignment problems mextra should
	   be (or follow) a long int */
	long mextra[1];		/* monster dependent info */
};

#define newmonst(xl)	(struct monst *) alloc((unsigned)(xl) + sizeof(struct monst))

extern struct monst *fmon;
extern struct monst *fallen_down;
struct monst *m_at();

/* these are in mspeed */
#define MSLOW 1 /* slow monster */
#define MFAST 2 /* speeded monster */

#define	NAME(mtmp)	(((char *) mtmp->mextra) + mtmp->mxlth)
#define	MREGEN		"TVi1"
#define	UNDEAD		"ZVW "
