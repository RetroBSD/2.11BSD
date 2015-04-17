#
/*
**	This header file contains the external (global) declarations
**	of variables and structures as well as the manifest constants
**	particular to OVQP.
**
**	By convention global variable identifiers are spelled with 
**	an initial capital letter; manifest constants are in caps
**	completely.
**
*/



/*
**	Manifest constants
*/
   
  
# define	tTFLAG		'O'	/* trace flag */

# define	LBUFSIZE	850	/* buffer size for holding query list */
					/* and concat and ascii buffers */
# define	NSIMP		15	/*maximum no. of "simple clauses" 
					 * allowed in Qual list
					 * (see "strategy" portion) */
# ifndef	STACKSIZ
# define	STACKSIZ	20	/* Stack size for interpreter */
# endif	
# define	MAXNODES	(2 * MAXDOM) + 50	/* max nodes in Qvect */

/* symbolic values for GETNXT parameter of fcn GET */
# define	CURTUP	0	/* get tuple specified by tid */
# define	NXTTUP	1	/* get next tuple after one specified by tid */
  

/* symbolic values for CHECKDUPS param of fcn INSERT */
# define	DUPS	0	/* allow a duplicate tuple to be inserted */
# define	NODUPS	1	/* check for and avoid inserting 
				 * a duplicate (if possible)*/


# define	TIDTYPE		INT
# define	TIDLEN		4

# define	CNTLEN 		4	/* counter for aggregate computations */
# define	CNTTYPE 	INT	/* counter type */

# define	ANYLEN		2	/* length for opANY */
# define	ANYTYPE		INT	/* type for opANY */

/* error codes for errors caused by user query ie. not syserrs */

# define	LISTFULL	4100	/* postfix query list full */
# define	BADCONV		4101	/* */
# define	BADUOPC		4102	/* Unary operator not allowed on char fields */
# define	BADMIX		4103	/* can't assign, compare or operate a numberic with a char */
# define	BADSUMC		4104	/* can't sum char domains (aggregate) */
# define	BADAVG		4105	/* can't avg char domains (aggregate) */
# define	STACKOVER	4106	/* interpreter stack overflow */
# define	CBUFULL		4107	/* not enough space for concat or ascii operation */
# define	BADCHAR		4108	/* arithmetic operation on two character fields */
# define	NUMERIC		4109	/* numeric field in a character operator */
# define	FLOATEXCEP	4110	/* floating point exception */
# define	CHARCONVERT	4111	/* bad ascii to numeric conversion */
# define	NODOVFLOW	4112	/* node vector overflow */
# define	BADSECINDX	4199	/* found a 6.0 sec index */

# define	cpderef(x)	(*((char **)(x)))

char		Outtup[MAXTUP];
char		Intup[MAXTUP];
char		*Origtup;
long		Intid;
long		Uptid;

int		Ov_qmode;	/* flag set to indicate mode of tuple disposition */
extern int	Equel;	/* equel flag set by initproc */



int		Bopen;			/* TRUE if batch file is open */

struct descriptor	*Scanr,		/* pts to desc of reln to be scanned,
					 * (i.e. either Srcdesc or Indesc) */
			*Source,	/* 0 if no source for qry, else points to Srcdesc */
			*Result;	/* 0 if no result for qry, else points to Reldesc */


long		*Counter;	/* cnts "gets" done in OVQP */
char		*Tend;		/* pts to end of data in Outtup */

struct symbol	**Tlist,
		**Alist,
		**Qlist,
		**Bylist;

int		Newq;			/* flags new user query to OVQP */
long		Lotid, Hitid;		/* lo & hi limits of scan in OVQP */

struct stacksym
{
	char	type;
	char	len;
	int	value[4];
}	Stack[STACKSIZ];	/* stack for OVQP interpreter */


int	Buflag;		/* flags a batch update situation (Ov_qmode != mdRETR) */
int	Targvc;		/* var count in Target list (flags constant Targ. list) */
int	Qualvc;		/* var count in Qual list */
int	Userqry;	/* flags a query on the users's result rel */
int	Retrieve;	/* true is a retrieve, else false */
int	Diffrel;	/* true is Source and Result are different */
int	Agcount;	/* count of the # of aggregates in the query */
long	Tupsfound;	/* counts # tuples which satified the query */
int	R_decomp;	/* file for reading info from decomp */
int	W_decomp;	/* file for writing to decomp */
struct symbol	*Qvect [MAXNODES];
int		Qvpointer;
