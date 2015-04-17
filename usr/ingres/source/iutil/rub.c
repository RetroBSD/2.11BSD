# include	"../ingres.h"

/*
**  RUBOUT SIGNAL PROCESSORS
**
**	These routines control the status of the rubout signal.
**	The external interface is through three routines:
**		ruboff(why) -- called by user to turn rubouts
**			off; 'why' is NULL or the name of a processor
**			printed in a message when an interrupt is
**			received.
**		rubon() -- called by user to turn rubouts back on.
**			If a rubout was received while they were
**			turned off it will be processed now.
**		rubproc() -- supplied by the user to do any rubout
**			processing necessary.  At minimum it must
**			call resyncpipes() to clear out the pipes.
**	There are some restrictions, notably that it is not
**	permissable to turn rubouts off when any pipe I/O might
**	occur.
*/

int		Rubgot;		/* set for rubout encountered but not processed */
extern		rubproc();	/* user rubout processing routine */
char		*Rubwhy;	/* current processor */
int		Rublevel;	/* depth of ruboff calls, -1 if off */

/*
**  RUBCATCH -- rubout catching routine.
**
**	This routine catches the signal, and either files it for future
**	reference, or processes.  Also, this routine can be called by
**	rubon to process a previously caught signal.
*/

rubcatch()
{
	signal(2, 1);

	/* check current state */
	if (Rublevel < 0)
	{
		/* error -- rubouts ignored */
		syserr("rubcatch");
	}
	else if (Rublevel > 0)
	{
		/* save the rubout for future processing */
		Rubgot++;
		if (Rubwhy != NULL)
		{
			printf("Rubouts locked out (%s in progress) -- please be patient\n",
			    Rubwhy);
		}
	}
	else
	{
		/* do real rubout processing */
		rubproc();
		signal(2, &rubcatch);
		reset();
	}
}





/*
**  TURN RUBOUTS OFF
**
**	Further rubouts will be caught by rubsave.
**	The parameter should be the name of the processor which
**	insists that rubouts do not occur.
*/

ruboff(why)
char	*why;
{
	/* check to see if this should be ignored */
	if (Rublevel < 0 || Rublevel++ > 0)
		return;

	/* set up to ignore interrupts */
	Rubgot = 0;
	Rubwhy = why;
}





/*
**  TURN RUBOUTS BACK ON
**
**	Rubout processing is restored to the norm (calling rubcatch).
**	If any rubouts have occured while disabled, they are processed
**	now.
*/

rubon()
{
	/* check to see if we should leave as-is */
	if (Rublevel < 0 || --Rublevel > 0)
		return;

	/* process any old interrupts */
	if (Rubgot > 0)
		rubcatch();

	/* reset state */
	Rubwhy = NULL;
}
