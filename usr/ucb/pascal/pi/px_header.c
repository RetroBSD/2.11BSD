static	char	sccsid[] = "@(#)px_header.c	2.2";	/*	SCCS id keyword	*/
/* Copyright (c) 1979 Regents of the University of California */

extern	errno;

#define	ETXTBSY	26

struct header 
    {
	int		magic;
	unsigned	txt_size;
	unsigned	data_size;
	unsigned	bss_size;
	unsigned	syms_size;
	unsigned	entry_point;
	unsigned	tr_size;
	unsigned	dr_size;
    };

#define	HEADER_BYTES	1024
#define	ADDR_LC		(HEADER_BYTES-sizeof (struct header))  - sizeof (short)

main(argc, argv)
	register int argc;
	register char *argv[];
{
	register int i, j;
	register short *ip;
	int largv[512], pv[2];

	if (argc > 510) {
		error("Too many arguments.\n");
		exit(1);
	}
	largv[0] = argv[0];
	largv[1] = "-";
	for (i = 1; i < argc; i++)
		largv[i + 1] = argv[i];
	largv[argc + 1] = 0;
	pipe(pv);
	i = fork();
	if (i == -1)
		error("Try again.\n");
	if (i == 0) {
		close(pv[0]);
		write(pv[1], ADDR_LC, sizeof ( short ));
		ip = ADDR_LC;
		i = *ip++;
		while (i != 0) {
			j = (i > 0 && i < 512) ? i : 512;
			write(pv[1], ip, j);
			ip += 512 / sizeof ( short );
			i -= j;
		}
		exit(1);
	}
	close(pv[1]);
	if (pv[0] != 3) {
		close(3);
		dup(pv[0]);
		close(pv[0]);
	}
	execv("/usr/ucb/px", largv);
	error("Px not found.\n");
}

error(cp)
	register char *cp;
{
	register int i;
	register char *dp;

	dp = cp;
	i = 0;
	while (*dp++)
		i++;
	write(2, cp, i);
	exit(1);
}

exit(i)
{
	_exit(i);
}
