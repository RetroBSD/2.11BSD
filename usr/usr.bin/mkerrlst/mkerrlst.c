/*
 *      Program Name:   mkerrlst.c
 *      Date: March 5, 1996
 *      Author: S.M. Schultz
 *
 *      -----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     05Mar96         1. Initial release into the public domain.
 *	1.1	14Mar96		2. Add ability to take messages from a file
 *				   in place of a fixed array.
*/

/*
 * mkerrlst - writes the error messages from sys_errlist[] or a input file of
 *	      strings to a formatted file prepending a header for quick access.
 *
 * This code can be used as a template for creating additional error message
 * files.  The "-o" option can be used to specify a different output file 
 * than _PATH_SYSERRLST.  The "-i" option is used to specify an input file
 * of error messages (one per line, maximum length 80 characters).  Error
 * messages are numbered starting at 0.
 *
 * If no "-i" option is used the sys_errlist[] array is used by default.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errlst.h>
#include <string.h>

	int	outf;
	struct	iovec	iov[2];
	off_t	msgoff;
	char	*outfn, *infn, *msg, *getmsg();
	struct	ERRLSTHDR	ehdr;
	FILE	*infp;

extern	char	*__progname;
extern	char	*sys_errlist[];
extern	int	sys_nerr;

main(argc,argv)
	{
	register int	c;
	int	len, maxlen = -1;

	while	((c = getopt(argc, argv, "o:i:")) != EOF)
		{
		switch	(c)
			{
			case	'o':
				outfn = optarg;
				break;
			case	'i':
				infn = optarg;
				break;
			case	'?':
			default:
				usage();
			}
		}

	if	(!outfn)
		outfn = _PATH_SYSERRLST;

	if	((outf = open(outfn, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
		err(1, "can't create %s", outfn);
	if	(fchmod(outf, 0644) < 0)
		err(1, "fchmod(%d,644)", outf);

	if	(infn)
		{
		infp = fopen(infn, "r");
		if	(!infp)
			err(1, "fopen(%s, r) failed\n", infn);
		}

	for	(c = 0; msg = getmsg(c, infp); c++)
		{
		len = strlen(msg);
		if	(len > maxlen)
			maxlen = len;
		}

	if	(infp)
		rewind(infp);

/*
 * Adjust the count so that the int written to the file is the highest valid
 * message number rather than the number of the first invalid error number.
 *
 * At the present time nothing much uses the maximum message length.
*/
	ehdr.magic = ERRMAGIC;
	ehdr.maxmsgnum = c - 1;
	ehdr.maxmsglen = maxlen;
	if	(write(outf, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
		err(1, "write");

	msgoff = sizeof (ehdr) + ((off_t)sizeof (struct ERRLST) * sys_nerr);
	iov[0].iov_base = (caddr_t)&msgoff;
	iov[0].iov_len = sizeof (off_t);

	iov[1].iov_len = sizeof (int);

	for	(c = 0; msg = getmsg(c, infp); c++)
		{
		len = strlen(msg);
		iov[1].iov_base = (caddr_t)&len;
		writev(outf, iov, 2);
		msgoff += len;
		msgoff++;		/* \n between messages */
		}

	iov[1].iov_base = "\n";
	iov[1].iov_len = 1;

	if	(infp)
		rewind(infp);

	for	(c = 0; msg = getmsg(c, infp); c++)
		{
		iov[0].iov_base = msg;
		iov[0].iov_len = strlen(iov[0].iov_base);
		writev(outf, iov, 2);
		}
	close(outf);
	if	(infp)
		fclose(infp);
	exit(0);
	}

char *
getmsg(c, fp)
	int	c;
	FILE	*fp;
	{
	static	char	buf[81];
	register char	*cp;

	if	(fp)
		{
		cp = fgets(buf, sizeof (buf) - 1, fp);
		if	(!cp)
			return(NULL);
		cp = index(buf, '\n');
		if	(cp)
			*cp = '\0';
		return(buf);
		}
	if	(c < sys_nerr)
		return(sys_errlist[c]);
	return(NULL);
	}

usage()
	{
	fprintf(stderr, "usage: %s -o filename\n", __progname);
	exit(1);
	}
