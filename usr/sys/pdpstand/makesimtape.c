/*
 *	@(#)makesimtape.c	2.1 (2.11BSD) 1998/12/31
 *		Hacked 'maketape.c' to write a file in a format suitable for
 *		use with Bob Supnik's PDP-11 simulator (V2.3) emulated tape 
 *		driver.
 *
 * 	NOTE: a PDP-11 has to flip the shorts within the long when writing out
 *	      the record size.  Seems a PDP-11 is neither a little-endian
 *	      machine nor a big-endian one.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#define MAXB 30

	char	buf[MAXB * 512];
	char	name[50];
	long	recsz, flipped, trl();
	int	blksz;
	int	mt, fd, cnt;
	struct	iovec	iovec[3];
	struct	iovec	tmark[2];
	void	usage();

main(argc, argv)
	int argc;
	char *argv[];
	{
	int i, j = 0, k = 0;
	long zero = 0;
	register char	*outfile = NULL, *infile = NULL;
	FILE *mf;
	struct	stat	st;

	while	((i = getopt(argc, argv, "i:o:")) != EOF)
		{
		switch	(i)
			{
			case	'o':
				outfile = optarg;
				break;
			case	'i':
				infile = optarg;
				break;
			default:
				usage();
				/* NOTREACHED */
			}
		}
	if	(!outfile || !infile)
		usage();
		/* NOTREACHED */
/*
 * Stat the outfile and make sure it either 1) Does not exist, or
 * 2) Exists but is a regular file.
*/
	if	(stat(outfile, &st) != -1 && !(S_ISREG(st.st_mode)))
		errx(1, "outfile must either not exist or be a regular file");
		/* NOTREACHED */

	mt = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if	(mt < 0)
		err(1, "Can not create %s", outfile);
		/* NOTREACHED */

	mf = fopen(infile, "r");
	if	(!mf)
		err(1, "Can not open %s", infile);
		/* NOTREACHED*/

	tmark[0].iov_len = sizeof (long);
	tmark[0].iov_base = (char *)&zero;

	while	(1)
		{
		if	((i = fscanf(mf, "%s %d", name, &blksz))== EOF)
			exit(0);
		if	(i != 2) {
			fprintf(stderr,"Help! Scanf didn't read 2 things (%d)\n", i);
			exit(1);
			}
		if	(blksz <= 0 || blksz > MAXB)
			{
			fprintf(stderr, "Block size %u is invalid\n", blksz);
			exit(1);
			}
		recsz = blksz * 512;	/* convert to bytes */
		iovec[0].iov_len = sizeof (recsz);
#ifdef	pdp11
		iovec[0].iov_base = (char *)&flipped;
#else
		iovec[0].iov_base = (char *)&recsz;
#endif
		iovec[1].iov_len = (int)recsz;
		iovec[1].iov_base = buf;
		iovec[2].iov_len =  iovec[0].iov_len;
		iovec[2].iov_base = iovec[0].iov_base;

		if	(strcmp(name, "*") == 0)
			{
			if	(writev(mt, tmark, 1) < 0)
				warn(1, "writev of pseudo tapemark failed");
			k++;
			continue;
			}
		fd = open(name, 0);
		if	(fd < 0)
			err(1, "Can't open %s for reading", name);
			/* NOTREACHED */
		printf("%s: block %d, file %d\n", name, j, k);

		/*
		 * we pad the last record with nulls
		 * (instead of the bell std. of padding with trash).
		 * this allows you to access text files on the
		 * tape without garbage at the end of the file.
		 * (note that there is no record length associated
		 *  with tape files)
		 */

		while	((cnt=read(fd, buf, (int)recsz)) == (int)recsz)
			{
			j++;
#ifdef	pdp11
			flipped = trl(recsz);
#endif
			if	(writev(mt, iovec, 3) < 0)
				err(1, "writev #1");
				/* NOTREACHED */
			}
		if	(cnt > 0)
			{
			j++;
			bzero(buf + cnt, (int)recsz - cnt);
#ifdef	pdp11
			flipped = trl(recsz);
#endif
			if	(writev(mt, iovec, 3) < 0)
				err(1, "writev #2");
				/* NOTREACHED */
			}
		close(fd);
		}
/*
 * Write two tape marks to simulate EOT
*/
	writev(mt, tmark, 1);
	writev(mt, tmark, 1);
	}

long
trl(l)
	long	l;
	{
	union	{
		long	l;
		short	s[2];
		} foo;
	register short	x;

	foo.l = l;
	x = foo.s[0];
	foo.s[0] = foo.s[1];
	foo.s[1] = x;
	return(foo.l);
	}

void
usage()
	{
	fprintf(stderr, "usage: makesimtape -o outfilefile -i inputfile\n");
	exit(1);
	}
