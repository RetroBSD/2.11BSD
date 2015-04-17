# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"parser.h"
# include	"../access.h"
# include	"../unix.h"

main(argc,argv1)
int	argc;
char	*argv1[];
{
	register int	rt;
	register char	**argv;
	extern int	yydebug;
	/*
	** This is a ROYAL kludge used to obtain data space for the
	** parser.  The treebuffer should really be allocated in
	** ../parser.h but unfortunately.
	*/
	char		treebuffer[TREEMAX];

	/* set up parser */
	argv = argv1;
	Qbuf = treebuffer;	/* see kludge notice above */
#	ifdef	xPTR1
	tTrace(&argc, argv, 'P');
#	endif
#	ifdef	xPTR2
	if (tTf(17, 0))
		prargs(argc, argv);
#	endif
	initproc("PARSER", argv);
	Noupdt = !setflag(argv, 'U', 0);
	Dcase = setflag(argv, 'L', 1);

	/* if param specified, set result reln storage structures */
	Relspec = "cheapsort";		/* default to cheapsort on ret into */
	Indexspec = "isam";		/* isam on index */
	for (rt=6; rt < argc; rt++)
	{
		if (argv[rt][0] == '-')
		{
			if (argv[rt][1] == 'r')
			{
				Relspec = &argv[rt][2];
				continue;
			}
			if (argv[rt][1] == 'n')
			{
				Indexspec = &argv[rt][2];
				continue;
			}
		}
	}
	if (sequal(Relspec, "heap"))
		Relspec = 0;
	if (sequal(Indexspec, "heap"))
		Indexspec = 0;

	rnginit();
	openr(&Desc, 0, "attribute");
	/*
	** The 'openr' must be done before this test so that the 'Admin'
	**	structure is initialized.  The 'Qrymod' flag is set when
	**	the database has query modification turned on.
	*/
	Qrymod = ((Admin.adhdr.adflags & A_QRYMOD) == A_QRYMOD);
	setexit();

	/* EXECUTE */
	for (;;)
	{
		startgo();		/* initializations for start of go-block */

		yyparse();		/* will not return until end of go-block or error */

		endgo();		/* do cleanup (resync, etc) for a go-block */
	}
}
 
/*
** RUBPROC
*/
rubproc()
{
	resyncpipes();
}

/*
**  PROC_ERROR ROUTINE
**
**	This routine handles the processing of errors for the parser
**	process.  It sets the variable 'Ingerr' if an error is passes
**	up from one of the processes below.
*/
proc_error(s1, rpipnum)
struct pipfrmt	*s1;
int		rpipnum;
{
	register struct pipfrmt	*s;
	register int		fd;
	register char		*b;
	char			buf[120];
	struct pipfrmt		t;

	fd = rpipnum;
	s = s1;
	b = buf;
	if (fd != R_down)
		syserr("proc_error: bad pipe");
	wrpipe(P_PRIME, &t, s->exec_id, 0, s->func_id);
	t.err_id = s->err_id;

	copypipes(s, fd, &t, W_up);
	rdpipe(P_PRIME, s);
	Ingerr = 1;
}
