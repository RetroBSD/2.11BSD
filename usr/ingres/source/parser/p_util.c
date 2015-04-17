# include	"../ingres.h"
# include	"../aux.h"
# include	"../tree.h"
# include	"parser.h"
# include	"../catalog.h"
# include	"../symbol.h"

/*
**  P_UTIL -- parser utility functions
**
**	These functions are generally unrelated except that they are
**	needed to operate the parser and are too small to be considered
**	seperate modules.
**
**	Defined Constants:
**
**	Defines:
**		timeofday	-- convert arguments to minutes since midnight
**		tlprepend	-- attach two target list components
**		header		-- prints the header for a retrieve to terminal
**		patmat		-- converts pattern matching characters in a string
**		permcom		-- adds a command to the permit command vector
**
**	Requires:
**		nothing
**
**	Required By:
**		y.tab.c		-- the grammar
**
**	Files:
**		none
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		none at present
**
**	History:
**		20 Dec 1978	-- written (rick)
*/








/*
**  TIMEOFDAY -- convert 2 integers to minutes since midnight
**
**	Converts the hours and minutes parameters to minutes since midnight
**	performing some error (bounds) checking on the time.
**
**	To answer the question about what is midnight, both 0:00 and 24:00
**	are handled, but not the same way.  The former is zero minutes from
**	midnight and the latter is 1440 minutes from midnight.  (1440 is
**	24 hrs times 60 minutes, or 1 minute past the end of the day.)
**
**	Parameters:
**		hrs		-- an integer pointer to the hour
**		mins		-- an integer pointer to the minutes
**
**	Returns:
**		integer time since midnight
**
**	Side Effects:
**		may detect an error and call yyerror which never returns.
**
**	Requires:
**		that the pointers be on integer boundaries
**
**	Called By:
**		y.tab.c		-- the grammar
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		BADHOURS	-- No such hour
**		BADMINS		-- No such minute
**		BAD24TIME	-- only 24:00 allowed
**
**	Syserrs:
**		none
**
**	History:
**		20 Dec 1978 	-- written (rick)
*/
timeofday(hrs, mins)
int	*hrs;
int	*mins;
{
	register int	h;
	register int	m;
	register int	rtval;

	h = *hrs;
	m = *mins;
	if (h > 24 || h < 0)
		/* no such hour */
		yyerror(BADHOURS, iocv(h), 0);
	if (m > 59 || h < 0)
		/* no such minute */
		yyerror(BADMINS, iocv(m), 0);
	if (h == 24)
	{
		h = 1440;
		if (m != 0)
			/* can only use 24:00 */
			yyerror(BAD24TIME, iocv(m), 0);
	}
	rtval = (h * 60) + m;
	return (rtval);
}


/*
**  TLPREPEND -- combine two target list components
**
**	Attach two target list components to each other.
**	Neither component need be a single element.  The
**	'a' component will be attached at the extreme left
**	of the 'b' component.
**
**	Parameters:
**		a		-- tl component to attach
**		b		-- tl base for attaching
**
**	Returns:
**		nothing
**
**	Side Effects:
**		this routine is a side effect.  It attaches a to b
**		and when it returns a is attached to b but the pointer
**		to b never changes (neither does the pointer to a)
**
**	Requires:
**		nothing
**
**	Called By:
**		y.tab.c		-- the grammar
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		20 Dec 1978	-- written (rick)
*/

struct querytree *
tlprepend(a, b)
struct querytree	*a;
struct querytree	*b;
{
	register struct querytree	*q;

	/* scan to the left end of b */
	for (q = b; q->left != NULL; q = q->left)
		;	/* no action */
	
	/* attach a to the end of b */
	q->left = a;
	return (b);
}






/*
**  HEADER.C -- print header for retrieve to terminal
**
**	header scans the parameter list managed with "initp" and
**	"setp" to reconstruct the field names and types and passing
**	them to the normal printhdr etc.
**
**	Defines:
**		header()
**
**	Requires:
**		printhdr	- utility lib
**		beginhdr	- utility lib
**		printeol	- utility lib
**		printeh		- utility lib
**		initp		- soon to become utility lib
**		atoi		- utility lib
**		Pc		- vble, number of params in list
**		Pv		- vble, list of parameters
**
**	History:
**		written (ancient history) (rick)
*/
header()
{
	register char	*ptr;
	register int	i;
	int		j;

	beginhdr();
	for (i = 0; i < Pc; i += 2)
	{
		atoi(&Pv[i + 1][1], &j);
		printhdr(Pv[i + 1][0] & I1MASK, j, Pv[i]);
	}
	printeol();
	printeh();
	initp();
}






/*
**  PATMAT -- converts pattern matching characters in a string
**
**	Searches a string up to a null byte for one of the pattern
**	matching characters '*', '?', '[', and ']'. It then converts
**	these characters to their internal control character equivalents.
**
**	Parameters:
**		str		-- the string to search
**
**	Returns:
**		0		-- no pattern matching in string
**		1		-- at least one pattern matching character
**
**	Side Effects:
**		none
**
**	Requires:
**		symbol.h
**
**	Called By:
**		y.tab.c		-- grammar
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		written (ancient history) (rick)
*/


/*
** PATMAT
**	hunts through a string and converts the pattern matching
**	characters and replaces with the corresponding cntrl chars
*/
patmat(str)
char	*str;
{
	register char	*p, *q;
	register int	flag;

	flag = 0;
	q = str;
	for (p = str; *p; p++)
	{
		if (*p == '\\')
		{
			*q++ = *++p;
			continue;
		}
		switch (*p)
		{
		  case '*':
			*q++ = PAT_ANY;
			flag = 1;
			continue;

		  case '?':
			*q++ = PAT_ONE;
			flag = 1;
			continue;

		  case '[':
			*q++ = PAT_LBRAC;
			flag = 1;
			continue;

		  case ']':
			*q++ = PAT_RBRAC;
			flag = 1;
			continue;
		}
		*q++ = *p;
	}
	*q = '\0';
	return (flag);
}
/*
**  PERMCOM -- map command allowed into protection catalog bits
**
**	translates the QMODE type symbols into the appropriate counterparts
**	for the permit statement allowed command vector.  The manifest
**	constants are designed to be inclusive or'd together to form a
**	composite bit map of OK actions.
**
**	Parameters:
**		a		-- the QMODE type symbol for the command to add
**
**	Returns:
**		none
**
**	Side Effects:
**		changes the variable Permcomd to reflect the additional permission
**
**	Requires:
**		Permcomd must be define globally
**		catalog.h for the proper constants
**
**	Called By:
**		y.tab.c		-- the grammar
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		bad QMODE(%d)	-- a bad symbol has been passed for mapping
**
**	History:
**		28 Dec 1978	-- written (rick)
*/

permcom(a)
int	a;
{
	switch (a)
	{
	  case mdRETR:
		Permcomd |= PRO_RETR;
		break;

	  case mdAPP:
		Permcomd |= PRO_APP;
		break;
	
	  case mdREPL:
		Permcomd |= PRO_REPL;
		break;

	  case mdDEL:
		Permcomd |= PRO_DEL;
		break;

	  case -1:
		Permcomd |= 0177777;		/* all bits set */
		break;
	
	  default:
		syserr("permcom: bad QMODE(%d)", a);
	}
}
