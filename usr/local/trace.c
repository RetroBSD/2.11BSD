#include <stdio.h>
#include <nlist.h>
#include <signal.h>
#include <sys/trace.h>

struct	nlist nl[] =
	{
	{ "_tracebuf" },
	{ "" },
	};

	int	mf, hdr(), lines = 1;

main(argc, argv)
	int argc;
	char **argv;
	{
	register i;
	int	iter;

	nlist("/vmunix", nl);
	if	(nl[0].n_type == 0)
		{
		fprintf(stderr, "no /vmunix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if	(mf < 0)
		{
		fprintf(stderr, "cannot open /dev/kmem\n");
		exit(1);
		}
	iter = 0;
	argc--, argv++;
	if	(argc > 1)
		iter = atoi(argv[1]);
	signal(SIGCONT, hdr);
loop:
	if	(--lines == 0)
		hdr();
	nmstats();
	fflush(stdout);
	if	(--iter &&argc > 0)
		{
		sleep(atoi(argv[0]));
		goto loop;
		}
	}

hdr()
	{

	puts("bhit      bmiss     bwrite    bhitra    bmissra   brelse    %bhit");
	lines = 19;
	}

nmstats()
	{
	float	pct;
	long	tracebuf[TR_NUM_210];

	lseek(mf, (long)nl[0].n_value, 0);
	read(mf, tracebuf, sizeof tracebuf);
	pct = tracebuf[TR_BREADHIT];
	pct /= (tracebuf[TR_BREADHIT] + tracebuf[TR_BREADMISS]);
	pct *= 100.0;
	printf("%-9D %-9D %-9D %-9D %-9D %-9D %.2f\n",
		tracebuf[TR_BREADHIT],
		tracebuf[TR_BREADMISS],
		tracebuf[TR_BWRITE],
		tracebuf[TR_BREADHITRA],
		tracebuf[TR_BREADMISSRA],
		tracebuf[TR_BRELSE],
		pct);
	}
