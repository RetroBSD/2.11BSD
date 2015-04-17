#
/*
**	COPYRIGHT
**
**	The Regents of the University of California
**
**	1977
**
**	This program material is the property of the
**	Regents of the University of California and
**	may not be reproduced or disclosed without
**	the prior written permission of the owner.
*/



# define	PARBUFSIZ	2000	/* size of buffer for dbu commands */
# define	TREEMAX		2500	/* max number of bytes for tree */
# define	MAXATT		150	/* max number of attributes in the att stash */

/* mode parameters for range table manipulation */
# define	LOOKREL		1
# define	LOOKVAR		2
# define	R_INTERNAL	3
# define	R_EXTERNAL	4

/* error numbers */
# define	TREEOFLO	2118	/* over flow tree buffer */
# define	PBUFOFLO	2106	/* over flow dbu arg buffer */

# define	NOATTRIN	2100	/* attrib not in relation */
# define	CANTUPDATE	2107	/* can't update rel */
# define	NOVBLE		2109	/* vble not declared */
# define	NOPATMAT	2120	/* no pattern matching in tl */
# define	RNGEXIST	2117	/* can't find rel for var */
# define	REPALL		2123	/* x.all on replace */
# define	BADCONSTOP	2134	/* bad constant operator */

# define	INDEXTRA	2111	/* too many atts in key */
# define	RESXTRA		2130	/* too many resdoms in tl */
# define	TARGXTRA	2131	/* tl larger than MAXTUP */
# define	AGGXTRA		2132	/* too many aggs */

# define	MODTYPE		2119	/* type conflict for MOD */
# define	CONCATTYPE	2121	/* type conflict for CONCAT */
# define	AVGTYPE		2125	/* type conflict for AVG(U) */
# define	SUMTYPE		2126	/* type conflict for SUM(U) */
# define	FOPTYPE		2127	/* type conflict for func ops */
# define	UOPTYPE		2128	/* type conflict for unary ops */
# define	NUMTYPE		2129	/* type conflict for numeric ops */
# define	RELTYPE		2133	/* type conflict for relatv op */

# define	RESTYPE		2103	/* result type mismatch w/expr */
# define	RESAPPEX	2108	/* append res rel not exist */
# define	RESEXIST	2135	/* result rel already exists */

# define	NXTCMDERR	2501	/* misspelt where problem */
# define	NOQRYMOD	2139	/* no qrymod in database */

# define	BADHOURS	2136	/* no such hour */
# define	BADMINS		2137	/* no such minute */
# define	BAD24TIME	2138	/* only 24:00 can be used */

/* -- ASSORTED DATA STRUCTURES -- */
struct atstash					/* attribute table */
{
	char		atbid;			/* attribute number */
	char		atbfrmt;		/* attribute form type */
	char		atbfrml;		/* attribute form length */
	char		atbname[MAXNAME];	/* attribute name */
	struct atstash	*atbnext;		/* pointer to next entry in chain */
};

struct rngtab				/* range table */
{
	int		rentno;			/* variable number of this table position */
	char		rmark;			/* was it used in this command */
	char		ractiv;			/* is entry empty */
	int		rposit;			/* lru position of entry */
	char		varname[MAXNAME + 1];	/* variable name */
	char		relnm[MAXNAME + 1];	/* relation name */
	char		relnowner[2];		/* relation owner */
	int		rstat;			/* relstat field of relation relation */
	int		ratts;			/* number of attributes in relation */
	struct atstash	*attlist;		/* head of attrib list for this reln */
};

struct constop				/* constant operator lookup table */
{
	char	*copname;		/* string name for identification */
	int	copnum;			/* op number */
	char	coptype;		/* op result type for formating */
	char	coplen;			/* op result length for formatting */
};

struct rngtab		Rngtab[MAXVAR + 1];	/* range table */
struct rngtab		*Resrng;	/* ptr to result reln entry */
struct atstash		Attable[MAXATT];	/* attrib stash space, turned into a list later */
struct atstash		*Freeatt;	/* free list of attrib stash */
struct querytree	*Tidnode;	/* pointer to tid node of targ list
					   for REPLACE, DELETE */
struct querytree	*Lastree;	/* pointer to root node of tree */
struct descriptor	Reldesc;	/* descriptor for range table lookup */
struct descriptor	Desc;		/* descriptor for attribute relation */
extern struct atstash	Faketid;	/* atstash structure for TID node */
#ifdef	DISTRIB
extern struct atstash	Fakesid;	/* atstash structure for SID node */
#endif

int			Rsdmno;		/* result domain number */
int			Opflag;		/* operator flag contains query mode */
char			*Relspec;	/* ptr to storage structure of result relation */
char			*Indexspec;	/* ptr to stor strctr of index */
char			*Indexname;	/* ptr to name of index */
char			Trfrmt;		/* format for type checking */
char			Trfrml;		/* format length for type checking */
char			*Trname;	/* pointer to attribute name */
int			Agflag;		/* how many aggs in this qry */
int			Equel;		/* indicates EQUEL preprocessor on */
int			Ingerr;		/* set if a query returns an error from processes below */
int			Patflag;	/* signals a pattern match reduction */
int			Qlflag;		/* set when processing a qual */
int			Noupdt;		/* INGRES user override of no update restriction */
extern int		yypflag;	/* disable action stmts if = 0 */
int			yyline;		/* line counter */
struct retcode		*Lastcnt;	/* ptr to last tuple count, or 0 */
struct retcode		Lastcsp;	/* space for last tuple count */
int			Dcase;		/* default case mapping */
char			**Pv;		/* ptr to list of dbu params */
char			Pbuffer[PARBUFSIZ];	/* buffer for dbu commands */
int			Pc;		/* number of dbu commands */
char			*Qbuf;		/* buffer which holds tree allocated on stack in main.c to get data space */
int			Permcomd;
int			Qrymod;		/* qrymod on in database flag */
