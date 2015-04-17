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


/*
**	Accessparam structure -- this structure is filled by
**	the paramd() and parami() routines. It gives a list of
**	key domains in their key sequence order.
*/

struct accessparam
{
	int	mode;		/* mode of relation, NOKEY, RANGEKEY, EXACTKEY */
	int	sec_index;	/* TRUE if relation is a secondary index. else FALSE */
	char	keydno[MAXDOM + 1];
};

/*
**  RETCODE STRUCTURE
**	This structure is passed back after each query to the front end
**	to give query status.
*/

struct retcode
{
	long	rc_tupcount;	/* count of tuples which satisfied */
	int	rc_status;	/* result status for the query */
	char	rc_siteid[2];	/* reserved for distributed decomp */
};




/*
**	Desxx structure -- This structure is used by opencatalog and
**	closecatalog. It is used to maintain a list of system relations
**	for caching.
*/

struct desxx
{
	char			*cach_relname;	/* name of the relation */
	struct descriptor	*cach_desc;	/* desc to use */
	struct descriptor	*cach_alias;	/* alias for above descriptor */};


/*
**  Relation status bits
**
**	These bits are in the relation relation, in the "relstat"
**	field.  They define status information about the relation.
*/

# define	S_CATALOG	0000001		/* system catalog */
# define	S_NOUPDT	0000002		/* no update allowed */
# define	S_PROTUPS	0000004		/* tuples exist in 'protect' */
# define	S_INTEG		0000010		/* integrity constrained */
# define	S_CONCUR	0000020		/* concurrency enforced */
# define	S_VIEW		0000040		/* relation is a view */
# define	S_VBASE		0000100		/* base rel for a view */
# define	S_INDEX		0000200		/* is a sec indx */
# define	S_BINARY	0000400		/* print char domains in binary */
# define	S_DISTRIBUTED	0001000		/* reserved for distributed rels */
# define	S_DISCRIT	0002000		/* resrved for distr temp rel */
# define	S_DISCOPY	0004000		/* reserved for distributed rels */
# define	S_PROTALL	0010000		/* if clear, all permission */
# define	S_PROTRET	0020000		/* if clear, read permission */

/*
**  Protection bits are defined as follows:
**
**	S_PROTUPS -- if set, there are tuples for this relation in the
**
**	S_PROTALL, S_PROTRET -- AS shown from the following table:*		protect catalog.
**      PROTALL  PROTRET     meaning
**            1  1        Need to look in the protect catalog to tell.
**            1  0        Permit RETRIEVE to ALL case.
**            0  1        Permit ALL to ALL case.
**            0  0        Permit ALL to ALL and RETRIEVE to ALL.
*/

/*
**  User status bits
**
**	These bits are in the status field of the users file.  They end
**	up in a variable "Status" after a call to initucode.
*/

# define	U_CREATDB	0000001		/* can create data bases */
# define	U_DRCTUPDT	0000002		/* can specify direct update */
# define	U_UPSYSCAT	0000004		/* can update system catalogs directly */
# define	U_TRACE		0000020		/* can use trace flags */
# define	U_QRYMODOFF	0000040		/* can turn off qrymod */
# define	U_APROCTAB	0000100		/* can use arbitrary proctab */
# define	U_EPROCTAB	0000200		/* can use =proctab form */
# define	U_SUPER		0100000		/* ingres superuser */

/*
**	The following defines declare the field number in the users
**	file for each field.
*/

# define	UF_NAME		0		/* login name */
# define	UF_UCODE	1		/* user code */
# define	UF_UID		2		/* UNIX user id */
# define	UF_GID		3		/* UNIX group id */
# define	UF_STAT		4		/* status bits */
# define	UF_FLAGS	5		/* default flags */
# define	UF_PTAB		6		/* default proctab */
# define	UF_IFILE	7		/* monitor init file */
# define	UF_DBLIST	9		/* list of valid databases */

# define	UF_NFIELDS	10		/* TOTAL number of fields */

/*
**	Usercode contains the current user's INGRES user-id code.
**	Pathname contains the name of the INGRES subtree.
*/

extern char	*Usercode;
extern char	*Pathname;



/*
**	This is for type conversion, to avoid ugly pointer
**	casts all over the place:
*/

# define	i1deref(x)	(*((char *)(x)))
# define	i2deref(x)	(*((int *)(x)))
# define	i4deref(x)	(*((long *)(x)))
# define	f4deref(x)	(*((float *)(x)))
# define	f8deref(x)	(*((double *)(x)))


/*
**  PRINTED OUTPUT ARGUMENTS
**
**	The following struct describes the printed output available
**	to the user.
*/

struct out_arg
{
	int	c0width;	/* minimum width of "c" field */
	int	i1width;	/* width of "i1" field */
	int	i2width;	/* width of "i2" field */
	int	i4width;	/* width of "i4" field */
	int	f4width;	/* width of "f4" field */
	int	f8width;	/* width of "f8" field */
	int	f4prec;		/* number of decimal places on "f4" */
	int	f8prec;		/* number of decimal places on "f8" */
	char	f4style;	/* "f4" output style */
	char	f8style;	/* "f8" output style */
	int	linesperpage;	/* number of lines per output page */
	char	coldelim;	/* column delimiter */
};

/* maximum width of any of the above fields */
# define	MAXFIELD	255

/*
**  any text line read from a file (for example, .../files/users) can
**	be at most MAXLINE bytes long.  buffers designed for holding
**	such info should be decleared as char buf[MAXLINE + 1] to allow
**	for the null terminator.
*/

# define	MAXLINE		256


/*
**  Assorted system stuff
**
**	FILEMODE is the file mode on a 'creat' call for all files in
**		the database and probably other external files.
*/

# define	FILEMODE	0600		/* db file mode */

/*
**	defines to specify the execid of each process in the
**	current process structure.
**	The constants are named with "EXEC_" concatenated with
**	the process name.
*/
# define	EXEC_DBU	'#'	/* data base utilities (or overlays) */
# define	EXEC_DECOMP	'$'	/* decomposition process */
# define	EXEC_OVQP	'&'	/* one variable query processor */
# define	EXEC_QRYMOD	'*'	/* query modification for view, protection, integrity */
# define	EXEC_PARSER	'@'	/* parser, scanner */
# define	EXEC_FRONT	'^'	/* could be equel prog or terminal monitor */
# define	EXEC_ERROR	'%'	/* exec_id of an error block */

/* stuff giving information about the machine */
# define	WORDSIZE	16	/* number of bits in word */
# define	LOG2WORDSIZE	4	/* log base 2 of WORDSIZE */
