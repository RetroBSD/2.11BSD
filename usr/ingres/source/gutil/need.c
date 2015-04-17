/*
**  NEED.C -- general buffer allocation routines
**
**	allow buffers with LIFO de-allocation
**
**	Defines:
**		need()
**		initbuf()
**
**	History:
**		10/10/78 -- (marc) modified from need in ovqp.
**			New are : err_num, err_func stuff, and
**			seterr().
*/






/* structure that the routines use to allocate space */
struct nodbuffer
{
	int		nleft;		/* bytes left */
	int		err_num;	/* error code on overflow */
	int		(*err_func)();	/* error function on overflow */
	char		*xfree;		/* next free byte */
	char		buffer [];	/*beginning of buffer area */
};

/*
**  NEED -- allocate space from a buffer
**
**	On buffer overflow, calls err_func from that field
**	in the buffer with the error code err_code from that field
**	in the buffer, then returns 0.
**	need() guarantees an even adress on return.
**
**	Parameters:
**		bf -- buffer
**		nbytes -- number of bytes desired
**
**	Returns:
**		pointer to allocated area
**		on  buffer overflow returns 0.
**
**	Side Effects:
**		adjusts buffer structure to reflect allocation.
**
*/

char *
need(bf, nbytes)
struct nodbuffer 	*bf;
int			nbytes;
{
	register char			*x;
	register struct nodbuffer	*buf;
	register int			i;

	buf = bf;
	i = nbytes;
	if (i > buf->nleft)
	{
			(*buf->err_func)(buf->err_num);
			return (0);
	}
	i += i & 01;
	x = buf->xfree;
	buf->xfree += i;
	buf->nleft -= i;
	return(x);
}

/*
**  INITBUF -- initialize a buffer
**
**	Must be called before the first need() call on the buffer.
**
**	Parameters:
**		bf -- buffer
**		size -- size fo buffer area
**		err_num -- error code for overflow
**		err_func -- function to call with err_code on error
**
**	Returns:
**		none
**
**	Side Effects:
**		initializes buffer structure
**
**	Diagnostics:
**		"initbuf : odd buffer adress 0%o" -- buffers must start
**			at an even adress.
*/

initbuf(bf, size, err_num, err_func)
struct nodbuffer	*bf;
int			size;
int			err_num;
int			(*err_func)();
{
	register struct nodbuffer	*buf;

	buf = bf;
	if ((unsigned int) buf & 01)
		syserr("initbuf : odd buffer adress 0%o", buf);
	buf->nleft = size - sizeof *buf;
	buf->xfree = buf->buffer;
	buf->err_num = err_num;
	buf->err_func = err_func;
}
