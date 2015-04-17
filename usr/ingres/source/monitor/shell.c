# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"monitor.h"

/*
**  CALL UNIX SHELL
**
**	The UNIX shell is called.  Shell() first tries to call an
**	alternate shell defined by the macro {shell}, and if it fails
**	calls /bin/sh.
**
**	If an argument is supplied, it is a shell file which is
**	supplied to the shell.
**
**	Uses trace flag 7
*/

shell()
{
	register int	i;
	register char	*p;
	register char	*shellfile;
	char		*getfilename();
	char		*macro();

	shellfile = getfilename();
	if (*shellfile == 0)
		shellfile = 0;

	fclose(Qryiop);
	if ((Xwaitpid = fork()) == -1)
		syserr("shell: fork");
	if (Xwaitpid == 0)
	{
		setuid(getuid());
#		ifndef xB_UNIX
		setgid(getgid());
#		endif
		for (i = 3; i < MAXFILES; i++)
			close(i);
		p = macro("{shell}");
#		ifdef xMTR3
		tTfp(7, 0, "{shell} = '%o'\n", p);
#		endif
		if (p != 0)
		{
			execl(p, p, shellfile, Qbname, 0);
			printf("Cannot call %s; using /bin/sh\n", p);
		}
		execl("/bin/sh", "sh", shellfile, Qbname, 0);
		syserr("shell: exec");
	}

	if (Nodayfile >= 0)
		printf(">>shell\n");
	/* wait for shell to complete */
	xwait();
}
