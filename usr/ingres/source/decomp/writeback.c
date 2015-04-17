# include	"../ingres.h"
# include	"../aux.h"
# include	"../tree.h"
# include	"decomp.h"
# include	"../pipes.h"

writeback(flag)
int	flag;
{
	struct pipfrmt		buf;
	extern struct pipfrmt	Inpipe;
	extern struct retcode	Retcode;

	flush();
	wrpipe(P_PRIME, &buf, EXEC_PARSER, 0, 0);
	if (flag > 0)
	{
		/* send back tuple count info */

#		ifdef xDTR1
		if (tTf(8, 10))
			printf("%s tups found\n", locv(Retcode.rc_tupcount));
#		endif

		wrpipe(P_NORM, &buf, W_up, &Retcode, sizeof (Retcode));
	}

	if (flag < 0)
	{
		rdpipe(P_PRIME, &Inpipe);
		copypipes(&Inpipe, R_down, &buf, W_up);
	}
	if (flag >= 0)
		wrpipe(P_END, &buf, W_up);
}
