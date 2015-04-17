# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../batch.h"
# include	"../pipes.h"
# include	"ovqp.h"

copyreturn()
{
	extern struct pipfrmt	Inpipe, Outpipe;
	register char		exec_id, func_id;

	/* copy information from decomp back to parser */
	rdpipe(P_PRIME, &Inpipe);
	wrpipe(P_PRIME, &Outpipe, EXEC_PARSER, 0, 0);
	copypipes(&Inpipe, R_down, &Outpipe, W_up);
}
