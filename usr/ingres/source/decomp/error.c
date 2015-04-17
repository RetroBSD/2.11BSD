# include	"../ingres.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"


derror(eno)
{
	extern struct pipfrmt	Outpipe;

	endovqp(NOACK);
	error(eno, 0);
	if (eno == QBUFFULL)
		clearpipe();	/* if overflow while reading pipe, flush the pipe */
	reinit();
	writeback(0);
	reset();
}



clearpipe()
{
	extern struct pipfrmt Inpipe;

	rdpipe(P_SYNC, &Inpipe, R_up);
}
