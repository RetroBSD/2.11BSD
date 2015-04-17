/*
 * 1995/06/09.  sprintf() returns an int (since patch #233) rather
 * 	   a "char *".  
*/

#include "defs"
#include "string_defs"
#if FAMILY == DMR
#	include "dmrdefs"
#endif
#if FAMILY==SCJ && OUTPUT==BINARY
#	include "scjdefs"
#endif

/*
      PDP 11-SPECIFIC PRINTING ROUTINES
*/

int maxregvar = 0;
static char textline[50];
int regnum[] = { 3, 2 };


prsave()
{
}



goret(type)
int type;
{
#if  FAMILY == DMR
	p2op(P2RETURN);
#endif
#if FAMILY==SCJ
	sprintf(textline, "\tjmp\tcret");
	p2pass(textline);
#endif
}




/*
 * move argument slot arg1 (relative to ap)
 * to slot arg2 (relative to ARGREG)
 */

mvarg(type, arg1, arg2)
int type, arg1, arg2;
{
mvarg1(arg1+4, arg2);
if(type == TYLONG)
	mvarg1(arg1+6, arg2+2);
}




mvarg1(m, n)
int m, n;
{
#if FAMILY == DMR
	p2reg(ARGREG, P2SHORT|P2PTR);
	p2op2(P2ICON, P2SHORT);
	p2i(n);
	p2op2(P2PLUS, P2SHORT|P2PTR);
	p2op2(P2INDIRECT, P2SHORT);
	p2reg(AUTOREG, P2SHORT|P2PTR);
	p2op2(P2ICON, P2SHORT);
	p2i(m);
	p2op2(P2PLUS, P2SHORT|P2PTR);
	p2op2(P2INDIRECT, P2SHORT);
	p2op2(P2ASSIGN, P2SHORT);
	putstmt();
#endif
#if FAMILY == SCJ
	sprintf(textline, "\tmov\t%d.(r5),%d.(r4)", m, n);
	p2pass(textline);
#endif
}




prlabel(fp, k)
FILEP fp;
int k;
{
fprintf(fp, "L%d:\n", k);
}



prconi(fp, type, n)
FILEP fp;
int type;
ftnint n;
{
register int *np;
np = &n;
if(type == TYLONG)
	fprintf(fp, "\t%d.;%d.\n", np[0], np[1]);
else
	fprintf(fp, "\t%d.\n", np[1]);
}



prcona(fp, a)
FILEP fp;
ftnint a;
{
fprintf(fp, "L%ld\n", a);
}



#if HERE!=PDP11
BAD NEWS
#endif

#if HERE==PDP11
prconr(fp, type, x)
FILEP fp;
int type;
double x;
{
register int k, *n;
n = &x;	/* nonportable cheat */
k = (type==TYREAL ? 2 : 4);
fprintf(fp, "\t");
while(--k >= 0)
	fprintf(fp, "%d.%c", *n++, (k==0 ? '\n' : ';') );
}
#endif




preven(k)
int k;
{
if(k > 1)
	fprintf(asmfile, "\t.even\n", k);
}



#if FAMILY == SCJ

prcmgoto(p, nlab, skiplabel, labarray)
expptr p;
int nlab, skiplabel, labarray;
{
int regno;

putforce(p->vtype, p);

if(p->vtype == TYLONG)
	{
	regno = 1;
	sprintf(textline, "\ttst\tr0");
	p2pass(textline);
	sprintf(textline, "\tbne\tL%d", skiplabel);
	p2pass(textline);
	}
else
	regno = 0;

sprintf(textline, "\tcmp\tr%d,$%d.", regno, nlab);
p2pass(textline);
sprintf(textline, "\tbhi\tL%d", skiplabel);
p2pass(textline);
sprintf(textline, "\tasl\tr%d", regno);
p2pass(textline);
sprintf(textline, "\tjmp\t*L%d(r%d)", labarray, regno);
p2pass(textline);
}


prarif(p, neg,zer,pos)
expptr p;
int neg, zer, pos;
{
register int ptype;

putforce( ptype = p->vtype, p);
if( ISINT(ptype) )
	{
	sprintf(textline, "\ttst\tr0");
	p2pass(textline);
	sprintf(textline, "\tjlt\tL%d", neg);
	p2pass(textline);
	sprintf(textline, "\tjgt\tL%d", pos);
	p2pass(textline);
	if(ptype != TYSHORT)
		{
		sprintf(textline, "\ttst\tr1");
		p2pass(textline);
		sprintf(textline, "\tjeq\tL%d", zer);
		p2pass(textline);
		}
	sprintf(textline, "\tjbr\tL%d", pos);
	p2pass(textline);
	}
else
	{
	sprintf(textline, "\ttstf\tr0");
	p2pass(textline);
	sprintf(textline, "\tcfcc");
	p2pass(textline);
	sprintf(textline, "\tjeq\tL%d", zer);
	p2pass(textline);
	sprintf(textline, "\tjlt\tL%d", neg);
	p2pass(textline);
	sprintf(textline, "\tjmp\tL%d", pos);
	p2pass(textline);
	}
}

#endif




char *memname(stg, mem)
int stg, mem;
{
static char s[20];

switch(stg)
	{
	case STGCOMMON:
	case STGEXT:
		sprintf(s, "_%s", varstr(XL, extsymtab[mem].extname) );
		break;

	case STGBSS:
	case STGINIT:
		sprintf(s, "v.%d", mem);
		break;

	case STGCONST:
		sprintf(s, "L%d", mem);
		break;

	case STGEQUIV:
		sprintf(s, "q.%d", mem);
		break;

	default:
		error("memname: invalid vstg %d", stg, 0, FATAL1);
	}
return(s);
}


prlocvar(s, len)
char *s;
ftnint len;
{
fprintf(asmfile, "%s:", s);
prskip(asmfile, len);
}



prext(name, leng, init)
char *name;
ftnint leng;
int init;
{
if(leng==0 || init)
	fprintf(asmfile, "\t.globl\t_%s\n", name);
else
	fprintf(asmfile, "\t.comm\t_%s,%ld.\n", name, leng);
}



prendproc()
{
}



prtail()
{
#if FAMILY == SCJ
	sprintf(textline, "\t.globl\tcsv,cret");
	p2pass(textline);
#else
	p2op(P2EOF);
#endif
}



prolog(ep, argvec)
struct entrypoint *ep;
struct addrblock *argvec;
{
int i, argslot, proflab;
register chainp p;
register struct nameblock *q;
register struct dimblock *dp;
char *funcname;
struct constblock *mkaddcon();

if(procclass == CLMAIN) {
	funcname = "MAIN_";
	prentry(funcname);
}

if(ep->entryname) {
	funcname = varstr(XL, ep->entryname->extname);
	prentry(funcname);
}

if(procclass == CLBLOCK)
	return;
if(profileflag)
	proflab = newlabel();
#if FAMILY == SCJ
	sprintf(textline, "\tjsr\tr5,csv");
	p2pass(textline);
	if(profileflag)
		{
		fprintf(asmfile, ".data\nL%d:\t_%s+1\n.bss\n", proflab, funcname);
		sprintf(textline, "\tmov\t$L%d,r0", proflab);
		p2pass(textline);
		sprintf(textline, "\tjsr\tpc,mcount");
		p2pass(textline);
		}
	sprintf(textline, "\tsub\t$.F%d,sp", procno);
	p2pass(textline);
#else
	p2op(P2SAVE);
	if(profileflag) {
		p2op2(P2PROFIL, proflab);
		p2str(funcname);
	}
	p2op2(P2SETSTK, ( (((int) autoleng)+1) & ~01) );
#endif

if(argvec == NULL)
	addreg(argloc = 4);
else
	{
	addreg( argloc = argvec->memoffset->const.ci );
	if(proctype == TYCHAR)
		{
		mvarg(TYADDR, 0, chslot);
		mvarg(TYLENG, SZADDR, chlgslot);
		argslot = SZADDR + SZLENG;
		}
	else if( ISCOMPLEX(proctype) )
		{
		mvarg(TYADDR, 0, cxslot);
		argslot = SZADDR;
		}
	else
		argslot = 0;

	for(p = ep->arglist ; p ; p =p->nextp)
		{
		q = p->datap;
		mvarg(TYADDR, argslot, q->vardesc.varno);
		argslot += SZADDR;
		}
	for(p = ep->arglist ; p ; p = p->nextp)
		{
		q = p->datap;
		if(q->vtype==TYCHAR || q->vclass==CLPROC)
			{
			if( q->vleng && ! ISCONST(q->vleng) )
				mvarg(TYLENG, argslot, q->vleng->memno);
			argslot += SZLENG;
			}
		}
	}

for(p = ep->arglist ; p ; p = p->nextp)
	if(dp = ( (struct nameblock *) (p->datap) ) ->vdim)
		{
		for(i = 0 ; i < dp->ndim ; ++i)
			if(dp->dims[i].dimexpr)
				puteq( fixtype(cpexpr(dp->dims[i].dimsize)),
					fixtype(cpexpr(dp->dims[i].dimexpr)));
		if(dp->basexpr)
			puteq( 	cpexpr(fixtype(dp->baseoffset)),
				cpexpr(fixtype(dp->basexpr)));
		}

if(typeaddr)
	puteq( cpexpr(typeaddr), mkaddcon(ep->typelabel) );
putgoto(ep->entrylabel);
}



prentry(s)
char *s;
{
#if FAMILY == SCJ
	sprintf(textline, "_%s:", s);
	p2pass(textline);
#else
	p2op(P2RLABEL);
	putc('_', textfile);
	p2str(s);
#endif
}




addreg(k)
int k;
{
#if FAMILY == SCJ
	sprintf(textline, "\tmov\tr5,r4");
	p2pass(textline);
	sprintf(textline, "\tadd\t$%d.,r4", k);
	p2pass(textline);
#else
	p2reg(ARGREG, P2SHORT);
	p2reg(AUTOREG, P2SHORT);
	p2op2(P2ICON, P2SHORT);
	p2i(k);
	p2op2(P2PLUS, P2SHORT);
	p2op2(P2ASSIGN, P2SHORT);
	putstmt();
#endif
}





prhead(fp)
FILEP fp;
{
#if FAMILY==SCJ
#	if OUTPUT == BINARY
		p2triple(P2LBRACKET, ARGREG-1-highregvar, procno);
		p2word( (long) (BITSPERCHAR*autoleng) );
		p2flush();
#	else
		fprintf(fp, "[%02d\t%06ld\t%02d\t\n", procno,
			BITSPERCHAR*autoleng, ARGREG-1-highregvar);
#	endif
#endif
}

prdbginfo()
{
register char *s;
char *t, buff[50];
register struct nameblock *p;
struct hashentry *hp;

if(s = entries->entryname->extname)
	s = varstr(XL, s);
else if(procclass == CLMAIN)
	s = "MAIN_";
else
	return;

/*
 *!Remove wierd symbol definition so f77 code may be overlayed. wfj 11/80
if(procclass != CLBLOCK)
	fprintf(asmfile, "~~%s = _%s\n", s, s);
 * This code equiv's a tilde name to the external name, which is normally
 * used by ld(1) to specify an intra-overlay address. I have no idea what
 * this was useful for. wfj--
 */

for(hp = hashtab ; hp<lasthash ; ++hp)
    if(p = hp->varp)
	{
	buff[0] = '\0';
	if(p->vstg == STGARG)
		sprintf(buff, "%o", p->vardesc.varno+argloc);
	else if(p->vclass == CLVAR)
		switch(p->vstg)
			{
			case STGBSS:
			case STGINIT:
			case STGEQUIV:
				t = memname(p->vstg, p->vardesc.varno);
				if(p->voffset)
					sprintf(buff, "%s+%o", t, p->voffset);
				else
					sprintf(buff, "%s", t);
				break;

			case STGAUTO:
				sprintf(buff, "%o", p->voffset);
				break;

			default:
				break;
			}
	if (buff[0])
		fprintf(asmfile, "~%s = %s\n", varstr(VL,p->varname), buff);
	}
fprintf(asmfile, "~~:\n");
}
