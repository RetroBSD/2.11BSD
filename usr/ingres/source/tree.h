/*
**	Structures Used In The Nodes Of The Querytree
*/

/*
**	Basic symbol structure. "Type" is one of the symbols
**	in "symbol.h", "len" is the length of the "value"
**	field (0 to 255 bytes), "value" is variable length and
**	holds the actual value (if len != 0) of the node.
*/
struct symbol
{
	char	type;			/* type codes in symbol.h */
	char	len;			/* length in bytes of value field */
	int	value[];		/* variable length (0 - 255 bytes) */
};

/*
**	Basic node in the querytree. Each node has a left and
**	right descendent. If the node is a leaf node then the
**	left and right pointers will be NULL. Depending on the
**	"type" field of the symbol structure, there may be additional
**	information. The cases are itemized below.
*/

struct querytree
{
	struct querytree	*left;
	struct querytree	*right;
	struct symbol		sym;
};


/*
**	Variable nodes of the form:
**		varno.attno
**	"Varno" is the variable number which can be translated
**	to a relation name by indexing into the range table.
**	"Attno" is the attribute number specifically the
**	"attid" field in the attribute tuple for the attribute.
**	"Frmt" and "frml" are the type and length of the attribute.
**	"Valptr" is used only in decomposition. If the variable
**	is tuple substituted then "valptr" will have a pointer to
**	the substituted value for the attribute; otherwise it will
**	be NULL. If reduction or one variable restriction has changed
**	the attribute number, then "valptr" will point to the next
**	active VAR node for this VAR.
*/

struct qt_var
{
	char	filler[6];
	char	varno;
	char	attno;
	char	frmt;
	char	frml;
	char	*valptr;
};


/*
**	Structure for ROOT, AGHEAD, and AND nodes. In the parser and qrymod
**	these nodes are of length zero and none of this information
**	is present. Decomp maintains information about the variables
**	in the left and right descendents of the nodes.
**	The "rootuser" flag is present only in the ROOT and AGHEAD nodes.
**	It is TRUE only in the original ROOT node of the query.
*/

struct qt_root
{
	char	filler[6];
	char	tvarc;		/* total of var's in sub-tree */
	char	lvarc;		/* # of variables in left branch */
	int	lvarm;		/* bit map of var's in left branch */
	int	rvarm;		/* bit map of var's in right branch*/
	int	rootuser;	/* flag: TRUE if root of user generated query */
};

struct qt_res					/* RESDOM and AOP nodes */
{
	char	filler[6];
	int	resno;			/* result domain number */
					/* type and length are referenced as
					** frmt and frml */
};

struct qt_ag
{
	char	filler[6];
	char	filler2[4];		/* fill in over aop_type, frmt, and frml */
	char	agfrmt, agfrml;		/* result format type and length for agg's */
};
struct qt_op
{
	char	filler[6];
	int	opno;			/* operator number */
};

struct qt_v
{
	int	fill[2];
	char	vtype;	/* variable type */
	char	vlen;	/* variable length */
	int	*vpoint;	/* variable pointer to value */
};
/*
** structure for referencing a SOURCEID type symbol for
**	pipe transmission
**
**	SOURCEID is a range table element.
*/
struct srcid
{
	char	type;			/* type codes in symbol.h */
	char	len;			/* length in bytes of value field */
	int	srcvar;			/* variable number */
	char	srcname[MAXNAME];	/* relation name */
	char	srcown[2];		/* relation owner usercode */
	int	srcstat;		/* relstat field */
};
