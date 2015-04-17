#
/*
 * px - interpreter for Berkeley Pascal
 * Version 1.0 August 1977
 *
 * Bill Joy, Charles Haley, Ken Thompson
 *
 * 1996/3/22 - make Perror look like that in ../pi/subr.c
 */

#include "0x.h"
#include "opcode.h"
#include "E.h"
#include <sys/types.h>
#include <sys/uio.h>

int	display[20]	= { display };

int	onintr();

main(ac, av)
	int ac;
	char *av[];
{
	register char *cp;
	register int bytes, rmdr;
	int size, *bp, i, of;

	i = signal(2, 1);
	argc = ac - 1, argv = av + 1;
	randim = 1./randm;
	setmem();
	if (av[0][0] == '-' && av[0][1] == 'o') {
		av[0] += 2;
		file = av[0];
		argv--, argc++;
		discard++;
	} else if (argc == 0)
		file = *--argv = "obj", argc++;
	else if (argv[0][0] == '-' && argv[0][1] == 0) {
		argv[0][0] = 0;
		file = 0;
		argv[0] = argv[-1];
	} else
		file = *argv;
	if (file) {
		cp = file;
		of = open(cp, 0);
		if (discard)
			unlink(cp);
	} else
		of = 3;
	if ((i & 01) == 0)
		signal(2, onintr);
	if (of < 0) {
oops:
		perror(cp);
		exit(1);
	}
	if (file) {
#include <sys/types.h>
#include <sys/stat.h>
		struct stat stb;
		fstat(of, &stb);
		size = stb.st_size;
	} else
		if (read(of, &size, 2) != 2) {
			ferror("Improper argument");
			exit(1);
		}
	if (size == 0) {
		ferror("File is empty");
		exit(1);
	}
	if (file) {
		read(of, &i, 2);
		if (i == 0407) {
			size -= 1024;
			lseek(of, (long)1024, 0);
		} else
			lseek(of, (long)0, 0);
	}
	bp = cp = alloc(size);
	if (cp == -1) {
		ferror("Too large");
		exit(1);
	}
	rmdr = size;
	while (rmdr != 0) {
		i = (rmdr > 0 && rmdr < 512) ? rmdr : 512;
		bytes = read(of, cp, i);
		if (bytes <= 0) {
			ferror("Unexpected end-of-file");
			exit(1);
		}
		rmdr -= bytes;
		cp += bytes;
	}
	if (read(of, cp, 1) == 1) {
		ferror("Expected end-of-file");
		exit(1);
	}
	close(of);
	if (file == 0)
		wait(&i);
	if (*bp++ != 0404) {
		ferror("Not a Pascal object file");
		exit(1);
	}
	if (discard && bp[(bp[0] == O_PXPBUF ? bp[5] + 8 : bp[1]) / 2 + 1] != O_NODUMP)
		write(2, "Execution begins...\n", 20);
	interpret(bp, size);
}

/*
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

/*
 * Initialization of random number "constants"
 */
long	seed	= 7774755.;
double	randa	= 62605.;
double	randc	= 113218009.;
double	randm	= 536870912.;

/*
 * Routine to put a string on the current
 * pascal output given a pointer to the string
 */
puts(str)
	char *str;
{
	register char *cp;

	cp = str;
	while (*cp)
		pputch(*cp++);
}

ferror(cp)
	char *cp;
{

	Perror(file, cp);
}

onintr()
{
	extern int draino[];

	if (dp == 0)
		exit(1);
	draino[0] = 512;
	draino[1] = &draino[2];
	error(EINTR);
}
