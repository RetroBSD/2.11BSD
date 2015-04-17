#include "defs.h"
#include <sys/file.h>

	MSG	NOFORK;
	MSG	ENDPCS;
	MSG     BADWAIT;
	char    *lp;
	int	sigint;
	int	sigqit;
	BKPTR   bkpthead;
	REGLIST reglist[];
	char	lastc;
	u_int	corhdr[];
	u_int	*uar0;
	int     overlay;
	char	curov;
	int	fcor;
	int	fsym;
	char	*errflg;
	int	signo;
	long	dot;
	char	*symfil;
	int	wtflag;
	int	pid;
	long	expv;
	int	adrflg;
	long	loopcnt;
	long	var[];
	int	userpc=1;

/* service routines for sub process control */

getsig(sig)
	{
	return(expr(0) ? shorten(expv) : sig);
	}

runpcs(runmode, execsig)
{
	int	rc;
	register BKPTR       bkpt;

	IF adrflg
	THEN userpc=shorten(dot);
	FI
	if (overlay)
		choverlay(((U*)corhdr)->u_ovdata.uo_curov);
	printf("%s: running\n", symfil);

	WHILE (loopcnt--)>0
	DO
#ifdef DEBUG
		printf("\ncontinue %d %d\n",userpc,execsig);
#endif
		stty(0,&usrtty);
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); readregs();

		/*look for bkpt*/
		IF signo==0 ANDF (bkpt=scanbkpt(uar0[PC]-2))
		THEN /*stopped at bkpt*/
		     userpc=uar0[PC]=bkpt->loc;
		     IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC, command(bkpt->comm,':')) ANDF --bkpt->count)
		     THEN execbkpt(bkpt); execsig=0; loopcnt++;
			  userpc=1;
		     ELSE bkpt->count=bkpt->initcnt;
			  rc=1;
		     FI
		ELSE rc=0; execsig=signo; userpc=1;
		FI
	OD
	return(rc);
}

endpcs()
{
	register BKPTR       bkptr;

	IF pid
	THEN ptrace(PT_KILL,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
}

setup()
{

	close(fsym); fsym = -1;
	IF (pid = fork()) == 0
	THEN ptrace(PT_TRACE_ME,0,0,0);
	     signal(SIGINT,sigint); signal(SIGQUIT,sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN printf("%s: cannot execute\n",symfil);
		  endpcs(); error((char *)0);
	     FI
	FI
}

execbkpt(bkptr)
BKPTR   bkptr;
{
	int	bkptloc;
#ifdef DEBUG
	printf("exbkpt: %d\n",bkptr->count);
#endif
	bkptloc = bkptr->loc;
	ptrace(PT_WRITE_I,pid,bkptloc,bkptr->ins);
	stty(0,&usrtty);
	ptrace(PT_STEP,pid,bkptloc,0);
	bpwait(); chkerr();
	ptrace(PT_WRITE_I,pid,bkptloc,BPT);
	bkptr->flag=BKPTSET;
}

doexec()
{
	char	*argl[MAXARG];
	char	args[LINSIZ];
	char	*p, **ap, *filnam;

	ap=argl; p=args;
	*ap++=symfil;
	REP     IF rdc()==EOR THEN break; FI
		/*
		 * If we find an argument beginning with a `<' or a `>', open
		 * the following file name for input and output, respectively
		 * and back the argument collocation pointer, p, back up.
		 */
		*ap = p;
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB DO *p++=lastc; readchar(); OD
		*p++=0; filnam = *ap+1;
		IF **ap=='<'
		THEN    close(0);
			IF open(filnam,0)<0
			THEN    printf("%s: cannot open\n",filnam); exit(0);
			FI
			p = *ap;
		ELIF **ap=='>'
		THEN    close(1);
			IF open(filnam, O_CREAT|O_WRONLY, 0666)<0
			THEN    printf("%s: cannot create\n",filnam); exit(0);
			FI
			p = *ap;
		ELSE    ap++;
		FI
	PER lastc!=EOR DONE
	*ap++=0;
	execv(symfil, argl);
}

BKPTR   scanbkpt(adr)
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr ANDF 
	  (bkptr->ovly == 0 || bkptr->ovly==curov)
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN del1bp(bkptr);
	   FI
	OD
}

del1bp(bkptr)
BKPTR bkptr;
{
	if (bkptr->ovly)
		choverlay(bkptr->ovly);
	ptrace(PT_WRITE_I,pid,bkptr->loc,bkptr->ins);
}

/* change overlay in subprocess */
choverlay(ovno)
	char	ovno;
{
	errno = 0;
	if (overlay && pid && ovno>0 && ovno<=NOVL)
		ptrace(PT_WRITE_U,pid,&(((U*)0)->u_ovdata.uo_curov),ovno);
	IF errno
	THEN printf("cannot change to overlay %d\n", ovno);
	FI
}

setbp()
{
	register BKPTR       bkptr;

	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag
	   THEN set1bp(bkptr);
	   FI
	OD
}
set1bp(bkptr)
BKPTR bkptr;
{
	register int         a;

	a = bkptr->loc;
	if (bkptr->ovly)
		choverlay(bkptr->ovly);
	bkptr->ins = ptrace(PT_READ_I, pid, a, 0);
	ptrace(PT_WRITE_I, pid, a, BPT);
	IF errno
	THEN printf("cannot set breakpoint: ");
	     psymoff(leng(bkptr->loc),ISYM,"\n");
	FI
}

bpwait()
{
	register int w;
	int stat;

	signal(SIGINT, SIG_IGN);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT,sigint);
	gtty(0,&usrtty);
	stty(0,&adbtty);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN printf(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	ELSE signo = stat>>8;
	     IF signo!=SIGTRAP
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

readregs()
{
	/*get REG values from pcs*/
	register u_int i, *afp;
	char	ovno;

	FOR i=0; i<NREG; i++
	DO uar0[reglist[i].roffs] =
		    ptrace(PT_READ_U, pid,
 		        (int *)((int)&uar0[reglist[i].roffs] - (int)&corhdr),
 			0);
	OD
	/* if overlaid, get ov */
	IF overlay
	THEN
		ovno = ptrace(PT_READ_U, pid,
		    &(((struct user *)0)->u_ovdata.uo_curov),0);
		var[VARC] = ovno;
		((U *)corhdr)->u_ovdata.uo_curov = ovno;
		setovmap(ovno);
	FI
/*
 * The following section was totally and completely broken.  It had been that
 * way since V7.  If you've ever wondered why the FP regs couldn't be displayed
 * for a traced program that was the reason.  The reading of the FP regs was
 * redone 1998/4/21 for 2.11BSD.
*/
	i = offsetof(struct user, u_fps);
	afp = (u_int *)&((U *)corhdr)->u_fps;
	while	(i < offsetof(struct user, u_fps) + sizeof (struct fps))
		{
		*afp++ = ptrace(PT_READ_U, pid, i, 0);
		i += sizeof (u_int);
		}
}
