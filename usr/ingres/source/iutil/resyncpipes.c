# include	"../pipes.h"

/*
**  RESYNCHRONIZE PIPES AFTER AN INTERRUPT
**
**	The pipes are all cleared out.  This routines must be called
**	by all processes in the system simultaneously.  It should be
**	called from the interrupt catching routine.
*/

resyncpipes()
{
	struct pipfrmt		buf;
	register struct pipfrmt	*pbuf;
	register int		fd;

	pbuf = &buf;
	wrpipe(P_PRIME, pbuf, 0, 0, 0);
	/* synchronize the downward pipes */
	fd = R_up;
	if (fd >= 0)
		rdpipe(P_INT, pbuf, fd);
	fd = W_down;
	if (fd >= 0)
		wrpipe(P_INT, pbuf, fd);

	/* now the upward pipes */
	fd = R_down;
	if (fd >= 0)
		rdpipe(P_INT, pbuf, fd);
	fd = W_up;
	if (fd >= 0)
		wrpipe(P_INT, pbuf, fd);

	return;
}
