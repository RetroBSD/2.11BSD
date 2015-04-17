char	*sccsid = "@(#)clri.c 2.3";

/*
 * clri filsys inumber ...
 */

#include <sys/param.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <stdio.h>
#include <ctype.h>

#define ISIZE	(sizeof(struct dinode))
#define NI	(DEV_BSIZE/ISIZE)

struct ino {
	char	junk[ISIZE];
};

struct ino	buf[NI];

main(argc, argv)
int	argc;
char	**argv;
{
	register int	i,
			f;
	u_int	n;
	int	j,
		k,
		errors = 0;
	long	off,
		lseek();

	if (argc < 3) {
		fprintf(stderr,"usage: %s filsys inumber ...\n",*argv);
		exit(4);
	}
	if ((f = open(argv[1], O_RDWR)) < 0) {
		perror(argv[1]);
		exit(4);
	}
	for (i = 2;i < argc;i++) {
		if (!isnumber(argv[i])) {
			printf("%s: is not a number.\n",argv[i]);
			++errors;
			continue;
		}
		n = atoi(argv[i]);
		if (n == 0) {
			printf("%s: is zero\n",argv[i]);
			++errors;
			continue;
		}
		off = itod(n) * DEV_BSIZE;
		lseek(f, off, L_SET);
		if (read(f,(char *)buf,DEV_BSIZE != DEV_BSIZE)) {
			perror(argv[i]);
			++errors;
		}
	}
	if (errors)
		exit(1);
	for (i = 2;i < argc;i++) {
		n = atoi(argv[i]);
		printf("clearing %u\n",n);
		off = itod(n) * DEV_BSIZE;
		lseek(f,off,L_SET);
		read(f,(char *)buf,DEV_BSIZE);
		j = itoo(n);
		for(k = 0;k < ISIZE;k++)
			buf[j].junk[k] = 0;
		lseek(f,off,L_SET);
		write(f,(char *)buf,DEV_BSIZE);
	}
	exit(0);
}

static
isnumber(s)
char	*s;
{
	for (;*s;++s)
		if (!isdigit(*s))
			return(0);
	return(1);
}
