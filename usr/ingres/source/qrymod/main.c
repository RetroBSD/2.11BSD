# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"qrymod.h"
# include	"../unix.h"


/*
**  QRYMOD -- query modification process
**
**	This process modifies queries to implement views, protection,
**	and integrity.
**
**	Positional Parameters:
**		standard
**
**	Flags:
**		none
**
**	Files:
**		'tree' relation -- stores trees for use by all
**			subsections.
**		'view' relation -- stores maps between views and
**			base relations.  In fact, this is not needed
**			by query modification, but only by destroy
**			and help.
**		'protect' relation -- stores information to tell
**			which protection constraint is appropriate
**			at a given time.
**		'constraint' relation -- maintains mappings between
**			relations and trees in the 'tree' catalog;
**			the qualifications on these trees must remain
**			true at the completion of every query.
**		'relation' relation -- the relstat field contains
**			useful information for several of the commands.
**			For views, most of the domains are ignored
**			by the rest of the system.
**
**	Return Codes:
**		standard
**
**	Defines:
**		Pipe -- the input pipe block.
**		Outpipe -- the output pipe block.
**		Terminal -- the current terminal id.
**		Reldes -- the relation relation descriptor.
**		Treedes -- the tree relation descriptor.
**		Prodes -- the protection relation descriptor.
**		Nullsync -- set if we should send a null query on an
**			error (to free up an EQUEL program).
**		main()
**		rubproc()
**
**	Requires:
**		define -- to define new constraints.
**		readqry -- to read and build a query with tree.
**		qrymod -- to do query modification.
**		issue -- to issue a query.
**		copypipes -- to copy dbu requests through.
**		rdpipe, wrpipe
**		tTrace, initproc -- for initialization
**		setexit -- for disasters (such as rubout).
**		openr -- to open the 'tree' catalog for use by
**			'define' and 'qrymod'.
**		ttyn -- to get the current terminal id.
**		acc_init -- to initialize the DBA code.
**
**	Trace Flags:
**		0
**
**	Diagnostics:
**		none
**
**	History:
**		2/14/79 -- version 6.2 release.
*/



struct pipfrmt		Pipe, Outpipe;
# ifndef xV7_UNIX
char			Terminal;	/* terminal descriptor */
# endif
# ifdef xV7_UNIX
char			Terminal[3];	/* terminal id */
# endif
struct descriptor	Prodes;		/* protection catalog descriptor */
struct descriptor	Reldes;		/* relation catalog descriptor */
struct descriptor	Treedes;	/* tree catalog descriptor */
struct descriptor	Intdes;		/* integrity catalog descriptor */
int			Nullsync;	/* send null qry on error */
extern int		Equel;		/* equel flag */
char			*Qbuf;		/* the query buffer */



main(argc, argv)
int	argc;
char	**argv;
{
	extern QTREE		*readqry();
	extern QTREE		*qrymod();
	extern int		pipetrrd();
	extern int		relntrrd();
	register char		execid;
	register char		funcid;
	register QTREE		*root;
	struct retcode		*rc;
	extern struct retcode	*issue();
	int			qbufbuf[QBUFSIZ / 2];
#	ifdef xV7_UNIX
	extern char		*ttyname();
	char			*tty;
#	endif

#	ifdef xQTR1
	tTrace(&argc, argv, 'Q');
#	endif
	initproc("QRYMOD", argv);
	acc_init();
	Qbuf = (char *) qbufbuf;

	/* determine user's terminal for protection algorithm */
#	ifndef xV7_UNIX
	Terminal = ttyn(1);
	if (Terminal == 'x')
		Terminal = ' ';
#	endif
#	ifdef xV7_UNIX
	tty = ttyname(1);
	if (bequal(tty, "/dev/", 5))
		tty = &tty[5];
	if (bequal(tty, "tty", 3))
		tty = &tty[3];
	pmove(tty, Terminal, 2, ' ');
	Terminal[2] = '\0';
#	endif

	setexit();
#	ifdef xQTR3
	if (tTf(0, 1))
		printf("setexit->\n");
#	endif

	/*
	**  MAIN LOOP
	**	Executed once for each query, this loop reads the
	**	exec & funcid, determines if the query is
	**	"interesting", modifies it if appropriate, and
	**	if there is anything left worth running, passes
	**	it down the pipe.
	*/

	for (;;)
	{
		Nullsync = FALSE;

		/* read the new function */
		rdpipe(P_PRIME, &Pipe);
		execid = rdpipe(P_EXECID, &Pipe, R_up);
		funcid = rdpipe(P_FUNCID, &Pipe, R_up);

		/* decide on the action to perform */
		switch (execid)
		{

		  case EXEC_DECOMP:
			/* read in query and return tree */
			root = readqry(&pipetrrd, TRUE);
			rdpipe(P_SYNC, &Pipe, R_up);

			/* test for sync of Equel on errors */
			if (Resultvar == -1 && Equel)
				Nullsync = TRUE;

			/* do tree modification and issue query */
			root = qrymod(root);
			rc = issue(execid, funcid, root);

			break;

		  case EXEC_QRYMOD:	/* define, view, protect, or integrity */
			define(funcid);
			rc = NULL;
			break;

		  default:		/* DBU function */
			wrpipe(P_PRIME, &Outpipe, execid, 0, funcid);
			copypipes(&Pipe, R_up, &Outpipe, W_down);
			rc = issue(execid, funcid, NULL);
			break;

		}

		/* signal done to above */
		wrpipe(P_PRIME, &Pipe, execid, 0, funcid);
		if (rc != NULL)
			wrpipe(P_NORM, &Pipe, W_up, rc, sizeof *rc);
		wrpipe(P_END, &Pipe, W_up);
	}
}

rubproc()
{
	resyncpipes();
}
