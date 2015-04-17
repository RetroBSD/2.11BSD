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



#
/*
**	conditional compilation flags
**
**	Some of these may be commented out to set them to "off".
**	The comment should be removed to set them to "on".
*/

/* access methods compilation flags */

/*	disable timing information
# define	xATM		/* timing information */
# define	xATR1		/* trace info, level 1 */
# define	xATR2		/* trace info, level 2, implies xTR1 */
# define	xATR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* decomposition compilation flags */

/*	disable timing information
# define	xDTM		/* timing information */
# define	xDTR1		/* trace info, level 1 */
# define	xDTR2		/* trace info, level 2, implies xTR1 */
# define	xDTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* EQUEL compilation flags */

/*	disable timing information
# define	xETM		/* timing information */
# define	xETR1		/* trace info, level 1 */
# define	xETR2		/* trace info, level 2, implies xTR1 */
# define	xETR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* monitor compilation flags */

/*	disable timing information
# define	xMTM		/* timing information */
# define	xMTR1		/* trace info, level 1 */
# define	xMTR2		/* trace info, level 2, implies xTR1 */
# define	xMTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* OVQP compilatiion flags */

/*	disable timing information
# define	xOTM		/* timing information */
# define	xOTR1		/* trace info, level 1 */
# define	xOTR2		/* trace info, level 2, implies xTR1 */
# define	xOTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* parser compilation flags */

/*	disable timing information
# define	xPTM		/* timing information */
# define	xPTR1		/* trace info, level 1 */
# define	xPTR2		/* trace info, level 2, implies xTR1 */
# define	xPTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* qrymod compilation flags */

/*	disable timing information
# define	xQTM		/* timing information */
# define	xQTR1		/* trace info, level 1 */
# define	xQTR2		/* trace info, level 2, implies xTR1 */
# define	xQTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* scanner compilation flags */

/*	disable timing information
# define	xSTM		/* timing information */
# define	xSTR1		/* trace info, level 1 */
# define	xSTR2		/* trace info, level 2, implies xTR1 */
# define	xSTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* DBU compilation flags */

/*	disable timing information
# define	xZTM		/* timing information */
# define	xZTR1		/* trace info, level 1 */
# define	xZTR2		/* trace info, level 2, implies xTR1 */
# define	xZTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/* support compilation flags */

/*	disable timing information
# define	xTTM		/* timing information */
# define	xTTR1		/* trace info, level 1 */
# define	xTTR2		/* trace info, level 2, implies xTR1 */
# define	xTTR3		/* trace info, level 3, implies xTR1 & xTR2 */

/*
**	INGRES manifest constants
**
**	These constants are manifest to the operation of the entire
**	system.  If anything
**	is changed part or all of the system will stop working.
**	The values have been carefully chosen and are not intended
**	to be modifiable by users.
*/

# define	MAXDOM		50		/* maximum number+1 of domains in a relation */
# define	MAXTUP		498		/* max size (in bytes) of a tuple */
# define	MAXNAME		12		/* max size of a name (in bytes) */
# define	MAXVAR		10		/* max # of variables */
# define	MAXKEYS		6		/* max # of keys in secondary index */
# define	MAXAGG		50		/* max number of aggs in a qry */
# define	MAXPARMS	MAXDOM * 2 + 20 /* max number of parameters
						** to the DBU controller
						** (allows for 2 per domain for
						** create which is the worst case
						*/
# define	STACKSIZ	20		/* max depth for arith. expr. stacks */
# define	I1MASK		0377		/* mask out sign extension that occurs
						**  when a c1 or i1 field is converted
						**  to an i2 field. 
						*/

# define	TRUE		1		/* logical one, true, yes, ok, etc.*/
# define	FALSE		0		/* logical zero, false, no, nop, etc. */
# ifndef	NULL
# define	NULL		0
# endif

# define	i_1		char
# define	i_2		int
# define	i_4		long
# define	c_1		char
# define	c_2		char
# define	c_12		char

/*
**	RELATION relation struct
**
**	The RELATION relation contains one tuple for each relation
**	in the database.  This relation contains information which
**	describes how each relation is actually stored in the
**	database, who the owner is, information about its size,
**	assorted operation information, etc.
*/

# define	RELID		1	/* domain for setkey */
# define	RELOWNER	2

/*
**	Note carefully!
**
**	Do not change this struct without changing the exact copy
**	of it occuring at the beginning of the descriptor struct!
*/

struct relation
{
	c_12	relid[MAXNAME];	/* relation name	*/
	c_2	relowner[2];	/* code of relation owner */
	i_1	relspec;	/* storage mode of relation	*/
				/* M_HEAP  unsorted paged heap	*/
				/* -M_HEAP compressed heap	*/
				/* M_ISAM  isam			*/
				/* -M_ISAM compressed isam	*/
				/* M_HASH  hashed		*/
				/* -M_HASH compressed hash	*/
	i_1	relindxd;	/* -1 rel is an index, 0 not indexed, 1 indexed */
	i_2	relstat;	/* relation status bits */
	i_4	relsave;	/*unix time until which relation is saved*/
	i_4	reltups;	/*number of tuples in relation	*/
	i_2	relatts;	/*number of attributes in relation	*/
	i_2	relwid;		/*width (in bytes) of relation	*/
	i_4	relprim;	/*no. of primary pages in relation*/
	i_4	relspare;	/*not used yet*/
};


/*
**	ATTRIBUTE relation struct
**
**	The ATTRIBUTE relation contains one tuple for each domain
**	of each relation in the database.  This relation describes
**	the position of each domain in the tuple, its format,
**	its length, and whether or not it is used in part of the key.
*/

# define	ATTRELID	1
# define	ATTOWNER	2
# define	ATTNAME		3
# define	ATTID		4


struct attribute
{
	c_12 	attrelid[MAXNAME];	/*relation name of which this is an attr */
	c_2	attowner[2];	/* code of relation owner */
	c_12	attname[MAXNAME];	/*alias for this domain*/
	i_2	attid;		/*domain number (from 1 to relatts)	*/
	i_2	attoff;		/*offset in tuple (no. of bytes*/
	i_1	attfrmt;	/* INT, FLOAT, CHAR (in symbol.h) */
	i_1	attfrml;	/* unsigned integer no of bytes	*/
	i_1	attxtra;	/* flag indicating whether this dom is part of a key */
};

/*
**	DESCRIPTOR struct
**
**	The DESCRIPTOR struct is initialized by OPENR to describe any
**	open relation.  The first part of the descriptor is the tuple
**	from the RELATION relation.  The remainder contains some magic
**	numbers and a template initialized from the ATTRIBUTE relation.
*/

struct descriptor
{
	c_12	relid[MAXNAME];	/* relation name	*/
	c_2	relowner[2];	/* code of relation owner */
	i_1	relspec;	/* storage mode of relation	*/
				/* M_HEAP  unsorted paged heap	*/
				/* -M_HEAP compressed heap	*/
				/* M_ISAM  isam			*/
				/* -M_ISAM compressed isam	*/
				/* M_HASH  hashed		*/
				/* -M_HASH compressed hash	*/
	i_1	relindxd;	/* -1 rel is an index, 0 not indexed, 1 indexed */
	i_2	relstat;	/* relation status bits */
	i_4	relsave;	/*unix time until which relation is saved*/
	i_4	reltups;	/*number of tuples in relation	*/
	i_2	relatts;	/*number of attributes in relation	*/
	i_2	relwid;		/*width (in bytes) of relation	*/
	i_4	relprim;	/*no. of primary pages in relation*/
	i_4	relspare;	/*not used yet*/
		/*the above part of the descriptor struct is identical
		  to the relation struct and the information in this
		  part of the struct is read directly from the
		  relation tuple by openr.  the rest of the descriptor
		  struct is calculated by openr.
		*/
	i_2	relfp;		/*filep for relation , if open	*/
	i_2	relopn;		/*indicates if relation is really open*/
	i_4	reltid;		/*when relation is open, this indicates
				  the tid in the relation relation for
				  this relation */
	i_4	reladds;	/*no. of additions of tuples during this open*/
	i_2	reloff[MAXDOM];	/*reloff[i] is offset to domain i 	*/
	c_1	relfrmt[MAXDOM]; /* format of domain i
				 ** INT, FLOAT, or CHAR  */
	c_1	relfrml[MAXDOM]; /* relfrml[i] is an unsigned integer
				  which indicates length
				  in bytes of domain
					*/
	c_1	relxtra[MAXDOM]; /*relxtra[i] is none zero if domain i is
				 ** a key domain for the relation */
	c_1	relgiven[MAXDOM]; /*cleared by openr and set before
				  call to find to indicate value of this
				  domain has been supplied in the key*/
};

# define	NOKEY		1	/* scan entire relation */
# define	EXACTKEY	2
# define	LRANGEKEY	3	/* low range key */
# define	FULLKEY		4	/* forces full key comparison */
# define	HRANGEKEY	5	/* high range key */

/*
**	tuple id struct
*/

struct tup_id
{
	char	pg1, pg0;
	char	line_id, pg2;
};
