/*
 * pi - Pascal interpreter code translator
 *
 * Charles Haley, Bill Joy UCB
 * Version 1.2 January 1979
 *
 *
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 *
 * 1996/3/22 - make Perror look like that in ../pi/subr.c
 */

#include "0.h"
#include <sys/types.h>
#include <sys/uio.h>

#ifndef PI1
/*
 * Does the string fp end in '.' and the character c ?
 */
dotted(fp, c)
	register char *fp;
	char c;
{
	register int i;

	i = strlen(fp);
	return (i > 1 && fp[i - 2] == '.' && fp[i - 1] == c);
}

/*
 * Toggle the option c.
 */
togopt(c)
	char c;
{
	register char *tp;

	tp = &opts[c-'a'];
	*tp = 1 - *tp;
}

/*
 * Set the time vector "tvec" to the
 * modification time stamp of the current file.
 */
gettime()
{
	struct stat stbuf;

	stat(filename, stbuf);
	tvec = stbuf.st_mtime;
}

/*
 * Convert a "ctime" into a Pascal style time line
 */
myctime(tv)
	time_t *tv;
{
	register char *cp, *dp;
	char *cpp;
	register i;
	static char mycbuf[26];

	cpp = ctime(tv);
	dp = mycbuf;
	cp = cpp;
	cpp[16] = 0;
	while (*dp++ = *cp++);
	dp--;
	cp = cpp+19;
	cpp[24] = 0;
	while (*dp++ = *cp++);
	return (mycbuf);
}

/*
 * Is "fp" in the command line list of names ?
 */
inpflist(fp)
	char *fp;
{
	register i, *pfp;

	pfp = pflist;
	for (i = pflstc; i > 0; i--)
		if (strcmp(fp, *pfp++) == 0)
			return (1);
	return (0);
}
#endif

/*
 * Boom!
 *
 * Can't use 'fprintf(stderr...)' because that would require stdio.h and
 * that can't be used because the 'ferror' macro would conflict with the routine
 * of the same name.   But we don't want to use sys_errlist[] because that's
 * ~2kb of D space.
*/

Perror(file, mesg)
	char *file, *mesg;
{
	struct	iovec	iov[3];

	iov[0].iov_base = file;
	iov[0].iov_len = strlen(file);
	iov[1].iov_base = ": ";
	iov[1].iov_len = 2;
	iov[2].iov_base = mesg;
	iov[2].iov_len = strlen(mesg);
	writev(2, iov, 3);
}

calloc(num, size)
	int num, size;
{
	register int p1, *p2, nbyte;

	nbyte = (num*size+1) & ~01;
	if ((p1 = alloc(nbyte)) == -1 || p1==0)
		return (-1);
	p2 = p1;
	nbyte >>= 1;		/* 2 bytes/word */
	do {
		*p2++ = 0;
	} while (--nbyte);
	return (p1);
}

copy(to, from, bytes)
	register char *to, *from;
	register int bytes;
{

	if (bytes != 0)
		do
			*to++ = *from++;
		while (--bytes);
}

/*
 * Is ch one of the characters in the string cp ?
 */
any(cp, ch)
	register char *cp;
	char ch;
{

	while (*cp)
		if (*cp++ == ch)
			return (1);
	return (0);
}

opush(c)
	register CHAR c;
{

	c -= 'a';
	optstk[c] <<= 1;
	optstk[c] |= opts[c];
	opts[c] = 1;
#ifdef PI0
	send(ROPUSH, c);
#endif
}

opop(c)
	register CHAR c;
{

	c -= 'a';
	opts[c] = optstk[c] & 1;
	optstk[c] >>= 1;
#ifdef PI0
	send(ROPOP, c);
#endif
}
