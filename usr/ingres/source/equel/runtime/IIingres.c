# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"../../unix.h"
# include	"../../aux.h"
# include	"IIglobals.h"
# define	CLOSED	'?'

char	*IImainpr	= "/usr/bin/ingres";
char	IIPathname[41];

IIingres(p1, p2, p3, p4, p5, p6, p7, p8, p9)
char	*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9;
/*
**	IIingres opens the needed pipes and
**	forks an ingres process.
**
**	ingres recognizes the EQUEL flag followed by
**	three control characters as being an equel processes
**
**	parameters to ingres are passed directly. only
**	the first 9 are counted.
*/

{
	int		pipes[6];		/* pipe vector buffer */
	char		eoption[10];	/* dummy variable to hold -EQUEL option */
	register char	*cp;
	char		*argv[12];
	register char	**ap;
	extern		IIresync();
	extern		*(IIinterrupt)(),	exit();
	char		pathbuf[60];

#	ifdef xETR1
	if (IIdebug)
		printf("IIingres\n");
#	endif
	/* test if ingres is already invoked */
	if (IIingpid)
		IIsyserr("Attempt to invoke INGRES twice");

	IIgetpath();
	/* open INGRES pipes */
	if (pipe(&pipes[0]) || pipe(&pipes[2]) || pipe(&pipes[4]))
		IIsyserr("pipe error in IIingres");

	IIw_down = pipes[1];	/* file desc for equel->parser */
	IIr_down = pipes[2];	/* file desc for parser->equel */
	IIr_front = pipes[4];	/* file desc for ovqp->equel */

	/* catch interupts if they are not being ignored */
	if (signal(2,1) != 1)
		signal(2,&IIresync);
	/* prime the control pipes */
	IIrdpipe(P_PRIME, &IIinpipe);
	IIwrpipe(P_PRIME, &IIoutpipe, IIw_down);

	/* set up equel option flag */
	cp = eoption;
	*cp++ = '-';
	*cp++ = EQUEL;
	*cp++ = pipes[0] | 0100;
	*cp++ = pipes[3] | 0100;
	*cp++ = CLOSED;	/* this will be the equel->ovqp pipe in the future */
	*cp++ = pipes[5] | 0100;
	/* put "6.2" at end of flag for OVQP to not do flush after
	 * every tuple
	 */
	*cp++ = '6';
	*cp++ = '.';
	*cp++ = '2';
	*cp = '\0';

	if ((IIingpid = fork()) < 0)
		IIsyserr("IIingres:cant fork %d", IIingpid);
	/* if parent, close the unneeded files and return */
	if (IIingpid)
	{
		if (close(pipes[0]) || close(pipes[3]) || close(pipes[5]))
			IIsyserr("close error 1 in IIingres");
#		ifdef xETR1
		if (IIdebug)
			printf("calling ingres with '%s' database %s\n", eoption, p1);
#		endif
		return;
	}
	/* the child overlays /usr/bin/ingres */
	ap = argv;
	*ap++ = "ingres";
	*ap++ = eoption;
	*ap++ = p1;
	*ap++ = p2;
	*ap++ = p3;
	*ap++ = p4;
	*ap++ = p5;
	*ap++ = p6;
	*ap++ = p7;
	*ap++ = p8;
	*ap++ = p9;
	*ap = 0;
	if (IImainpr[0] != '/')
	{
		strcpy(pathbuf, IIPathname);
		strcat(pathbuf, "/");
		strcat(pathbuf, IImainpr);
	}
	else
		strcpy(pathbuf, IImainpr);
	execv(pathbuf, argv);
	IIsyserr("cannot exec %s in IIingres", pathbuf);
}

/*
**  IIGETPATH -- initialize the IIPathname variable
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Sets IIPathname to the home directory of the USERINGRES
**		[unix.h] user.
**
**	Called By:
**		IIingres.c
**
**	History:
**		3/26/79 -- (marc) written
*/

IIgetpath()
{
	char			line[MAXLINE + 1];
	static char		reenter;
	register int		i;
	register char		*lp;
	struct iob		iobuf;
	char			*field[UF_NFIELDS];

	if (reenter)
		return;
	else
		reenter += 1;

	if ((i = IIfopen("/etc/passwd", &iobuf)) < 0)
		IIsyserr("IIgetpath: no /etc/passwd");

	do
	{
		/* get a line from the passwd file */
		i = 0;
		for (lp = line; (*lp = IIgetc(&iobuf)) != '\n'; lp++)
		{
			if (*lp == -1)
				IIsyserr("IIgetpath: no user 'ingres' in /etc/passwd");
			if (++i > sizeof line - 1)
			{
				*lp = '\0';
				IIsyserr("IIgetpath: line overflow: \"%s\"",
				line);
			}
		}
		*lp = '\0';
		for (i = 0, lp = line; *lp != '\0'; lp++)
		{
			if (*lp == ':')
			{
				*lp = '\0';
				if (i > UF_NFIELDS)
					IIsyserr("IIgetpath: too many fields in passwd \"%s\"",
					line);
				field[i++] = lp + 1;
			}
		}
		/* check for enough fields for valid entry */
		if (i < 3)
			IIsyserr("IIgetpath: too few fields \"%s\"",
			line);
	}  while (!IIsequal(line, USERINGRES));
	IIclose(&iobuf);

	/* check that pathname won't overflow static buffer */
	if (field[i - 1] - field[i - 2] > sizeof IIPathname)
		IIsyserr("IIgetpath: path too long \"%s\"", field[i - 2]);

	/* move pathname into buffer */
	IIbmove(field[i - 2], IIPathname, field[i - 1] - field[i - 2]);
}
