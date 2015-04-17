# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"


/*
**  TRAPQUERY -- trap queries which succeeded.
**
**	This module traps the current query into the file
**	specified by Trapfile.  It will open Trapfile if
**	it is not already open.
**
**	Parameters:
**		retcode -- the return code of the query.
**		name -- the name of the file in which to dump the
**			query buffer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Outputs the query buffer to Trapfile.
**
**	Called By:
**		go.
**
**	Files:
**		Trapfile -- the descriptor of the file in which to
**			trap the query.
**		Qbname -- the name of the query buffer.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Bugs:
**		If 'name' (that is, the {querytrap} macro) changes
**			during a run, the output file does not change.
**
**	History:
**		3/2/79 -- Grabbed from michael.
*/

FILE	*Trapfile;

trapquery(retcode, name)
struct retcode	*retcode;
char		*name;
{
	register FILE	*iop;
	static int	first;
	register char	*sp, c;
	int		timevec[2];
	extern		fgetc();
	char		*ctime();
	char		*locv();

	if (first < 0)
		return;
	if (Trapfile == NULL)
	{
		if ((Trapfile = fopen(name, "a")) == NULL)
		{
			printf("can't trap query in %s\n", name);
			first = -1;
			return;
		}
	}
	if (first == 0)
	{
		time(timevec);
		sp = ctime(timevec);
		while (*sp)
			putc(*sp++, Trapfile);
		first++;
	}

	if ((iop = fopen(Qbname, "r")) == NULL)
		syserr("go: open 1");
	macinit(&fgetc, iop, 1);

	while ((c = macgetch()) > 0)
		putc(c, Trapfile);

	if (retcode->rc_status == RC_OK)
	{
		sp = locv(retcode->rc_tupcount);
		while (*sp)
			putc(*sp++, Trapfile);
		putc('\n', Trapfile);
	}

	fclose(iop);
}
