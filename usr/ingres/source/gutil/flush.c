# include	<stdio.h>

/*
**  FLUSH -- flush standard output and error
**
**	Parameters:
**		none
**
**	Return:
**		none
**
**	Requires:
**		fflush()
**
**	Defines:
**		flush()
**
**	Called by:
**		general user
**
**	History:
**		2/27/78 (eric) -- changed to flush only standard output
**			and error.
**		1/4/78 -- written by eric
*/

flush()
{
	fflush(stdout);
	fflush(stderr);
}
