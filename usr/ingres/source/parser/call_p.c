# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"parser.h"
# include	"../symbol.h"

/*
** CALL_P -- send the buffered command
**	the execid and funcid are given.
*/
call_p(qmode, execid)
int	qmode;
int	execid;
{
	struct pipfrmt		p;
	register struct pipfrmt	*pp;
	register int		i;

	pp = &p;
	wrpipe(P_PRIME, pp, execid, 0, qmode);
	for (i = 0; i < Pc; i++)
	{
#		ifdef	xPTR2
		tTfp(6, 0, "Pv[%d]=%s\n", i, Pv[i]);
#		endif
		wrpipe(P_NORM, pp, W_down, Pv[i], 0);
	}
	wrpipe(P_END, pp, W_down);
	initp();
	syncdn();
}

# define	PARGSIZE	(MAXDOM * 2 + 10)	/* max no of params to dbu */

setp(msg)
char	*msg;
{
	register char	*p;
	register char	*s;
	char		*need();

	s = msg;
	p = need(Pbuffer, length(s) + 1);
	smove(s, p);

	Pv[Pc++] = p;

	if (Pc >= PARGSIZE)
		syserr("SETP: oflo args=%d", Pc);
}

initp()
{
	extern int	neederr();
	char		*need();

	initbuf(Pbuffer, PARBUFSIZ, PBUFOFLO, &neederr);
	Pv = (char **) need(Pbuffer, PARGSIZE * (sizeof(*Pv)));
	Pc = 0;
}
