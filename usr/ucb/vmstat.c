/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)vmstat.c	5.4.2 (2.11BSD GTE) 1997/3/28";
#endif

#include <stdio.h>
#include <ctype.h>
#include <nlist.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/vm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/dir.h>
#include <sys/inode.h>
#include <sys/namei.h>
#ifdef pdp11
#include <machine/machparam.h>
#include <sys/text.h>
#endif

struct nlist nl[] = {
#define	X_CPTIME	0
	{ "_cp_time" },
#define	X_RATE		1
	{ "_rate" },
#define X_TOTAL		2
	{ "_total" },
#define	X_DEFICIT	3
	{ "_deficit" },
#define	X_FORKSTAT	4
	{ "_forkstat" },
#define X_SUM		5
	{ "_sum" },
#define	X_FIRSTFREE	6
	{ "_firstfree" },
#define	X_MAXFREE	7
	{ "_maxfree" },
#define	X_BOOTTIME	8
	{ "_boottime" },
#define	X_DKXFER	9
	{ "_dk_xfer" },
#define X_REC		10
	{ "_rectime" },
#define X_PGIN		11
	{ "_pgintime" },
#define X_HZ		12
	{ "_hz" },
#define X_PHZ		13
	{ "_phz" },
#define X_NCHSTATS	14
	{ "_nchstats" },
#define	X_INTRNAMES	15
	{ "_intrnames" },
#define	X_EINTRNAMES	16
	{ "_eintrnames" },
#define	X_INTRCNT	17
	{ "_intrcnt" },
#define	X_EINTRCNT	18
	{ "_eintrcnt" },
#define	X_DK_NDRIVE	19
	{ "_dk_ndrive" },
#define	X_XSTATS	20
	{ "_xstats" },
#ifdef pdp11
#define	X_DK_NAME	21
	{ "_dk_name" },
#define	X_DK_UNIT	22
	{ "_dk_unit" },
#define X_FREEMEM	23
	{ "_freemem" },
#else
#define X_MBDINIT	21
	{ "_mbdinit" },
#define X_UBDINIT	22
	{ "_ubdinit" },
#endif
	{ "" },
};

#ifdef pdp11
char	**dk_name;
int	*dk_unit;
struct	xstats	pxstats, cxstats;
size_t	pfree;
int	flag29;
#endif
char	**dr_name;
int	*dr_select;
int	dk_ndrive;
int	ndrives = 0;
#ifdef pdp11
char	*defdrives[] = { "rp0", 0 };
#else
char	*defdrives[] = { "hp0", "hp1", "hp2",  0 };
#endif
double	stat1();
int	firstfree, maxfree;
int	hz;
int	phz;
int	HZ;

#if defined(vax) ||  defined(pdp11)
#define	INTS(x)	((x) - (hz + phz) < 0 ? 0 : (x) - (hz + phz))
#endif

struct {
	int	busy;
	long	time[CPUSTATES];
	long	*xfer;
#ifdef pdp11
	struct	vmrate Rate;
	struct	vmtotal	Total;
	struct	vmsum Sum;
#else
	struct	vmmeter Rate;
	struct	vmtotal	Total;
	struct	vmmeter Sum;
#endif
	struct	forkstat Forkstat;
	unsigned rectime;
	unsigned pgintime;
} s, s1, z;
#define	rate		s.Rate
#define	total		s.Total
#define	sum		s.Sum
#define	forkstat	s.Forkstat

#ifdef pdp11
struct	vmsum osum;
#else
struct	vmmeter osum;
#endif
int	deficit;
double	etime;
int 	mf;
time_t	now, boottime;
int	printhdr();
int	lines = 1;
extern	char *calloc();

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *ctime();
	register i;
	int iter, iflag = 0;
	long nintv, t;
	char *arg, **cp, buf[BUFSIZ];

	nlist("/vmunix", nl);
	if(nl[0].n_type == 0) {
		fprintf(stderr, "no /vmunix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if(mf < 0) {
		fprintf(stderr, "cannot open /dev/kmem\n");
		exit(1);
	}
	iter = 0;
	argc--, argv++;
	while (argc>0 && argv[0][0]=='-') {
		char *cp = *argv++;
		argc--;
		while (*++cp) switch (*cp) {

		case 't':
			dotimes();
			exit(0);

		case 'z':
			close(mf);
			mf = open("/dev/kmem", 2);
			lseek(mf, (long)nl[X_SUM].n_value, L_SET);
			write(mf, &z.Sum, sizeof z.Sum);
			exit(0);

		case 'f':
			doforkst();
			exit(0);
		
		case 's':
			dosum();
			exit(0);

		case 'i':
			iflag++;
			break;
#ifdef pdp11
		case 'p':
			flag29++;
			break;
#endif

		default:
			fprintf(stderr,
			    "usage: vmstat [ -fsi ] [ interval ] [ count]\n");
			exit(1);
		}
	}
#ifndef pdp11
	lseek(mf, (long)nl[X_FIRSTFREE].n_value, L_SET);
	read(mf, &firstfree, sizeof firstfree);
	lseek(mf, (long)nl[X_MAXFREE].n_value, L_SET);
	read(mf, &maxfree, sizeof maxfree);
#endif
	lseek(mf, (long)nl[X_BOOTTIME].n_value, L_SET);
	read(mf, &boottime, sizeof boottime);
	lseek(mf, (long)nl[X_HZ].n_value, L_SET);
	read(mf, &hz, sizeof hz);
	if (nl[X_PHZ].n_value != 0) {
		lseek(mf, (long)nl[X_PHZ].n_value, L_SET);
		read(mf, &phz, sizeof phz);
	}
	HZ = phz ? phz : hz;
	if (nl[X_DK_NDRIVE].n_value == 0) {
		fprintf(stderr, "dk_ndrive undefined in system\n");
		exit(1);
	}
	lseek(mf, (long)nl[X_DK_NDRIVE].n_value, L_SET);
	read(mf, &dk_ndrive, sizeof (dk_ndrive));
	if (dk_ndrive <= 0) {
		fprintf(stderr, "dk_ndrive %d\n", dk_ndrive);
		exit(1);
	}
	dr_select = (int *)calloc(dk_ndrive, sizeof (int));
	dr_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_unit = (int *)calloc(dk_ndrive, sizeof (int));
#define	allocate(e, t) \
    s./**/e = (t *)calloc(dk_ndrive, sizeof (t)); \
    s1./**/e = (t *)calloc(dk_ndrive, sizeof (t));
	allocate(xfer, long);
	for (arg = buf, i = 0; i < dk_ndrive; i++) {
		dr_name[i] = arg;
		sprintf(dr_name[i], "dk%d", i);
		arg += strlen(dr_name[i]) + 1;
	}
	read_names();
	time(&now);
	nintv = now - boottime;
	if (nintv <= 0 || nintv > 60L*60L*24L*365L*10L) {
		fprintf(stderr,
		    "Time makes no sense... namelist must be wrong.\n");
		exit(1);
	}
	if (iflag) {
		dointr(nintv);
		exit(0);
	}
	/*
	 * Choose drives to be displayed.  Priority
	 * goes to (in order) drives supplied as arguments,
	 * default drives.  If everything isn't filled
	 * in and there are drives not taken care of,
	 * display the first few that fit.
	 */
	ndrives = 0;
	while (argc > 0 && !isdigit(argv[0][0])) {
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], argv[0]))
				continue;
			dr_select[i] = 1;
			ndrives++;
		}
		argc--, argv++;
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		for (cp = defdrives; *cp; cp++)
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				ndrives++;
				break;
			}
	}
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		ndrives++;
	}
#ifdef pdp11
	/* handle initial retrieval of the xstats structure */
	lseek(mf, (long)nl[X_XSTATS].n_value, L_SET);
	read(mf, &cxstats, sizeof(cxstats));
#endif
	if (argc > 1)
		iter = atoi(argv[1]);
	signal(SIGCONT, printhdr);
loop:
	if (--lines == 0)
		printhdr();
	lseek(mf, (long)nl[X_CPTIME].n_value, L_SET);
 	read(mf, s.time, sizeof s.time);
	lseek(mf, (long)nl[X_DKXFER].n_value, L_SET);
	read(mf, s.xfer, dk_ndrive * sizeof (long));
#ifdef pdp11
	/*
	 * This would be a whole lot easier if the variables in each
	 * were all longs...
	 */
	if (nintv != 1) {
		lseek(mf, (long)nl[X_SUM].n_value, L_SET);
		read(mf, &sum, sizeof(sum));
		rate.v_swtch = sum.v_swtch;
		rate.v_trap = sum.v_trap;
		rate.v_syscall = sum.v_syscall;
		rate.v_intr = sum.v_intr;
		rate.v_pdma = sum.v_pdma;
		rate.v_ovly = sum.v_ovly;
		rate.v_pgin = sum.v_pgin;
		rate.v_pgout = sum.v_pgout;
		rate.v_swpin = sum.v_swpin;
		rate.v_swpout = sum.v_swpout;
	}
	else {
		lseek(mf, (long)nl[X_RATE].n_value, L_SET);
		read(mf, &rate, sizeof rate);
		lseek(mf, (long)nl[X_SUM].n_value, L_SET);
		read(mf, &sum, sizeof sum);
	}
	pxstats = cxstats;
	lseek(mf, (long)nl[X_XSTATS].n_value, L_SET);
	read(mf, &cxstats, sizeof(cxstats));
	lseek(mf, (long)nl[X_FREEMEM].n_value, L_SET);
	read(mf, &pfree, sizeof(pfree));
#else
	if (nintv != 1)
		lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	else
		lseek(mf, (long)nl[X_RATE].n_value, L_SET);
	read(mf, &rate, sizeof rate);
#endif
	lseek(mf, (long)nl[X_TOTAL].n_value, L_SET);
	read(mf, &total, sizeof total);
	osum = sum;
	lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	read(mf, &sum, sizeof sum);
	lseek(mf, (long)nl[X_DEFICIT].n_value, L_SET);
	read(mf, &deficit, sizeof deficit);
	etime = 0;
	for (i=0; i < dk_ndrive; i++) {
		t = s.xfer[i];
		s.xfer[i] -= s1.xfer[i];
		s1.xfer[i] = t;
	}
	for (i=0; i < CPUSTATES; i++) {
		t = s.time[i];
		s.time[i] -= s1.time[i];
		s1.time[i] = t;
		etime += s.time[i];
	}
	if(etime == 0.)
		etime = 1.;
#ifdef pdp11
	printf("%2d%2d%2d", total.t_rq, total.t_dw, total.t_sw);
	/*
	 * We don't use total.t_free because it slops around too much
	 * within this kernel
	 */
	printf("%6D", ctok(total.t_avm));
	if (flag29)
		printf("%4D",
		    total.t_avm ? (total.t_avmtxt * 100) / total.t_avm : 0);
	printf("%6d", ctok(pfree));
#else
	printf("%2d%2d%2d", total.t_rq, total.t_dw+total.t_pw, total.t_sw);
#define pgtok(a) ((a)*NBPG/1024)
	printf("%6d%6d", pgtok(total.t_avm), pgtok(total.t_free));
#endif
#ifdef pdp11
	if (flag29)
		printf("%4D%3D  ", rate.v_swpin / nintv, rate.v_swpout / nintv);
	else {
		printf("%4D",
		    (cxstats.alloc_inuse - pxstats.alloc_inuse) / iter);
		printf("%3D",
		    (cxstats.alloc_cachehit - pxstats.alloc_cachehit) / iter);
		printf("%4D%4D", rate.v_pgin / nintv, rate.v_pgout / nintv);
		printf("%4D", (cxstats.free - pxstats.free) / iter);
		printf("%4D", (cxstats.free_cache - pxstats.free_cache) / iter);
		printf("%4D", rate.v_ovly / nintv);
	}
#else
	printf("%4d%3d", (rate.v_pgrec - (rate.v_xsfrec+rate.v_xifrec))/nintv,
	    (rate.v_xsfrec+rate.v_xifrec)/nintv);
	printf("%4d", pgtok(rate.v_pgpgin)/nintv);
	printf("%4d%4d%4d%4d", pgtok(rate.v_pgpgout)/nintv,
	    pgtok(rate.v_dfree)/nintv, pgtok(deficit), rate.v_scan/nintv);
#endif
	etime /= (float)HZ;
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			stats(i);
#ifdef pdp11
	if (flag29)
		printf("%4D%4D%4D%4D%4D%4D",
		    rate.v_pdma / nintv, INTS(rate.v_intr / nintv),
		    rate.v_syscall / nintv, rate.v_trap / nintv,
		    rate.v_ovly / nintv, rate.v_swtch / nintv);
	else
#endif
	printf("%4D%4D%4D", INTS(rate.v_intr/nintv), rate.v_syscall/nintv,
	    rate.v_swtch/nintv);
	for(i=0; i<CPUSTATES; i++) {
		float f = stat1(i);
#ifdef pdp11
		if (!flag29)
#endif
		if (i == 0) {		/* US+NI */
			i++;
			f += stat1(i);
		}
		printf("%3.0f", f);
	}
	printf("\n");
	fflush(stdout);
	nintv = 1;
	if (--iter &&argc > 0) {
		sleep(atoi(argv[0]));
		goto loop;
	}
}

printhdr()
{
	register int i, j;

#ifdef pdp11
	if (flag29)
	    printf(" procs       memory      swap      ");
	else
	    printf(" procs     memory              page           ");
#else
	printf(" procs     memory              page           ");
#endif
	i = (ndrives * 3 - 6) / 2;
	if (i < 0)
		i = 0;
	for (j = 0; j < i; j++)
		putchar(' ');
	printf("faults");
	i = ndrives * 3 - 6 - i;
	for (j = 0; j < i; j++)
		putchar(' ');
#ifdef pdp11
	if (flag29) {
		printf("              cpu\n");
		printf(" r b w   avm  tx   fre   i  o   ");
	}
	else {
		printf("               cpu\n");
		printf(" r b w   avm   fre  ti tc  pi  po  fr  fc  ov ");
	}
#else
	printf("               cpu\n");
	printf(" r b w   avm   fre  re at  pi  po  fr  de  sr ");
#endif
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			printf("%c%c ", dr_name[i][0], dr_name[i][2]);	
#ifdef pdp11
	if (flag29)
	    printf(" pd  in  sy  tr  ov  cs us ni sy id\n");
	else
#endif
	printf(" in  sy  cs us sy id\n");
	lines = 19;
}

dotimes()
{
#ifdef pdp11
	printf("page in/out/reclamation is not applicable to 2.11BSD\n");
#else
	lseek(mf, (long)nl[X_REC].n_value, L_SET);
	read(mf, &s.rectime, sizeof s.rectime);
	lseek(mf, (long)nl[X_PGIN].n_value, L_SET);
	read(mf, &s.pgintime, sizeof s.pgintime);
	lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	read(mf, &sum, sizeof sum);
	printf("%d reclaims, %d total time (usec)\n", sum.v_pgrec, s.rectime);
	printf("average: %d usec / reclaim\n", s.rectime/sum.v_pgrec);
	printf("\n");
	printf("%d page ins, %d total time (msec)\n",sum.v_pgin, s.pgintime/10);
	printf("average: %8.1f msec / page in\n", s.pgintime/(sum.v_pgin*10.0));
#endif
}

dosum()
{
	struct nchstats nchstats;
	long nchtotal;
	struct xstats  xstats;

	lseek(mf, (long)nl[X_SUM].n_value, L_SET);
	read(mf, &sum, sizeof sum);
	printf("%9D swap ins\n", sum.v_swpin);
	printf("%9D swap outs\n", sum.v_swpout);
	printf("%9D pages swapped in\n", sum.v_pswpin / CLSIZE);
	printf("%9D pages swapped out\n", sum.v_pswpout / CLSIZE);
#ifndef pdp11
	printf("%9D total address trans. faults taken\n", sum.v_faults);
#endif
	printf("%9D page ins\n", sum.v_pgin);
	printf("%9D page outs\n", sum.v_pgout);
#ifndef pdp11
	printf("%9D pages paged in\n", sum.v_pgpgin);
	printf("%9D pages paged out\n", sum.v_pgpgout);
	printf("%9D sequential process pages freed\n", sum.v_seqfree);
	printf("%9D total reclaims (%d%% fast)\n", sum.v_pgrec,
	    (sum.v_fastpgrec * 100) / (sum.v_pgrec == 0 ? 1 : sum.v_pgrec));
	printf("%9D reclaims from free list\n", sum.v_pgfrec);
	printf("%9D intransit blocking page faults\n", sum.v_intrans);
	printf("%9D zero fill pages created\n", sum.v_nzfod / CLSIZE);
	printf("%9D zero fill page faults\n", sum.v_zfod / CLSIZE);
	printf("%9D executable fill pages created\n", sum.v_nexfod / CLSIZE);
	printf("%9D executable fill page faults\n", sum.v_exfod / CLSIZE);
	printf("%9D swap text pages found in free list\n", sum.v_xsfrec);
	printf("%9D inode text pages found in free list\n", sum.v_xifrec);
	printf("%9D file fill pages created\n", sum.v_nvrfod / CLSIZE);
	printf("%9D file fill page faults\n", sum.v_vrfod / CLSIZE);
	printf("%9D pages examined by the clock daemon\n", sum.v_scan);
	printf("%9D revolutions of the clock hand\n", sum.v_rev);
	printf("%9D pages freed by the clock daemon\n", sum.v_dfree / CLSIZE);
#endif
	printf("%9D cpu context switches\n", sum.v_swtch);
	printf("%9D device interrupts\n", sum.v_intr);
	printf("%9D software interrupts\n", sum.v_soft);
#if defined(vax) || defined(pdp11)
	printf("%9D pseudo-dma dz interrupts\n", sum.v_pdma);
#endif
	printf("%9D traps\n", sum.v_trap);
#ifdef pdp11
	printf("%9D overlay emts\n", sum.v_ovly);
#endif
	printf("%9D system calls\n", sum.v_syscall);
#define	nz(x)	((x) ? (x) : 1)
	lseek(mf, (long)nl[X_NCHSTATS].n_value, 0);
	read(mf, &nchstats, sizeof nchstats);
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_badhits +
	    nchstats.ncs_falsehits + nchstats.ncs_miss + nchstats.ncs_long;
	printf("%9D total name lookups", nchtotal);
	printf(" (cache hits %D%% system %D%% per-process)\n",
	    nchstats.ncs_goodhits * 100 / nz(nchtotal),
	    nchstats.ncs_pass2 * 100 / nz(nchtotal));
	printf("%9s badhits %D, falsehits %D, toolong %D\n", "",
	    nchstats.ncs_badhits, nchstats.ncs_falsehits, nchstats.ncs_long);
	lseek(mf, (long)nl[X_XSTATS].n_value, 0);
	read(mf, &xstats, sizeof xstats);
	printf("%9D total calls to xalloc (cache hits %D%%)\n",
	    xstats.alloc, xstats.alloc_cachehit * 100 / nz(xstats.alloc));
	printf("%9s sticky %ld flushed %ld unused %ld\n", "",
	    xstats.alloc_inuse, xstats.alloc_cacheflush, xstats.alloc_unused);
	printf("%9D total calls to xfree", xstats.free);
	printf(" (sticky %D cached %D swapped %D)\n",
	    xstats.free_inuse, xstats.free_cache, xstats.free_cacheswap);
}

#ifdef pdp11
char Pages[] = "clicks";
#else
char Pages[] = "pages";
#endif

doforkst()
{
	lseek(mf, (long)nl[X_FORKSTAT].n_value, L_SET);
	read(mf, &forkstat, sizeof forkstat);
	printf("%D forks, %D %s, average=%.2f\n",
		forkstat.cntfork, forkstat.sizfork, Pages,
		(float) forkstat.sizfork / forkstat.cntfork);
	printf("%D vforks, %D %s, average=%.2f\n",
		forkstat.cntvfork, forkstat.sizvfork, Pages,
		(float)forkstat.sizvfork / forkstat.cntvfork);
}

stats(dn)
{

	if (dn >= dk_ndrive) {
		printf("  0");
		return;
	}
	printf("%3.0f", s.xfer[dn]/etime);
}

double
stat1(row)
{
	double t;
	register i;

	t = 0;
	for(i=0; i<CPUSTATES; i++)
		t += s.time[i];
	if(t == 0.)
		t = 1.;
	return(s.time[row]*100./t);
}

dointr(nintv)
	long nintv;
{
#ifdef pdp11
	printf("Device interrupt statistics are not applicable to 2.11BSD\n");
#else
	int nintr, inttotal;
	long *intrcnt;
	char *intrname, *malloc();

	nintr = (nl[X_EINTRCNT].n_value - nl[X_INTRCNT].n_value) / sizeof(long);
	intrcnt = (long *) malloc(nl[X_EINTRCNT].n_value -
		nl[X_INTRCNT].n_value);
	intrname = malloc(nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	if (intrcnt == NULL || intrname == NULL) {
		fprintf(stderr, "vmstat: out of memory\n");
		exit(9);
	}
	lseek(mf, (long)nl[X_INTRCNT].n_value, L_SET);
	read(mf, intrcnt, nintr * sizeof (long));
	lseek(mf, (long)nl[X_INTRNAMES].n_value, L_SET);
	read(mf, intrname, nl[X_EINTRNAMES].n_value - nl[X_INTRNAMES].n_value);
	printf("interrupt      total      rate\n");
	inttotal = 0;
	while (nintr--) {
		if (*intrcnt)
			printf("%-12s %8ld %8ld\n", intrname,
			    *intrcnt, *intrcnt / nintv);
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	printf("Total        %8ld %8ld\n", inttotal, inttotal / nintv);
#endif
}

#define steal(where, var) \
	lseek(mf, where, L_SET); read(mf, &var, sizeof var);
/*
 * Read the drive names out of kmem.
 */

#ifdef vax
#include <vaxuba/ubavar.h>
#include <vaxmba/mbavar.h>
#endif

read_names()
{
#ifdef pdp11
	char two_char[2];
	register int i;

	lseek(mf, (long)nl[X_DK_NAME].n_value, L_SET);
	read(mf, dk_name, dk_ndrive * sizeof (char *));
	lseek(mf, (long)nl[X_DK_UNIT].n_value, L_SET);
	read(mf, dk_unit, dk_ndrive * sizeof (int));

	for(i = 0; dk_name[i]; i++) {
		lseek(mf, (long)dk_name[i], L_SET);
		read(mf, two_char, sizeof two_char);
		sprintf(dr_name[i], "%c%c%d", two_char[0], two_char[1],
		    dk_unit[i]);
	}
#else
#ifdef vax
	struct mba_device mdev;
	register struct mba_device *mp;
	struct mba_driver mdrv;
	short two_char;
	char *cp = (char *) &two_char;
	struct uba_device udev, *up;
	struct uba_driver udrv;

	mp = (struct mba_device *) nl[X_MBDINIT].n_value;
	up = (struct uba_device *) nl[X_UBDINIT].n_value;
	if (up == 0) {
		fprintf(stderr, "vmstat: Disk init info not in namelist\n");
		exit(1);
	}
	if (mp) for (;;) {
		steal(mp++, mdev);
		if (mdev.mi_driver == 0)
			break;
		if (mdev.mi_dk < 0 || mdev.mi_alive == 0)
			continue;
		steal(mdev.mi_driver, mdrv);
		steal(mdrv.md_dname, two_char);
		sprintf(dr_name[mdev.mi_dk], "%c%c%d",
		    cp[0], cp[1], mdev.mi_unit);
	}
	for (;;) {
		steal(up++, udev);
		if (udev.ui_driver == 0)
			break;
		if (udev.ui_dk < 0 || udev.ui_alive == 0)
			continue;
		steal(udev.ui_driver, udrv);
		steal(udrv.ud_dname, two_char);
		sprintf(dr_name[udev.ui_dk], "%c%c%d",
		    cp[0], cp[1], udev.ui_unit);
	}
#endif vax
#endif /* pdp11 */
}
