/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)main.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * sysconfig -- Program to auto configure a kernel to the devices which
 * are present.  Needs the ucall() system call and special kernel to work.
 */

#include <machine/autoconfig.h>
#include <stdio.h>

char	*nlist_name = "/unix",		/* kernel */
	*dtab_name = "/etc/dtab",	/* dtab file */
	*myname;			/* program name */
int	kmem,
	verbose = NO,
	debug = NO,
	complain = NO,
	pflag = NO;

main(argc,argv)
int	argc;
char	**argv;
{
	extern char	*optarg;
	static char	*kmem_name = "/dev/kmem";
	int	c;
	char	*C,
		*rindex();

	setbuf(stdout, NULL);
	myname = (C = rindex(*argv,'/')) ? ++C : *argv;
	while((c = getopt(argc,argv,"Pcdi:k:n:v")) != EOF)
		switch((char)c) {
			case 'P':	/* pflag, ask Mike */
				pflag = YES;
				break;
			case 'c':	/* complain about bad vectors */
				complain = YES;
				break;
			case 'd':	/* debugging run */
				debug = YES;
				break;
			case 'i':	/* not dtab, different file */
				dtab_name = optarg;
				break;
			case 'k':	/* not /dev/kmem, different file */
				kmem_name = optarg;
				break;
			case 'n':	/* not /unix, different file */
				nlist_name = optarg;
				break;
			case 'v':	/* verbose output */
				verbose = YES;
				break;
			default:
				fputs("usage: ",stderr);
				fputs(myname,stderr);
				fputs(" [-c] [-d] [-v] [-i file] [-k file] [-n file]\n",stderr);
				exit(AC_SETUP);
		}

	if ((kmem = open(kmem_name, 2)) < 0) {
		perror(kmem_name);
		exit(AC_SETUP);
	}

	/* Read the dtab into internal tables so we can play with it */
	read_dtab();

	/* Now set up for and call nlist so we can get kernel symbols */
	read_nlist();

	/* And at last change the kernel to suit ourselves */
	auto_config();

	/* All done go bye bye now */
	exit(AC_OK);
}
