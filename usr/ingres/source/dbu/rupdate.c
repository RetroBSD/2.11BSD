# include	"../ingres.h"

/*
**  RUBOUT SETUP FOR DEFFERED UPDATE PROCESSOR
**
**	These routines setup the special processing for the rubout
**	signal for the deferred update processor.  The update
**	processor is then called.
*/

rupdate(pc, pv)
int	pc;
char	**pv;
{
	register int	rtval;

	/* set up special signal processing */
	ruboff("batch update");

	/* call update */
	rtval = update(pc, pv);

	/* clean up signals */
	rubon();

	return (rtval);
}
