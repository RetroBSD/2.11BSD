#include "defs.h"
#include <ctype.h>

	MSG	BADSYM;
	MSG	BADVAR;
	MSG	BADKET;
	MSG	BADSYN;
	MSG	NOCFN;
	MSG	NOADR;
	MSG	BADLOC;

extern	struct	SYMbol	*symbol, *cache_by_string();
	int	lastframe;
	int	kernel;
	int	savlastf;
	long	savframe;
	char	svlastov;
	int	savpc;
	int	callpc;
	char	*lp;
	int	octal;
	char	*errflg;
	long	localval;
	static	char	isymbol[MAXSYMLEN + 2];
	char	lastc;
	u_int	*uar0;
	u_int	corhdr[];
	char	curov, startov, lastsymov;
	int	overlay;
	long	dot;
	long	ditto;
	int	dotinc;
	long	var[];
	long	expv;

expr(a)
{	/* term | term dyadic expr |  */
	int		rc;
	long		lhs;

	lastsymov = 0;
	rdc(); lp--; rc=term(a);

	WHILE rc
	DO  lhs = expv;

	    switch (readchar()) {

		    case '+':
			term(a|1); expv += lhs; break;

		    case '-':
			term(a|1); expv = lhs - expv; break;

		    case '#':
			term(a|1); expv = round(lhs,expv); break;

		    case '*':
			term(a|1); expv *= lhs; break;

		    case '%':
			term(a|1); expv = lhs/expv; break;

		    case '&':
			term(a|1); expv &= lhs; break;

		    case '|':
			term(a|1); expv |= lhs; break;

		    case ')':
			IF (a&2)==0 THEN error(BADKET); FI

		    default:
			lp--;
			return(rc);
	    }
	OD
	return(rc);
}

term(a)
{	/* item | monadic item | (expr) | */

	switch (readchar()) {

		    case '*':
			term(a|1); expv=chkget(expv,DSP); return(1);

		    case '@':
			term(a|1); expv=chkget(expv,ISP); return(1);

		    case '-':
			term(a|1); expv = -expv; return(1);

		    case '~':
			term(a|1); expv = ~expv; return(1);

		    case '(':
			expr(2);
			IF *lp!=')'
			THEN	error(BADSYN);
			ELSE	lp++; return(1);
			FI

		    default:
			lp--;
			return(item(a));
	}
}

item(a)
{	/* name [ . local ] | number | . | ^ | <var | <register | 'x | | */
	int		base, d, frpt, regptr;
	char		savc;
	char		hex;
	long		frame;
	union {float r; long i;} real;
	register struct SYMbol	*symp;
	char		savov;

	hex=FALSE;

	readchar();
	IF symchar(0)
	THEN	readsym();
		IF lastc=='.'
		THEN	frame=(kernel?corhdr[KR5]:uar0[R5])&EVEN;
			lastframe=0; callpc=kernel?(-2):uar0[PC];
			if (overlay){
				savov = curov;
				setovmap(startov);
			}
			WHILE errflg==0
			DO  savpc=callpc;
			    findroutine(frame);
			    if  (eqsym(cache_sym(symbol), isymbol,'~'))
			    	break;
			    lastframe=frame;
			    frame=get(frame,DSP)&EVEN;
			    IF frame==0
			    THEN error(NOCFN);
			    FI
			OD
			savlastf=lastframe; savframe=frame;
			svlastov = curov;
			readchar();
			IF symchar(0)
			THEN	chkloc(expv=frame);
			FI
			if (overlay)
				setovmap(savov);
		ELIF (symp=lookupsym(isymbol))==0 THEN error(BADSYM);
		ELSE expv = symp->value; lastsymov=symp->ovno;
		FI
		lp--;


	ELIF isdigit(lastc) ORF (hex=TRUE, lastc=='#' ANDF isxdigit(readchar()))
	THEN	expv = 0;
		base = (lastc == '0' ORF octal ? 8 : (hex ? 16 : 10));
		WHILE (hex ? isxdigit(lastc) : isdigit(lastc))
		DO  expv *= base;
		    IF (d=convdig(lastc))>=base THEN error(BADSYN); FI
		    expv += d; readchar();
		    IF expv==0 ANDF (lastc=='x' ORF lastc=='X')
		    THEN hex=TRUE; base=16; readchar();
		    FI
		OD
		IF lastc=='.' ANDF (base==10 ORF expv==0) ANDF !hex
		THEN	real.r=expv; frpt=0; base=10;
			WHILE isdigit(readchar())
			DO	real.r *= base; frpt++;
				real.r += lastc-'0';
			OD
			WHILE frpt--
			DO	real.r /= base; OD
			expv = real.i;
		FI
		lp--;

	ELIF lastc=='.'
	THEN	readchar();
		IF symchar(0)
		THEN	savov = curov;
			curov = svlastov;
			lastframe=savlastf; callpc=savpc; findroutine(savframe);
			chkloc(savframe);
			if(overlay)
				setovmap(savov);
		ELSE	expv=dot;
		FI
		lp--;

	ELIF lastc=='"'
	THEN	expv=ditto;

	ELIF lastc=='+'
	THEN	expv=inkdot(dotinc);

	ELIF lastc=='^'
	THEN	expv=inkdot(-dotinc);

	ELIF lastc=='<'
	THEN	savc=rdc();
		IF (regptr=getreg(savc)) != NOREG
		THEN	expv=uar0[regptr];
		ELIF (base=varchk(savc)) != -1
		THEN	expv=var[base];
		ELSE	error(BADVAR);
		FI

	ELIF lastc=='\''
	THEN	d=4; expv=0;
		WHILE quotchar()
		DO  IF d--
		    THEN IF d==1 THEN expv <<=16; FI
			 expv |= ((d&1)?lastc:lastc<<8);
		    ELSE error(BADSYN);
		    FI
		OD

	ELIF a
	THEN	error(NOADR);
	ELSE	lp--; return(0);
	FI
	return(1);
}

/* service routines for expression reading */

readsym()
{
	register char	*p;

	p = isymbol;
	REP IF p < &isymbol[MAXSYMLEN]
	    THEN *p++ = lastc;
	    FI
	    readchar();
	PER symchar(1) DONE
	*p++ = 0;
}

struct SYMbol *
lookupsym(symstr)
	char	*symstr;
{
	register struct SYMbol *symp, *sc;

	symset();
	while	(symp = symget())
		{
	   	if	(overlay && (symp->type == ISYM))
			{
			if	(sc = cache_by_string(symstr, 1))
				return(sc);
			if	(eqsym(no_cache_sym(symp), symstr,'~'))
				break;
			}
		else
			{
			if	(sc = cache_by_string(symstr, 0))
				return(sc);
			if	(eqsym(no_cache_sym(symp), symstr,'_'))
				break;
			}
		}
/*
 * We did not enter anything into the cache (no sense inserting hundreds
 * of symbols which didn't match) while examining the (entire) symbol table.
 * Now that we have a match put it into the cache (doing a lookup on it is
 * the easiest way).
*/
	if	(symp)
		(void)cache_sym(symp);
	return(symp);
}

convdig(c)
char c;
{
	IF isdigit(c)
	THEN	return(c-'0');
	ELIF isxdigit(c)
	THEN	return(c-'a'+10);
	ELSE	return(17);
	FI
}

symchar(dig)
{
	IF lastc=='\\' THEN readchar(); return(TRUE); FI
	return( isalpha(lastc) ORF lastc=='_' ORF dig ANDF isdigit(lastc) );
}

varchk(name)
{
	IF isdigit(name) THEN return(name-'0'); FI
	IF isalpha(name) THEN return((name&037)-1+10); FI
	return(-1);
}

chkloc(frame)
long		frame;
{
	readsym();
	REP IF localsym(frame)==0 THEN error(BADLOC); FI
	    expv=localval;
	PER !eqsym(cache_sym(symbol), isymbol,'~') DONE
}

eqsym(s1, s2, c)
register char *s1, *s2;
	char	c;
	{

	if	(!strcmp(s1, s2))
		return(TRUE);
	else if (*s1++ == c)
		return(!strcmp(s1, s2));
	return(FALSE);
	}
