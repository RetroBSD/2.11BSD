# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../pipes.h"
# include	"decomp.h"
# include	"../unix.h"

# define	tTFLAG	'D'

struct pipfrmt 	Inpipe, Outpipe;

int		Batchupd;

main(argc,argv)
int	argc;
char	*argv[];
{
	register int 	i;
	register char	exec_id, func_id;

	tTrace(&argc, argv, tTFLAG);

	initproc("DECOMP", argv);

	Batchupd = setflag(argv, 'b', 1);

	acc_init();

	R_ovqp = R_up;
	W_ovqp = W_up;
	R_dbu = R_down;
	W_dbu = W_down;

	/*
	** Do the necessary decomp initialization. This includes
	** buffering standard output (if i/d system) and giving
	** access methods more pages (if i/d system).
	** init_decomp is defined in either call_ovqp or call_ovqp70.
	*/
	init_decomp();

	setexit();

	for (;;)
	{
		rdpipe(P_PRIME, &Inpipe);
		exec_id = rdpipe(P_EXECID, &Inpipe, R_up);
		func_id = rdpipe(P_FUNCID, &Inpipe, R_up);
		if (exec_id == EXEC_DECOMP)
		{
			qryproc();
			continue;
		}

		/* copy pipes on down */
		wrpipe(P_PRIME, &Outpipe, exec_id, 0, func_id);
		copypipes(&Inpipe, R_up, &Outpipe, W_down);

		/* read response and send eop back up to parser */
		writeback(-1);
	}

}

/*
**  RUBPROC -- process a rubout signal
**
**	Called from the principle rubout catching routine
**	when a rubout is to be processed. Notice that rubproc
**	must return to its caller and not call reset itself.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		reinitializes the state of the world.
**
**	Called By:
**		rubcatch
*/


rubproc()
{
	struct pipfrmt	pipebuf;
	extern int	Equel;

	/*
	** Sync with equel if we have the equel pipe.
	**	This can happen only if ovqp and decomp
	**	are combined.
	*/
	if (W_front >= 0 && Equel)
		wrpipe(P_INT, &pipebuf, W_front);

	flush();
	resyncpipes();
	endovqp(RUBACK);
	reinit();
	return;
}
