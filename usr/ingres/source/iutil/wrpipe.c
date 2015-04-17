# include	"../ingres.h"
# include	"../pipes.h"

/*
**  Buffered Write on Pipe
**
**	The message `msg' of length `n' is buffered and written onto
**	the pipe with UNIX file descriptor `des'.  `Buf' is the addr
**	of the buffer in which buffering is to take place.  If `n' is
**	zero, `msg' is taken to be a null-terminated string of char-
**	acters.  `Mode' gives the operations mode of wrpipe().
**
**	Buffers written with wrpipe() should be read with rdpipe().
**
**	Modes:
**
**	P_PRIME -- Prime the pipe for writing.  This involves clearing
**		out the error field, setting normal status, etc.  In
**		general, it should always be done when you want to
**		start writing.  It need not be done after writing an
**		end-of-pipe (EOP), as this operation automatically
**		resets the pipe.  The "exec_id" field (read by
**		rdpipe(P_EXECID)) is set to `des', and the "func_id"
**		field is set to `n'.
**
**	P_NORM -- Normal mode.  `Msg' is buffered into `buf' and
**		physically written into the pipe as necessary.
**
**	P_END -- Writes an EOP.  Only the first three parameters need
**		be specified.
**
**	P_FLUSH -- Flushes the buffer into the pipe, but does not write
**		an EOP.  In fact, a non-EOP is guaranteed to be
**		written.
**
**	P_WRITE -- Same as P_FLUSH, but the mode is unchanged.  This
**		mode is pretty obscure, and you probably need not use
**		it.
**
**	P_INT -- Writes an interrupt synchronization block into the
**		pipe.  This mode need only be called by resyncpipes()
**		and need not normally concern the user.  It is critical
**		that this mode be called by all processes so that dead-
**		lock or syserrs do not occur.
**
**	P_PARAM -- Copies one byte parameter into structure. Can be
**		read via a call to rdpipe(P_PARAM).
*/

extern int	write();		/* standard write function */
int		(*Pi_wr_fn)() = &write;	/* pipe write function */

wrpipe(mode, buf1, des, msg, n)
int		mode;
struct pipfrmt	*buf1;
int		des;
char		*msg;
int		n;
{
	register int		nn;
	extern char		*Proc_name;
	register int		msgl;
	register struct pipfrmt	*buf;
	int			flushflg;
	int			i;

	buf = buf1;
#	ifdef xATR1
	if (tTf(99, 8))
		printf("\n%s wrpipe md %d buf %u des %d msg %l n %d\n",
			Proc_name, mode, buf, des, msg, n);
#	endif
	nn = msgl = 0;
	flushflg = 0;
	switch (mode)
	{
	  case P_PRIME:
		buf->buf_len = buf->pbuf_pt = buf->err_id = buf->param_id = 0;
		buf->hdrstat = NORM_STAT;
		buf->exec_id = des;
		buf->func_id = n;
		return(0);

	  case P_PARAM:
		buf->param_id = des;
		return (0);

	  case P_NORM:
		msgl = nn = (n == 0) ? length(msg) + 1 : n;
		break;

	  case P_END:
		buf->hdrstat = LAST_STAT;
		flushflg++;
		break;

	  case P_FLUSH:
		buf->hdrstat = NORM_STAT;
		flushflg++;
		break;

	  case P_WRITE:
		flushflg--;
		break;

	  case P_INT:
		buf->hdrstat = SYNC_STAT;
		flushflg++;
		break;

	  default:
		syserr("wrpipe: bad mode %d", mode);
	}

	while (nn > 0 || flushflg)
	{
		/* BUFFER OUT DATA */
		while (nn > 0 && buf->pbuf_pt < PBUFSIZ)
			buf->pbuf[buf->pbuf_pt++] = msg[msgl - (nn--)];
		/* WRITE PIPE ? */
		if (buf->pbuf_pt >= PBUFSIZ || flushflg)
		{
			if (flushflg >= 0)
				buf->buf_len = buf->pbuf_pt;
#			ifdef xATR2
			if (tTf(99, 10))
				prpipe(buf);
#			endif
			if ((i = (*Pi_wr_fn)(des, buf, HDRSIZ+PBUFSIZ)) < HDRSIZ+PBUFSIZ)
				syserr("wrpipe: wr err %d %d", i, des);
			buf->pbuf_pt = 0;
			buf->err_id = 0;
			flushflg = 0;
		}
	}
#	ifdef xATR3
	if (tTf(99, 9))
	{
		printf("%s wrpipe ret %d\n", Proc_name, msgl);
	}
#	endif
	return (msgl);
}
