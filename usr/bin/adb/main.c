#include "defs.h"

	MSG	NOEOR;
	char	*myname;	/* program name */
	int	argcount;
	int	mkfault;
	int	executing;
	int	infile;
	char	*lp;
	int	maxoff;
	int	maxpos;
	int	(*sigint)();
	int	(*sigqit)();
	int	wtflag;
	int	kernel;
	long	maxfile;
	long	maxstor;
	long	txtsiz;
	long	datsiz;
	long	datbas;
	long	stksiz;
	long	ovlsiz;
	int	overlay;
	char	*errflg;
	int	exitflg;
	int	magic;
	long	entrypt;
	char	lastc;
	int	eof;
	int	lastcom;
	long	var[36];
	char	*symfil;
	char	*corfil;
	char	*printptr;
	char	*Ipath = "/usr/share/adb";

long	round(a,b)
long		a, b;
{
	long		w;
	w = ((a+b-1)/b)*b;
	return(w);
}

/* error handling */

chkerr()
{
	IF errflg ORF mkfault
	THEN	error(errflg);
	FI
}

error(n)
	char	*n;
{
	errflg=n;
	iclose(0, 1); oclose();
	longjmp(erradb,1);
}

fault(a)
{
	signal(a,fault);
	lseek(infile,0L,2);
	mkfault++;
}

/* set up files and initial address mappings */

main(argc, argv)
	register char	**argv;
	int	argc;
{
	short	mynamelen;		/* length of program name */

	maxfile = 1L << 24;
	maxstor = 1L << 16;
	if	(isatty(0))
		myname = *argv;
	else
		myname = "adb";
	mynamelen = strlen(myname);

	gtty(0,&adbtty);
	gtty(0,&usrtty);
	WHILE argc>1
	DO	IF !strcmp("-w",argv[1])
		THEN	wtflag=2; argc--; argv++; continue;
		ELIF !strcmp("-k",argv[1])
		THEN	kernel++; argc--; argv++; continue;
		ELIF !strcmp("-I",argv[1])
		THEN	Ipath = argv[2]; argc -= 2; argv += 2; continue;
		ELSE	break;
		FI
	OD

	IF argc>1 THEN symfil = argv[1]; FI
	IF argc>2 THEN corfil = argv[2]; FI
	argcount=argc;
	setsym(); setcor();

	/* set up variables for user */
	maxoff=MAXOFF; maxpos=MAXPOS;
	var[VARB] = datbas;
	var[VARD] = datsiz;
	var[VARE] = entrypt;
	var[VARM] = magic;
	var[VARS] = stksiz;
	var[VART] = txtsiz;
	/* if text overlay, enter total overlay area size */
	IF overlay
	THEN	var[VARO] = ovlsiz;
	FI

	IF (sigint=signal(SIGINT,SIG_IGN))!=SIG_IGN
	THEN	sigint=fault; signal(SIGINT,fault);
	FI
	sigqit=signal(SIGQUIT,SIG_IGN);
	siginterrupt(SIGINT, 1);
	siginterrupt(SIGQUIT, 1);
	setjmp(erradb);
	IF executing THEN delbp(); FI
	executing=FALSE;

	LOOP	flushbuf();
		IF errflg
		THEN printf("%s\n",errflg);
		     exitflg=(int)errflg;
		     errflg=0;
		FI
		IF mkfault
		THEN	mkfault=0; printc(EOR);
			printf("%s\n", myname);
		FI
		IF myname ANDF !infile
		THEN	write(1,myname,mynamelen);
			write(1,"> ",2);
		FI
		lp=0; rdc(); lp--;
		IF eof
		THEN	IF infile
			THEN	iclose(-1, 0); eof=0; longjmp(erradb,1);
			ELSE	done();
			FI
		ELSE	exitflg=0;
		FI
		command(0,lastcom);
		IF lp ANDF lastc!=EOR THEN error(NOEOR); FI
	POOL
}

done()
{
	endpcs();
	exit(exitflg);
}

