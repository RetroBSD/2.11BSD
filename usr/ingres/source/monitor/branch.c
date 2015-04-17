# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  BRANCH
**
**	The "filename" following the \b op must match the "filename"
**	which follows some \k command somewhere in this same file.
**	The input pointer is placed at that point if possible.  If
**	the label does not exist, an error is printed and the next
**	character read is an EOF.
**
**	Uses trace flag 16
*/

branch()
{
	register char	c;
	register int	i;
	extern char	getch();

#	ifdef xMTR2
	if (tTf(16, -1))
		printf(">>branch: ");
#	endif

	/* see if conditional */
	while ((c = getch()) > 0)
		if (c != ' ' && c != '\t')
			break;
	if (c == '?')
	{
		/* got a conditional; evaluate it */
		Oneline = TRUE;
		macinit(&getch, 0, 0);
		i = expr();

		if (i <= 0)
		{
			/* no branch */
#			ifdef xMTR2
			if (tTf(16, 0))
				printf("no branch\n");
#			endif
			getfilename();
			return;
		}
	}
	else
	{
		Peekch = c;
	}

	/* get the target label */
	if (branchto(getfilename()) == 0)
		if (branchto(macro("{default}")) == 0)
		{
			Peekch = -1;
			printf("Cannot branch\n");
		}
	return;
}


/* changed by marc on 4/14/80 to fix return error */
branchto(label)
char	*label;
{
	char		target[100];
	register char	c;

	smove(label, target);
	if (rewind(Input))
	{
		printf("Cannot branch on a terminal\n");
		return (1);
	}

	/* clear(0); */
	/* search for the label */
	while ((c = getch()) > 0)
	{
		if (c != '\\')
			continue;
		if (getescape(0) != C_MARK)
			continue;
		if (sequal(getfilename(), target))
			return (1);
	}
	return (0);
}
