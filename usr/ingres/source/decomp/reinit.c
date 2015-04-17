# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../lock.h"

/*
** REINIT -- reinitialize decomp upon end of query, error, or interrupt.
**	All open relations are closed, temp relations destroyed,
**	and relation locks released.
*/

reinit()
{
	closers();
	if (Lockrel)	/* release all relation locks */
		unlall();
}
