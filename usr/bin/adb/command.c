#include "defs.h"

	MSG	BADEQ;
	MSG	NOMATCH;
	MSG	BADVAR;
	MSG	BADCOM;
	MAP	txtmap;
	MAP	datmap;

	char	symov, lastsymov, curov;
	int	executing;
	char	*lp;
	int	fcor;
	int	fsym;
	int	mkfault;
	char	*errflg;
	char	lastc;
	char	eqformat[512] = "o";
	char	stformat[512] = "o\"= \"^i";
	u_int	corhdr[], *uar0;
	long	dot;
	long	ditto;
	int	dotinc;
	int	lastcom = '=';
	long	var[];
	long	locval;
	long	locmsk;
	int	pid;
	long	expv;
	long	adrval;
	int	adrflg;
	long	cntval;
	int	cntflg;
extern	char	*myname;

/* command decoding */

command(buf,defcom)
	char	*buf;
	char	defcom;
{
	int	itype, ptype, modifier, regptr;
	char	longpr, eqcom;
	char	wformat[1];
	char	savc;
	long	w, savdot;
	char	*savlp=lp;

	IF buf
	THEN IF *buf==EOR
	     THEN return(FALSE);
	     ELSE lp=buf;
	     FI
	FI

	REP
	IF adrflg=expr(0)
	THEN dot=expv; ditto=dot; symov=lastsymov;
	FI
	adrval=dot;
	IF rdc()==',' ANDF expr(0)
	THEN cntflg=TRUE; cntval=expv;
	ELSE cntflg=FALSE; cntval=1; lp--;
	FI

	IF !eol(rdc())
	THEN lastcom=lastc;
	ELSE IF adrflg==0 THEN dot=inkdot(dotinc); FI
	     lp--; lastcom=defcom;
	FI

	switch(lastcom&STRIP) {

	    case '/':
		itype=DSP; ptype=DSYM;
		goto trystar;

	    case '=':
		itype=NSP; ptype=ASYM;
		goto trypr;

	    case '?':
		itype=ISP; ptype=ISYM;
		goto trystar;

	    trystar:
		IF rdc()=='*' THEN lastcom |= QUOTE; ELSE lp--; FI
		IF lastcom&QUOTE
		THEN itype |= STAR; ptype = (DSYM+ISYM)-ptype;
		FI

	    trypr:
		longpr=FALSE; eqcom=lastcom=='=';
		switch (rdc()) {

			case 'm':
			    {/*reset map data*/
			    int		fcount;
			    MAPPTR	smap;
			    long	*mp;

			    IF eqcom THEN error(BADEQ); FI
			    smap=(itype&DSP?&datmap:&txtmap);
			    fcount=3;
			    IF itype&STAR
			    THEN mp = &(smap->b2);
			    ELSE mp = &(smap->b1);
			    FI
			    WHILE fcount-- ANDF expr(0)
			    DO *(mp)++ = expv; OD
			    IF rdc()=='?' THEN smap->ufd=fsym;
			    ELIF lastc == '/' THEN smap->ufd=fcor;
			    ELSE lp--;
			    FI
			    }
			    break;

			case 'L':
			    longpr=TRUE;
			case 'l':
			    /*search for exp*/
			    IF eqcom THEN error(BADEQ); FI
			    dotinc=2; savdot=dot;
			    expr(1); locval=expv;
			    IF expr(0) THEN locmsk=expv; ELSE locmsk = -1L; FI
			    LOOP w=leng(get(dot,itype));
				 IF longpr
				 THEN w=itol(w,get(inkdot(2),itype));
				 FI
				 IF errflg ORF mkfault ORF (w&locmsk)==locval THEN break; FI
				 dot=inkdot(dotinc);
			    POOL
			    IF errflg
			    THEN dot=savdot; errflg=NOMATCH;
			    FI
			    psymoff(dot,ptype,"");
			    break;

			case 'W':
			    longpr=TRUE;
			case 'w':
			    IF eqcom THEN error(BADEQ); FI
			    wformat[0]=lastc; expr(1);
			    REP  savdot=dot; psymoff(dot,ptype,":%16t"); exform(1,wformat,itype,ptype);
				 errflg=0; dot=savdot;
				 IF longpr
				 THEN put(dot,itype,expv);
				 FI
				 put((longpr?inkdot(2):dot),itype,shorten(expv));
				 savdot=dot;
				 printf("=%8t"); exform(1,wformat,itype,ptype);
				 printc(EOR);
			    PER  expr(0) ANDF errflg==0 DONE
			    dot=savdot;
			    chkerr();
			    break;

			default:
			    lp--;
			    getformat(eqcom ? eqformat : stformat);
			    IF !eqcom
			    THEN IF symov ANDF symov!=curov ANDF
				ptype == ISYM ANDF dot>=txtmap.bo
				 THEN setovmap(symov);
				      var[VARC]=symov;
				 FI
				 IF *stformat!='a'
				 THEN psymoff(dot,ptype,":%16t");
				 FI
			    FI
			    scanform(cntval,(eqcom?eqformat:stformat),itype,ptype);
		}
		break;

	    case '>':
		lastcom=0; savc=rdc();
		IF (regptr=getreg(savc)) != NOREG
		THEN uar0[regptr]=shorten(dot);
		     ptrace(PT_WRITE_U,pid,(int)&uar0[regptr]-(int)&corhdr,
			uar0[regptr]);
		     IF (uar0+regptr) == &(((U*)corhdr)->u_ovdata.uo_curov)
		     THEN var[VARC]=dot; setovmap((char)dot); FI
		ELIF (modifier=varchk(savc)) != -1
		THEN	var[modifier]=dot;
			IF modifier == VARC THEN setovmap((char)dot); FI
		ELSE	error(BADVAR);
		FI
		break;

	    case '!':
		lastcom=0;
		unox(); break;

	    case '$':
		lastcom=0;
		printtrace(nextchar()); break;

	    case ':':
		IF !executing
		THEN executing=TRUE;
		     subpcs(nextchar());
		     executing=FALSE;
		     lastcom=0;
		FI
		break;

	    case 0:
		printf("%s\n", myname);
		break;

	    default: error(BADCOM);
	}

	flushbuf();
	PER rdc()==';' DONE
	IF buf THEN lp=savlp; ELSE lp--; FI
	return(adrflg ANDF dot!=0);
}

