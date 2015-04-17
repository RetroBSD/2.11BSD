# include	"../ingres.h"
# include	"../pipes.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../access.h"
# include	"../lock.h"

/*
**  DBU CONTROLLER -- Handles parameter passing & return for dbu's.
**
**	This is the common control routine for calling dbu routines
**	and switching overlays when necessary. The specific dbu routine
**	is indicated in the funcid according to a set of numbers
**	defined in ../symbol.h. The execid is EXEC_DBU.
**
**	The dbu controller calls init_proctab to read in the process table.
**	This defines the location of each dbu routine (whether they are in
**	overlaya, overlayc, overlayi or overlaym). If the requested dbu routine
**	is not in the current overlay, the correct overlay is called with
**	the execid of the correct overlay (a,c,i,m) and the correct funcid.
**
**	To call a dbu, the parameters are read into a buffer and a pc,pv
**	parameter structure is made. When the dbu returns, all system
**	relations are flushed and a sync is returned.
**	If the Dburetflag is set, then the controller returns the
**	Dburetcode structure back.
**
**	Positional Parameters:
**		standard plus
**			Xparams[0] - pathname for ksort
**			Xparams[1] - process table
**
**	Flags:
**		-Z -- trace flags. See individual routines for details
**		--X -- X & 077 is a file descriptor to read to get the
**			first pipe block (instead of R_up).
**
**	Files:
**		none
**
**	Requires:
**		initproc() - to initialize the process
**		acc_init() - to open any am files.
**		Files[] - array of open file descriptors
**		init_proctab() - reads in process table
**
**	Compilation Flags:
**		xZTR[123] used by various routines
**
**	Trace Flags:
**		Z1
**
**	Diagnostics:
**		see reference manual
**
**	Compilation Instructions:
**		To make a specific overlay use
**			setup overlay a		(for overlaya)
**		This does the following:
**			cc -f -n overlaya.c ../../lib/dbulib \\
**						../../lib/access \\
**						../../lib/utility \
**
**			mv a.out overlaya
**			chmod 4700 overlaya
**			mv overlaya ../../bin/overlaya
**
**	History:
**		1/25/79 (eric) -- Changed '--X' flag to read file
**			X.  This simplifies rdpipe.
**		1/8/79 (rse) -- added rubproc routine
**		12/18/78 (rse) -- added Dburetcode structure.
**		10-6-78 (rse) - wrote comment block; added pages
**				read/wrote on trace flag
*/




# define	MAXARGS		18	/* max number of args to main() */
# define	STRBUFSIZ	1000	/* max bytes of arguments */

extern int	Files[MAXFILES];
char		Cur_id;
char		Execid;
char		Funcid;
int		Noupdt;
struct pipfrmt	Pbuf;
int		Dburetflag;
struct retcode	Dburetcode;


main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*Proc_name;
	register char	*sp;
	char		*parmv[MAXPARMS];	/* ptrs to string parameters */
	char		stringbuf[STRBUFSIZ];	/* hold parameters for DBU functions */
	char		*vect[MAXARGS];
	int		exind;
	register int	i;
	int		j;
	extern char	**Xparams;
	extern		(*Func[])();		/* function pointers */
	register char	*funcflag;
	char		**vp;
	extern long	Accuread, Accusread, Accuwrite;
	int		pipevect[2];

	initproc("OVERLAYx", argv);
	Noupdt = !setflag(argv, 'U', 0);
	exind = length(argv[0]) - 1;
	Cur_id = argv[0][exind];
	Proc_name[7] = Cur_id;
	bmove(argv, vect, argc * sizeof argv[0]);
	vect[argc] = 0;
	i = argc;
#	ifdef xZTR1
	tTrace(&argc, argv, 'Z');
	if (tTf(1, 2))
		prargs(i, vect);
#	endif

	/*
	**  Scan argument vector for '--' flag; if found, save location
	**  of 'X' part for use when we exec another overlay, and read
	**  the block from the last overlay.  If not found, create a
	**  '--' flag for later use.
	*/

	funcflag = NULL; /* tih */

	for (vp = vect; *vp != NULL; vp++)
	{
		sp = *vp;
		if (sp[0] == '-' && sp[1] == '-')
		{
			/* deferred EXECID/FUNCID */
			funcflag = &sp[2];
			i = *funcflag & 077;
			Execid = rdpipe(P_EXECID, &Pbuf, i);
			Funcid = rdpipe(P_FUNCID, &Pbuf, i);
			close(i);
		}
	}
	if (funcflag == NULL)
	{
		/* put in a -- flag */
		*vp++ = sp = "---";
		funcflag = &sp[2];
		*vp = NULL;
		Execid = 0;		/* read initial exec/func id */
	}

	/*
	**  Create list of open files.
	**	This allows us to close all extraneous files when an
	**	overlay completes (thus checking consistency) or on
	**	an interrupt, without really knowing what was going
	**	on.  Notice that we call acc_init so that any files
	**	the access methods need will be open for this test.
	*/

	acc_init();

#	ifdef xZTR3
	if (tTf(1, 8))
		printf("Open files: ");
#	endif
	for (i = 0; i < MAXFILES; i++)
		if ((Files[i] = dup(i)) >= 0)
		{
			close(Files[i]);
#			ifdef xZTR3
			if (tTf(1, 8))
				printf(" %d", i);
#			endif
		}
#	ifdef xZTR3
	if (tTf(1, 8))
		printf("\n");
#	endif

	/* initialize the process table */
	init_proctab(Xparams[1], Cur_id);

	setexit();

	/*  *** MAIN LOOP ***  */
	for (;;)
	{
		/* get exec and func id's */
		if (Execid == 0)
		{
			rdpipe(P_PRIME, &Pbuf);
			Execid = rdpipe(P_EXECID, &Pbuf, R_up);
			Funcid = rdpipe(P_FUNCID, &Pbuf, R_up);
		}

		if (Execid == EXEC_DBU)
			get_proctab(Funcid, &Execid, &Funcid);

		/* see if in this overlay */
		if (Execid != Cur_id)
			break;
		i = 0;

		/* read the parameters */
		/*** this should check for overflow of stringbuf ***/
		for (sp = stringbuf; j = rdpipe(P_NORM, &Pbuf, R_up, sp, 0); sp += j)
			parmv[i++] = sp;
		parmv[i] = (char *) -1;
#		ifdef xZTR1
		if (tTf(1, 4))
		{
			printf("overlay %c%c: ", Execid, Funcid);
			prargs(i, parmv);
		}
#		endif
		if (i >= MAXPARMS)
			syserr("arg ovf %d %c%c", i, Cur_id, Funcid);

		/* call the specified function (or it's alias) */
#		ifdef xZTR3
		if (tTf(1, 7))
			printf("calling %c%c\n", Execid, Funcid);
#		endif
#		ifdef xZTR1
		if (tTf(50, 0))
		{
			Accuread = 0;
			Accuwrite = 0;
			Accusread = 0;
		}
#		endif
		j = Funcid - '0';
		if (Funcid >= 'A')
			j -= 'A' - '9' -1;
		Dburetflag = FALSE;
		i = (*Func[j])(i, parmv);
#		ifdef xZTR1
		if (tTf(1, 5))
			printf("returning %d\n", i);
		if (tTf(50, 0))
		{
			printf("DBU read %s pages,", locv(Accuread));
			printf("%s catalog pages,", locv(Accusread));
			printf("wrote %s pages\n", locv(Accuwrite));
		}
#		endif

		/* do termination processing */
		finish();
		if (Dburetflag)
			wrpipe(P_NORM, &Pbuf, W_up, &Dburetcode, sizeof (Dburetcode));
		wrpipe(P_END, &Pbuf, W_up);
	}

	/*
	**  Transfer to another overlay.
	**	We only get to this point if we are in the wrong
	**	overlay to execute the desired command.
	**
	**	We close system catalog caches and access method files
	**	so that no extra open files will be laying around.
	**	Then we adjust the pathname of this overlay to point
	**	to the next overlay.  We then perform a little magic:
	**	we create a pipe, write the current pipe block into
	**	the write end of that pipe, and then pass the read
	**	end on to the next overlay (via the --X flag) -- thus
	**	using the pipe mechanism as a temporary buffer.
	*/

	/* close the system catalog caches */
	closecatalog(TRUE);

	/* close any files left open for access methods */
	if (acc_close())
		syserr("MAIN: acc_close");

	/* setup the execid/Funcid for next overlay */
	vect[0][exind] = Execid;

	/* create and fill the communication pipe */
	if (pipexx(pipevect) < 0)
		syserr("main: pipe");
	*funcflag = pipevect[0] | 0100;
	wrpipe(P_WRITE, &Pbuf, pipevect[1], NULL, 0);
	close(pipevect[1]);

#	ifdef xZTR1
	if (tTf(1, 1))
		printf("calling %s\n", vect[0]);
#	endif
	execv(vect[0], vect);
	syserr("cannot exec %s", vect[0]);
}



finish()
{
	struct stat	fstat_buf;
	register int	i;
	register int	j;

#	ifdef xZTR1
	if (tTf(1, 15))
	{
		for (i = 0; i < MAXFILES; i++)
		{
			j = dup(i);
			if (j < 0)
			{
				if (Files[i] >= 0)
					printf("file %d closed by %c%c\n",
						i, Cur_id, Funcid);
			}
			else
			{
				close(j);
				if (Files[i] < 0)
				{
					fstat(i, &fstat_buf);
					printf("file %d (i# %u) opened by %c%c\n",
						i, fstat_buf.st_ino, Cur_id, Funcid);
					close(i);
				}
			}
		}
	}
#	endif

	/* flush the buffers for the system catalogs */
	closecatalog(FALSE);

	wrpipe(P_PRIME, &Pbuf, Execid, 0, Funcid);
	Execid = Funcid = 0;
}



rubproc()
{
	register int	i;

	flush();
	resyncpipes();
	finish();

	/* close any extraneous files */
	unlall();
	for (i = 0; i < MAXFILES; i++)
		if (Files[i] < 0)
			close(i);
}



/*
**  PIPEXX -- dummy call to pipe
**
**	This is exactly like pipe (in fact, it calls pipe),
**	but is included because some versions of the C interface
**	to UNIX clobber a register varaible.  Yeuch, shit, and
**	all that.
*/

pipexx(pv)
int	pv[2];
{
	return (pipe(pv));
}
