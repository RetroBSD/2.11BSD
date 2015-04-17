# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"../aux.h"
# include	"ovqp.h"

extern struct pipfrmt	Inpipe, Outpipe;

startovqp()

/*
**	startovqp is called at the beginning of
**	the execution of ovqp.
*/

{
	extern			flptexcep();
	extern struct pipfrmt	Eoutpipe;

	if (Equel)
		wrpipe(P_PRIME, &Eoutpipe, 0, 0, 0);	/* prime the write pipe to Equel */


	Tupsfound = 0;	/* counts the number of tuples which sat the qual */
	Retrieve = Bopen = FALSE;
	/* catch floating point signals */
	signal(8, flptexcep);
}

/*
**	Give a user error for a floating point exceptions
*/
flptexcep()
{
	ov_err(FLOATEXCEP);
}
