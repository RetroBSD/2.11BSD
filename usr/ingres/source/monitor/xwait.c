# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  WAIT FOR PROCESS TO DIE
**
**	Used in edit() and shell() to terminate the subprocess.
**	Waits on pid "Xwaitpid".  If this is zero, xwait() returns
**	immediately.
**
**	This is called by "catchsig()" so as to properly terminate
**	on a rubout while in one of the subsystems.
**
**	Uses trace flag 15
*/

xwait()
{
	int		status;
	register int	i;
	register char	c;

#	ifdef xMTR2
	if (tTf(15, 0))
		printf("xwait: [%d]\n", Xwaitpid);
#	endif
	if (Xwaitpid == 0)
	{
		cgprompt();
		return;
	}
	while ((i = wait(&status)) != -1)
	{
#		ifdef xMTR2
		if (tTf(15, 1))
			printf("pid %d stat %d\n", i, status);
#		endif
		if (i == Xwaitpid)
			break;
	}

	Xwaitpid = 0;

	/* reopen query buffer */
	if ((Qryiop = fopen(Qbname, "a")) == NULL)
		syserr("xwait: open %s", Qbname);
	Notnull = 1;

	cgprompt();
}
