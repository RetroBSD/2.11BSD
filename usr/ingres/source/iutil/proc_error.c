# include	"../pipes.h"

/*
**  DEFAULT PROC_ERROR ROUTINE
**
**	This routine handles the processing of errors for a typical
**	process.  Please feel free to redefine it if you need other
**	features.
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
}
