# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  OUTPUT PROMPT CHARACTER
**
**	The prompt is output to the standard output.  It will not be
**	output if -ss mode is set or if we are not at a newline.
**
**	The parameter is printed out if non-zero.
**
**	Uses trace flag 14
*/

prompt(msg)
char	*msg;
{
	if (!Prompt || Peekch < 0)
		return;
	if (Nodayfile >= 0)
	{
		if (msg)
			printf("\07%s\n", msg);
		printf("* ");
	}
}


/*
**  PROMPT WITH CONTINUE OR GO
*/

cgprompt()
{
	prompt(Notnull ? "continue" : "go");
}
