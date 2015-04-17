#
/*
**	This header file contains all the defined constant
**	and special structures used by decomposition. Certain
**	global variables which are referenced by many modules
**	are also included. By convention global names always
**	begin with a capital letter.
*/



   
# define OVHDP		2		/*  overhead for a projection  */
# define OVHDM		10		/*  overhead for a modify  */

# define MAXRELN	6		/* size of relation descriptor cache */
# define MAXRANGE	MAXVAR+1	/* size of range table for source
					** and result relations. This size
					** must match size of parser range table
					*/
  
# define QBUFSIZ	2000		/* buffer size (bytes) of original query tree */
# define SQSIZ		3400		/* buffer size for tree copies + sub-queries */
# define AGBUFSIZ	350		/* buffer size for temp agg tree components */
# define PBUFSIZE	500		/* size of parameter buffer area for setp() */
# define PARGSIZE	MAXPARMS	/* max number of arguments for setp() */

/* error messages */
# define NODESCAG	4602	/* no descriptor for aggr func */
# define QBUFFULL	4610	/* Initial query buffer overflow */
# define SQBUFFULL	4612	/* sub-query buffer overflow */
# define STACKFULL	4613	/* trbuild stack overflow */
# define AGBUFFULL	4614	/* agg buffer overflow */
# define AGFTOBIG	4615	/* agg function exceeds MAXTUP or MAXDOM */
# define RETUTOBIG	4620	/* retr unique target list exceeds MAXTUP */

/* symbolic values for GETNXT parameter of fcn GET */
# define NXTTUP	1	/* get next tuple after one specified by tid */

/* flag for no result relation */
# define	NORESULT	-1

/* Range table slot which is always free for aggregate temp rels */
# define	FREEVAR		MAXRANGE	/* free var number for aggs */

/* Range table slot which is used for secondary index */
# define	SECINDVAR	MAXRANGE + 1
  

int	Qmode;		/* flag set to indicate mode of tuple disposition */
int	Resultvar;	/* if >= 0 result variable */
int	Sourcevar;	/* likewise for source variable */
int	Newq;		/* OVPQ must devise new strategy */
int	Newr;		/* force OVQP to reopen result relation */
int	R_ovqp;		/* pipe for reading from ovqp */
int	W_ovqp;		/* pipe for writing to ovqp */
int	R_dbu;		/* ditto dbu */
int	W_dbu;		/* ditto dbu */



/* range table structure */
struct rang_tab
{
	int		relnum;		/* internal decomp relation number */
	int		rtspec;		/* relspec of relation */
	int		rtstat;		/* relstat of relation */
	int		rtwid;		/* relwidth of relation */
	long		rtcnt;		/* tupcount of relation */
	int		rtaltnum;	/* reserved for distributed decomp */
	char		*rtattmap;	/* reserved for distributed decomp */
	long		rtdcnt;		/* reserved for distributed decomp */
	struct d_range	*rtsrange;	/* reserved for distributed decomp */
};

extern struct rang_tab	Rangev[];


/* structure used by reduction to maintain component pieces */
struct complist
{
	struct complist		*nextcomp;	/* next comp piece */
	struct complist		*linkcomp;	/* next clause of this comp */
	struct querytree	*clause;	/* associated clause */
	int			bitmap;		/* map of all assoc clauses */
};


/* globals used to hold original query and range table */
char			Qbuf[QBUFSIZ];		/* buffer which holds symbol lists */
struct querytree	*Qle;			/* ptr to QLEND node */
struct querytree	*Tr;			/* ptr to TREE node */


/* The following structure reserved for distributed decomp */
struct d_range
{
	int		relnum;
	long		drtupcnt;
	char		drsite[2];
	int		draltnum;
	int		drstat;
	struct d_range	*drnext;
};
