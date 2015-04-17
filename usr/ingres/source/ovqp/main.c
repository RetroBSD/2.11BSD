# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../batch.h"
# include	"../pipes.h"
# include	"ovqp.h"

int		Batchupd;
struct pipfrmt	Inpipe, Outpipe;

main(argc, argv)
int	argc;
char	**argv;
{
	register int	i;
	register char	execid;
	register char	funcid;
	extern		proc_error();

#	ifdef xOTR1
	tTrace(&argc, argv, 'O');
#	endif
	initproc("OVQP", argv);
	R_decomp = R_down;
	W_decomp = W_down;
	Batchupd = setflag(argv, 'b', 1);
	acc_init();	/* init access methods */

	setexit();

	for (;;)
	{

		/* copy the message from parser to decomp */
		rdpipe(P_PRIME, &Inpipe);
		execid = rdpipe(P_EXECID, &Inpipe, R_up);
		funcid = rdpipe(P_FUNCID, &Inpipe, R_up);
		wrpipe(P_PRIME, &Outpipe, execid, 0, funcid);
		copypipes(&Inpipe, R_up, &Outpipe, W_down);

		/* do decomp to ovqp processing */
		if (execid == EXEC_DECOMP)
		{
			rdpipe(P_PRIME, &Inpipe);
			execid = rdpipe(P_EXECID, &Inpipe, R_down);
			funcid = rdpipe(P_FUNCID, &Inpipe, R_down);
			if (execid == EXEC_OVQP)
			{
				ovqp();
				flush();
			}
		}

		/* finish ovqp msg -or- do dbu return stuff */
		copyreturn();
	}
}



rubproc()
{
	extern int	Equel;
	struct pipfrmt	pipebuf;

	/* all relations must be closed before decomp
	** potentially destroys any temporaries
	*/
	if (Bopen)
	{
		rmbatch();	/* remove the unfinished batch file */
		Bopen = FALSE;
	}
	closeall();
	unlall();	/* remove any outstanding locks */

	/* if equel then clear equel data pipe */
	if (Equel)
		wrpipe(P_INT, &pipebuf, W_front);
	resyncpipes();

	flush();
}
