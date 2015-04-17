# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"IIglobals.h"

/*
**  Buffered Read From Pipe
**
**	Reads from the pipe with the UNIX file descriptor `des' into
**	the buffer `buf'.  `Mode' tells what is to be done.  `Msg' is
**	the address of the input buffer, and `n' is the length of the
**	info.  If `n' is zero, the data is assumed to be a null-
**	terminated string.
**
**	`Buf' is defined in "../pipes.h" together with the modes.
**	The pipe should be written by wrpipe() to insure that the
**	pipe formats match.
**
**	Modes:
**
**	P_PRIME -- Primes the buffer.  This involves cleaning out all
**		the header stuff so that a physical read is forced
**		on the next normal call to rdpipe().  It also resets
**		the error flag, etc.
**
**	P_NORM -- This is the normal mode.  If the buffer is empty,
**		it is filled from the pipe.  `N' bytes are moved into
**		`msg'.  If `n' is zero, bytes are moved into `msg' up
**		to and including the first null byte.  The pipe is
**		read whenever necessary, so the size of the data
**		buffer (controlled by PBUFSIZ) has absolutely no
**		effect on operation.
**
**	P_SYNC -- This mode reads the buffer, filling the pipe as
**		necessary, until an End Of Pipe (EOP) is encountered.
**		An EOP is defined as an empty buffer with a mode
**		("hdrstat") of "END_STAT".  Any data left in the pipe
**		is thrown away, but error messages are not.  Anything
**		not already read by the user is read by P_SYNC, so if
**		you are not certain that you have already read the
**		previous EOP.
**
**	P_EXECID -- The first (exec_id) field of the pipe buffer header
**		is read and returned.  This is used (for instance) by
**		the DBU routines, which must get the exec_id, but must
**		not read the rest of the pipe in case they must overlay
**		themselves.  It must ALWAYS be followed by a rdpipe of
**		mode P_FUNCID!!
**
**	P_FUNCID -- The second through last bytes of the pipe are read
**		into the buffer, and the function code ("func_id") is
**		returned.  This is the second half of a P_EXECID call.
**
**	P_INT -- In the event of an interrupt, a wrpipe() call should
**		be made to all writable pipes of mode P_INT, and then
**		rdpipe() calls should be made on all readable pipes
**		of this mode.  This clears out the pipe up until a
**		special type of pipe header ("SYNC_STAT") is read.  It
**		is absolutely critical that this is called in conjunc-
**		tion with all other processes.  This mode is used in
**		resyncpipes(), which performs all the correct calls
**		in the correct order.
**
**	In all cases except P_INT mode, if an error is read from the
**	pipe it is automatically passed to proc_error().  This routine
**	is responsible for passing the error message to the next higher
**	process.
**
**	If an end of file is read from the pipe, the end_job() routine
**	is called.  This routine should do any necessary end of job
**	processing needed (closing relations, etc.) and return with
**	a zero value.
**
**	Default proc_error() and end_job() routines exist in the
**	library, so if you don't want any extra processing, you can
**	just not bother to define them.
**
**	In general, the number of bytes actually read is returned.  If
**	you read an EOP, zero is returned.
*/

IIrdpipe(mode, buf1, des, msg, n)
int		mode;
char		*msg;
int		n;
int		des;		/* UNIX file descriptor for pipe */
struct pipfrmt	*buf1;
{
	register int		knt;
	register int		syncflg;
	int			i;
	extern char		*IIproc_name;
	char			*proc_name;
	register struct pipfrmt	*buf;
	char			*argv[10], args[512];
	char			*ap;
	int			argc, err;

#	ifdef xATR1
	proc_name = IIproc_name ? IIproc_name : "";
#	endif

	buf = buf1;

#	ifdef xATR1
	if (IIdebug > 1)
		printf("\n%s ent rdpipe md %d buf %u des %d\n", proc_name, mode, buf, des);
#	endif
	syncflg = 0;
	switch (mode)
	{

	  case P_PRIME:
		buf->pbuf_pt = buf->buf_len = buf->err_id = 0;
		buf->hdrstat = NORM_STAT;
		return (0);

	  case P_NORM:
		break;

	  case P_SYNC:
		syncflg++;
		break;

	  case P_EXECID:
		if (read(des, &buf->exec_id, 1) < 1)
		{
			goto eoj;
		}
#		ifdef xATR3
		if (IIdebug > 1)
			printf("%s rdpipe ret %c\n", proc_name,  buf->exec_id);
#		endif
		return (buf->exec_id);

	  case P_FUNCID:
		i = read(des, &buf->func_id, HDRSIZ+PBUFSIZ-1) + 1;
		buf->pbuf_pt = -1;
		goto chkerr;

	  case P_INT:
		syncflg--;
		buf->hdrstat = NORM_STAT;
		break;

	  default:
		IIsyserr("rdpipe: bad call %d", mode);
	}
	knt = 0;
	if (buf->pbuf_pt < 0)
		buf->pbuf_pt = 0;

	do
	{
		/* check for buffer empty */
		while (buf->pbuf_pt >= buf->buf_len || syncflg)
		{
			/* read a new buffer full */
			if (buf->hdrstat == LAST_STAT && syncflg >= 0)
			{
				goto out;
			}
#			ifdef xATR3
			if (IIdebug > 1)
			{
				printf("%s rdng %d w len %d pt %d err %d sf %d\n",
					proc_name, des, buf->buf_len, buf->pbuf_pt, buf->err_id, syncflg);
			}
#			endif
			i = read(des, buf, HDRSIZ+PBUFSIZ);
			if (i == 0)
			{
			eoj:
#				ifdef xATR3
				if (IIdebug > 1)
					printf("%s rdpipe exit\n", proc_name);
#				endif
				if (syncflg < 0)
					IIsyserr("rdpipe: EOF %d", des);
				/* exit(end_job(buf, des)); */
				exit(-1);
			}
			buf->pbuf_pt = 0;
		chkerr:
#			ifdef xATR2
			if (IIdebug > 1)
				IIprpipe(buf);
#			endif
			if (i < HDRSIZ+PBUFSIZ)
				IIsyserr("rdpipe: rd err buf %u des %d", buf, des);
			if (buf->hdrstat == SYNC_STAT)
			{
				if (syncflg < 0)
					return (1);
				else
					IIsyserr("rdpipe: unexpected SYNC_STAT");
			}
			if (buf->err_id != 0 && syncflg >= 0)
			{
#				ifdef xATR2
				if (IIdebug > 1)
					printf("%s rdpipe err %d\n", proc_name, buf->err_id);
#				endif
				/* proc_error(buf, des); */
				/* The error parameters are read here
				** and put into an argv, argc structure
				** so that the user doesn't have to use
				** Ingres utilities to write an intelligent
				** error handler.
				*/
				ap = args;
				argc = 0;
				err = buf->err_id;
				while (i = IIrdpipe(P_NORM,buf,des,ap,0))
				{
					argv[argc++] = ap;
					ap += i;
					if (argc > 10 || (args - ap) >= 500)
						IIsyserr("Too many error parameters!");
				}
				IIerror(err, argc, argv);
#				ifdef xATR3
				if (IIdebug > 1)
					printf("%s pe ret\n", proc_name);
#				endif
				buf->pbuf_pt = buf->buf_len;
				buf->hdrstat = NORM_STAT;
			}
			if (buf->pbuf_pt < 0)
			{
				/* P_FUNCID mode */
#				ifdef xATR1
				if (IIdebug > 1)
					printf("%s rdpipe ret %c\n",
						proc_name, buf->func_id);
#				endif
				return (buf->func_id);
			}
		}
		/* get a byte of information */
		msg[knt++] = buf->pbuf[buf->pbuf_pt++];
	} while ((n == 0) ? (msg[knt-1]) : (knt < n));

out:
#	ifdef xATR1
	if (IIdebug > 1)
	{
		printf("%s rdpipe ret %d", proc_name, knt);
		if (n == 0 && syncflg == 0)
			printf(", str `%s'", msg);
		printf("\n");
	}
#	endif
	return(knt);
}
