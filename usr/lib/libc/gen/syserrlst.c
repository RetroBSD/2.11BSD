/*
 *      Program Name:   syserrlst.c
 *      Date: March 6, 1996
 *      Author: S.M. Schultz
 *
 *      -----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     06Mar96         1. Initial release into the public domain.
*/

/*
 * syserrlst - reads an error message from _PATH_SYSERRLST to a static
 *	       buffer.
 *
 * __errlst - same as above but the caller must specify an error list file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>
#include <errlst.h>

static	char	msgstr[64];
	char	*__errlst();

char *
syserrlst(err)
	int	err;
	{

	return(__errlst(err, _PATH_SYSERRLST));
	}

/*
 * Returns NULL if an error is encountered.  It is the responsiblity of the
 * caller (strerror(3) is most cases) to convert NULL into a "Error N".
*/

char *
__errlst(err, path)
	int	err;
	char	*path;
	{
	register int	f;
	register char	*retval = NULL;
	int	saverrno;
	struct	iovec	iov[2];
	off_t	off;
	struct	ERRLST	emsg;
	struct	ERRLSTHDR ehdr;

	saverrno = errno;
	f = open(path, O_RDONLY);
	if	(f < 0 || !path)
		goto oops;

	if	(read(f, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
		goto oops;

	if	(err < 0 || err > ehdr.maxmsgnum || ehdr.maxmsgnum <= 0 ||
		 ehdr.magic != ERRMAGIC)
		goto	oops;

	off = sizeof (ehdr) + ((off_t)sizeof (struct ERRLST) * err);
	if	(lseek(f, off, L_SET) == (off_t)-1)
		goto oops;

	if	(read(f, &emsg, sizeof (emsg)) != sizeof (emsg))
		goto oops;

	if	(emsg.lenmsg >= sizeof (msgstr))
		emsg.lenmsg = sizeof (msgstr) - 1;
	
	if	(lseek(f, emsg.offmsg, L_SET) == (off_t)-1)
		goto oops;

	if	(read(f, msgstr, emsg.lenmsg) != emsg.lenmsg)
		goto oops;

	retval = msgstr;
	retval[emsg.lenmsg] = '\0';
oops:
	if	(f >= 0)
		close(f);
	errno = saverrno;
	return(retval);
	}
