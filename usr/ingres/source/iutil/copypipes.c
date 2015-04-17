# include	"../pipes.h"

/*
**  COPY PIPES
**
**	Copys pipe 'fromd' to pipe 'tod' using pipe buffers 'fromb'
**	and 'tob'.
**
**	Neither pipe is primed or otherwise set up.  The end of pipe
**	is copied.
*/

copypipes(fromb1, fromd, tob1, tod)
struct pipfrmt	*fromb1, *tob1;
int		fromd, tod;
{
	register struct pipfrmt	*fromb, *tob;
	register int		i;
	char			buf[120];

	fromb = fromb1;
	tob = tob1;

	while ((i = rdpipe(P_NORM, fromb, fromd, buf, sizeof buf)) > 0)
		wrpipe(P_NORM, tob, tod, buf, i);
	wrpipe(P_END, tob, tod);
}
