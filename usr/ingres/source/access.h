#
/*
**  ACCESS.H -- definitions relating to the access methods.
**
**	Version:
**		@(#)access.h	1.5	8/24/80
*/

# ifndef PGSIZE


# define	PGSIZE		512		/* size of page */
# define	PGPTRSIZ	4		/* size of a tid */
# define	MAXTUPS		100		/* maximum number of tups */

/* storage structure flags; < 0 means compressed */
# define	M_HEAP		5		/* paged heap */
# define	M_ISAM		11		/* indexed sequential */
# define	M_HASH		21		/* random hash */
# define	M_BTREE		31		/* BTREES */
# define	M_TRUNC		99		/* internal pseudo-mode: truncated */

# define	NACCBUFS	3		/* number of access method buffers */

/* error flags */
# define	AMREAD_ERR	-1
# define	AMWRITE_ERR	-2
# define	AMNOFILE_ERR	-3	/* can't open file for a relation */
# define	AMREL_ERR	-4	/* can't open relation relation */
# define	AMATTR_ERR	-5	/* can't open attribute relation */
# define	AMNOATTS_ERR	-6	/* attribute missing or xtra in att-rel */
# define	AMCLOSE_ERR	-7	/* can't close relation */
# define	AMFIND_ERR	-8	/* unidentifiable stora  Petructure in find */
# define	AMINVL_ERR	-9	/* invalid TID */
# define	AMOPNVIEW_ERR	-10	/* attempt to open a view for rd or wr */

/* the following is the access methods buffer */
struct accbuf
{
	/* this stuff is actually stored in the relation */
	long		mainpg;		/* next main page (0 - eof) */
	long		ovflopg;	/* next ovflo page (0 - none) */
	short		nxtlino;	/* next avail line no for this page */
	char		firstup[PGSIZE - 12];	/* tuple space */
	short		linetab[1];	/* line table at end of buffer - grows down */
					/* linetab[lineno] is offset into
					** the buffer for that line; linetab[nxtlino]
					** is free space pointer */

	/* this stuff is not stored in the relation */
	long		rel_tupid;	/* unique relation id */
	long		thispage;	/* page number of the current page */
	int		filedesc;	/* file descriptor for this reln */
	struct accbuf	*modf;		/* use time link list forward pointer */
	struct accbuf	*modb;		/* back pointer */
	int		bufstatus;	/* various bits defined below */
};

/* The following assignments are status bits for accbuf.bufstatus */
# define	BUF_DIRTY	001	/* page has been changed */
# define	BUF_LOCKED	002	/* page has a page lock on it */
# define	BUF_DIRECT	004	/* this is a page from isam direct */

/* access method buffer typed differently for various internal operations */
struct raw_accbuf
{
	char	acc_buf[NACCBUFS];
};

/* pointers to maintain the buffer */
extern struct accbuf	*Acc_head;	/* head of the LRU list */
extern struct accbuf	*Acc_tail;	/* tail of the LRU list */
extern struct accbuf	Acc_buf[NACCBUFS];	/* the buffers themselves */


/*
**  ADMIN file struct
**
**	The ADMIN struct describes the initial part of the ADMIN file
**	which exists in each database.  This file is used to initially
**	create the database, to maintain some information about the
**	database, and to access the RELATION and ATTRIBUTE relations
**	on OPENR calls.
*/

struct adminhdr
{
	char	adowner[2];	/* user code of data base owner */
	short	adflags;	/* database flags */
};

struct admin
{
	struct adminhdr		adhdr;
	struct descriptor	adreld;
	struct descriptor	adattd;
};

/*
**  Admin status bits
**
**	These bits define the status of the database.  They are
**	contained in the adflags field of the admin struct.
*/

# define	A_DBCONCUR	0000001		/* set database concurrency */
# define	A_QRYMOD	0000002		/* database uses query modification */
# define	A_NEWFMT	0000004		/* database is post-6.2 */


/* following is buffer space for data from admin file */
extern struct admin		Admin;

/*
**  PGTUPLE -- btree index key (a tid and an index key)
*/

struct pgtuple
{
	struct tup_id	childtid;		/* the pointer comes before */
	char		childtup[MAXTUP];
};

/*
** global counters for the number of UNIX read and write
**  requests issued by the access methods.
*/

extern long	Accuread, Accuwrite;

/*
**	Global values used by everything
*/

char		*Acctuple;		/* pointer to canonical tuple */
int		Accerror;		/* error no for fatal errors */
char		Accanon[MAXTUP];	/* canonical tuple buffer */

# endif PGSIZE
