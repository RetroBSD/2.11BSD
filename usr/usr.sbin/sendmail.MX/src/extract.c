/* string extraction/restoration routines */

#define BUFLEN 256
#include "sendmail.h"
#include <varargs.h>

char	*StringFile =	"/usr/share/misc/sendmail.sr";	/* extracted string storage */
static int strfile = -1, ourpid = 0;

errprep(offset, buf)
unsigned short offset;
char *buf;
{
register int pid = getpid();

	if (pid != ourpid) {
		ourpid = pid;
		if (strfile >= 0) {
			close(strfile);
			strfile = -1;
		}
	}
	if (strfile < 0) {
		strfile = open(StringFile, 0);
		if (strfile < 0) {
oops:
			QuickAbort++;
			syserr("Cannot find strings");
		}
	}
	if (lseek(strfile, (long) offset, 0) < 0
			|| read(strfile, buf, BUFLEN) <= 0)
		goto oops;
}

/* extracted string front end for printf() */
/*VARARGS1*/
strprerror(fmt, va_alist)
	int fmt;
	va_dcl
{
	va_list ap;
	char buf[BUFLEN];

	errprep(fmt, buf);
	va_start(ap);
	vprintf(buf, ap);
	va_end(ap);
}

/* extracted string front end for sprintf() */
/*VARARGS1*/
strsrerror(fmt, obuf, va_alist)
	int fmt;
	char *obuf;
	va_dcl
{
	char buf[BUFLEN];
	va_list ap;

	errprep(fmt, buf);
	va_start(ap);
	vsprintf(obuf, buf, ap);
	va_end(ap);
}

/* extracted string front end for fprintf() */
/*VARARGS1*/
strfrerror(fmt, fd, va_alist)
	int fmt, fd;
	va_dcl
{
	va_list ap;
	char buf[BUFLEN];

	errprep(fmt, buf);
	va_start(ap);
	vfprintf(fd, buf, ap);
	va_end(ap);
}

/* extracted string front end for syslog() */
/*VARARGS1*/
slogerror(fmt, pri, a, b, c, d, e)
	int fmt, pri;
{
	char buf[BUFLEN];

	errprep(fmt, buf);
	syslog(pri, buf, a, b, c, d, e);
}

/* extracted string front end for syserr() */
/*VARARGS1*/
syserror(fmt, a, b, c, d, e)
	int fmt;
{
	char buf[BUFLEN];
	extern int errno;
	register int olderrno = errno;

	errprep(fmt, buf);
	errno = olderrno;
	syserr(buf, a, b, c, d, e);
}

/* extracted string front end for usrerr() */
/*VARARGS1*/
usrerror(fmt, a, b, c, d, e)
	int fmt;
{
	char buf[BUFLEN];
	extern int errno;
	register int olderrno = errno;

	errprep(fmt, buf);
	errno = olderrno;
	usrerr(buf, a, b, c, d, e);
}

/* extracted string front end for nmessage() */
/*VARARGS2*/
nmesserror(fmt, num, a, b, c, d, e)
	int fmt;
	char *num;
{
	char buf[BUFLEN];

	errprep(fmt, buf);
	nmessage(num, buf, a, b, c, d, e);
}
/* extracted string front end for message() */
/*VARARGS2*/
messerror(fmt, num, a, b, c, d, e)
	int fmt;
	char *num;
{
	char buf[BUFLEN];

	errprep(fmt, buf);
	message(num, buf, a, b, c, d, e);
}
