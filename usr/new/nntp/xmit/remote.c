/*
** remote communication routines for NNTP/SMTP style communication.
**
**	sendcmd		- return TRUE on error.
**
**	readreply	- return reply code or FAIL for error;
**				modifies buffer passed to it.
**
**	converse	- sendcmd() & readreply();
**				return reply code or FAIL for error;
**				modifies buffer passed to it.
**
**	hello		- establish connection with remote;
**				check greeting code.
**
**	goodbye		- give QUIT command, and shut down connection.
**
**	sfgets		- safe fgets(); does fgets with TIMEOUT.
**			  (N.B.: possibly unportable stdio macro ref in here)
**
**	rfgets		- remote fgets() (calls sfgets());
**				does SMTP dot escaping and
**				\r\n -> \n conversion.
**
**	sendfile	- send a file with SMTP dot escaping and
**				\n -> \r\n conversion.
**
** Erik E. Fair <fair@ucbarpa.berkeley.edu>
*/

#include "nntpxmit.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#ifdef SYSLOG
#include <syslog.h>
#endif
#include "get_tcp_conn.h"
#include "nntp.h"

static	jmp_buf	SFGstack;
FILE	*rmt_rd;
FILE	*rmt_wr;
char	*sfgets();
char	*rfgets();

extern	int	errno;
extern	char	Debug;
extern	char	*errmsg();
extern	char	*strcpy();
extern	void	log();

/*
** send cmd to remote, terminated with a CRLF.
*/
sendcmd(cmd)
char	*cmd;
{
	if (cmd == (char *)NULL)
		return(TRUE);	/* error */
	dprintf(stderr, ">>> %s\n", cmd);	/* DEBUG */
	(void) fprintf(rmt_wr, "%s\r\n", cmd);
	(void) fflush(rmt_wr);
	return(ferror(rmt_wr));
}

/*
** read a reply line from the remote server and return the code number
** as an integer, and the message in a buffer supplied by the caller.
** Returns FAIL if something went wrong.
*/
readreply(buf, size)
register char	*buf;
int	size;
{
	register char	*cp;
	register int	len;

	if (buf == (char *)NULL || size <= 0)
		return(FAIL);

	/*
	** make sure it's invalid, unless we say otherwise
	*/
	buf[0] = '\0';

	/*
	** read one line from the remote
	*/
	if (sfgets(buf, size, rmt_rd) == NULL)
		return(FAIL);	/* error reading from remote */

	/*
	** Make sure that what the remote sent us had a CRLF at the end
	** of the line, and then null it out.
	*/
	if ((len = strlen(buf)) > 2 && *(cp = &buf[len - 2]) == '\r' &&
		*(cp + 1) == '\n')
	{
		*cp = '\0';
	} else
		return(FAIL);	/* error reading from remote */

	dprintf(stderr, "%s\n", buf);	/* DEBUG */
	/*
	** Skip any non-digits leading the response code 
	** and then convert the code from ascii to integer for
	** return from this routine.
	*/
	cp = buf;
	while(*cp != '\0' && isascii(*cp) && !isdigit(*cp))
		cp++;	/* skip anything leading */

	if (*cp == '\0' || !isascii(*cp))
		return(FAIL);	/* error reading from remote */

	return(atoi(cp));
}

/*
** send a command to the remote, and wait for a response
** returns the response code, and the message in the buffer
*/
converse(buf, size)
char	*buf;
int	size;
{
	register int	resp;

	if (sendcmd(buf))
		return(FAIL);	/* Ooops! Something went wrong in xmit */
	/*
	** Skip the silly 100 series messages, since they're not the
	** final response we can expect
	*/
	while((resp = readreply(buf, size)) >= 100 && resp < 200)
		continue;
	return(resp);
}

/*
** Contact the remote server and set up the two global FILE pointers
** to that descriptor.
**
** I can see the day when this routine will have 8 args:  one for
** hostname, and one for each of the seven ISO Reference Model layers
** for networking. A curse upon those involved with the ISO protocol
** effort: may they be forced to use the network that they will create,
** as opposed to something that works (like the Internet).
*/
hello(host, transport)
char	*host;
int	transport;
{ char	*service;
	char	*rmode = "r";
	char	*wmode = "w";
	char	*e_fdopen = "fdopen(%d, \"%s\"): %s";
	int	socket0, socket1;	/* to me (bad pun) */
	char	buf[BUFSIZ];

	switch(transport) {
	case T_IP_TCP:
		service = "nntp";
		socket0 = get_tcp_conn(host, service);
		break;
	case T_DECNET:
#ifdef DECNET
		(void) signal(SIGPIPE, SIG_IGN);
		service = "NNTP";
		socket0 = dnet_conn(host, service, 0, 0, 0, 0, 0);
		if (socket0 < 0) {
			switch(errno) {
			case EADDRNOTAVAIL:
				socket0 = NOHOST;
				break;
			case ESRCH:
				socket0 = NOSERVICE;
				break;
			}
		}
		break;
#else
		log(L_WARNING, "no DECNET support compiled in");
		return(FAIL);
#endif
	case T_FD:
		service = "with a smile";
		socket0 = atoi(host);
		break;
	}

	if (socket0 < 0) {
		switch(socket0) {
		case NOHOST:
			sprintf(buf, "%s host unknown", host);
			log(L_WARNING, buf);
			return(FAIL);
		case NOSERVICE:
			sprintf(buf, "%s service unknown: %s", host, service);
			log(L_WARNING, buf);
			return(FAIL);
		case FAIL:
			sprintf(buf, "%s hello: %s", host, errmsg(errno));
			log(L_NOTICE, buf);
			return(FAIL);
		}
	}

	if ((socket1 = dup(socket0)) < 0) {
		sprintf(buf, "dup(%d): %s", socket0, errmsg(errno));
		log(L_WARNING, buf);
		(void) close(socket0);
		return(FAIL);
	}

	if ((rmt_rd = fdopen(socket0, rmode)) == (FILE *)NULL) {
		sprintf(buf, e_fdopen, socket0, rmode);
		log(L_WARNING, buf);
		(void) close(socket0);
		(void) close(socket1);
		return(FAIL);
	}

	if ((rmt_wr = fdopen(socket1, wmode)) == (FILE *)NULL) {
		sprintf(buf, e_fdopen, socket1, wmode);
		log(L_WARNING, buf);
		(void) fclose(rmt_rd);
		rmt_rd = (FILE *)NULL;
		(void) close(socket1);
		return(FAIL);
	}

	switch(readreply(buf, sizeof(buf))) {
	case OK_CANPOST:
	case OK_NOPOST:
		if (ferror(rmt_rd)) {
			goodbye(DONT_WAIT);
			return(FAIL);
		}
		break;
	default:
		if (buf[0] != '\0') {
			char	err[BUFSIZ];

			sprintf(err, "%s greeted us with %s", host, buf);
			log(L_NOTICE, err);
		}
		goodbye(DONT_WAIT);
		return(FAIL);
	}
	return(NULL);
}

/*
** Say goodbye to the nice remote server.
**
** We trap SIGPIPE because the socket might already be gone.
*/
goodbye(wait_for_reply)
int	wait_for_reply;
{
	register ifunp	pstate = signal(SIGPIPE, SIG_IGN);

	if (sendcmd("QUIT"))
		wait_for_reply = FALSE;	/* override, something's wrong. */
	/*
	** I don't care what they say to me; this is just being polite.
	*/
	if (wait_for_reply) {
		char	buf[BUFSIZ];

		(void) readreply(buf, sizeof(buf));
	}
	(void) fclose(rmt_rd);
	rmt_rd = (FILE *)NULL;
	(void) fclose(rmt_wr);
	rmt_wr = (FILE *)NULL;
	if (pstate != (ifunp)(-1));
		(void) signal(SIGPIPE, pstate);
}

static
to_sfgets()
{
	longjmp(SFGstack, 1);
}

/*
** `Safe' fgets, ala sendmail. This fgets will timeout after some
** period of time, on the assumption that if the remote did not
** return, they're gone.
** WARNING: contains a possibly unportable reference to stdio
** error macros.
*/
char *
sfgets(buf, size, fp)
char	*buf;
int	size;
FILE	*fp;
{
	register char	*ret;
	int	esave;

	if (buf == (char *)NULL || size <= 0 || fp == (FILE *)NULL)
		return((char *)NULL);
	if (setjmp(SFGstack)) {
		(void) alarm(0);		/* reset alarm clock */
		(void) signal(SIGALRM, SIG_DFL);
#ifdef apollo
		fp->_flag |= _SIERR;
#else
		fp->_flag |= _IOERR;		/* set stdio error */
#endif
#ifndef ETIMEDOUT
		errno = EPIPE;			/* USG doesn't have ETIMEDOUT */
#else
		errno = ETIMEDOUT;		/* connection timed out */
#endif
		return((char *)NULL);		/* bad read, remote time out */
	}
	(void) signal(SIGALRM, to_sfgets);
	(void) alarm(TIMEOUT);
	ret = fgets(buf, size, fp);
	esave = errno;
	(void) alarm(0);			/* reset alarm clock */
	(void) signal(SIGALRM, SIG_DFL);	/* reset SIGALRM */
	errno = esave;
	return(ret);
}

/*
** Remote fgets - converts CRLF to \n, and returns NULL on `.' EOF from
** the remote. Otherwise it returns its first argument, like fgets(3).
*/
char *
rfgets(buf, size, fp)
char	*buf;
int	size;
FILE	*fp;
{
	register char	*cp = buf;
	register int	len;

	if (buf == (char *)NULL || size <= 0 || fp == (FILE *)NULL)
		return((char *)NULL);
	*cp = '\0';
	if (sfgets(buf, size, fp) == (char *)NULL)
		return((char *)NULL);

	/* <CRLF> => '\n' */
	if ((len = strlen(buf)) > 2 && *(cp = &buf[len - 2]) == '\r') {
		*cp++ = '\n';
		*cp = '\0';
	}

	/* ".\n" => EOF */
	cp = buf;
	if (*cp++ == '.' && *cp == '\n') {
		return((char *)NULL);	/* EOF */
	}

	/* Dot escaping */
	if (buf[0] == '.')
		(void) strcpy(&buf[0], &buf[1]);
	return(buf);
}

/*
** send the contents of an open file descriptor to the remote,
** with appropriate RFC822 filtering (e.g. CRLF line termination,
** and dot escaping). Return FALSE if something went wrong.
*/
sendfile(fp)
FILE	*fp;
{
	register int	c;
	register FILE	*remote = rmt_wr;
	register int	nl = TRUE;	/* assume we start on a new line */

/*
** I'm using putc() instead of fputc();
** why do a subroutine call when you don't have to?
** Besides, this ought to give the C preprocessor a work-out.
*/
#define	PUTC(c)	if (putc(c, remote) == EOF) return(FALSE)

	if (fp == (FILE *)NULL)
		return(FALSE);

	/*
	** the second test makes no sense to me,
	** but System V apparently needed it...
	*/
	while((c = fgetc(fp)) != EOF && !feof(fp)) {
		switch(c) {
		case '\n':
			PUTC('\r');		/* \n -> \r\n */
			PUTC(c);
			nl = TRUE;		/* for dot escaping */
			break;
		case '.':
			if (nl) {
				PUTC(c);	/* add a dot */
				nl = FALSE;
			}
			PUTC(c);
			break;
		default:
			PUTC(c);
			nl = FALSE;
			break;
		}
	}
	if (!nl) {
		PUTC('\r');
		PUTC('\n');
	}
	return( !(sendcmd(".") || ferror(fp)) );
}
