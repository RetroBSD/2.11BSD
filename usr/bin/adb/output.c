#include "defs.h"

	int	mkfault;
	int	infile;
	int	outfile = 1;
	int	maxpos;
	char	printbuf[MAXLIN];
	char	*printptr = printbuf;
	char	*digitptr;
	MSG	TOODEEP;
	long	var[];

printc(c)
	char	c;
{
	char	d;
	char	*q;
	int	posn, tabs, p;

	IF mkfault
	THEN	return;
	ELIF (*printptr=c)==EOR
	THEN tabs=0; posn=0; q=printbuf;
	     FOR p=0; p<printptr-printbuf; p++
	     DO d=printbuf[p];
		IF (p&7)==0 ANDF posn
		THEN tabs++; posn=0;
		FI
		IF d==SP
		THEN posn++;
		ELSE WHILE tabs>0 DO *q++=TB; tabs--; OD
		     WHILE posn>0 DO *q++=SP; posn--; OD
		     *q++=d;
		FI
	     OD
	     *q++=EOR;
	     write(outfile,printbuf,q-printbuf);
	     printptr=printbuf;
	ELIF c==TB
	THEN *printptr++=SP;
	     WHILE (printptr-printbuf)&7 DO *printptr++=SP; OD
	ELIF c
	THEN printptr++;
	FI
	IF printptr >= &printbuf[MAXLIN-9] THEN
		write(outfile, printbuf, printptr-printbuf);
		printptr = printbuf;
	FI
}

flushbuf()
{	IF printptr!=printbuf
	THEN printc(EOR);
	FI
}

/*VARARGS*/
printf(fmat,a1)
	char	*fmat;
	char	**a1;
{
	char	*fptr, *s;
	int	*vptr;
	long	*dptr;
	double	*rptr;
	int	width, prec;
	char	c, adj;
	int	x, decpt, n;
	long	lx;
	char	digits[64];
	char	*ecvt();

	fptr = fmat; vptr = (int *)&a1;

	WHILE c = *fptr++
	DO  IF c!='%'
	    THEN printc(c);
	    ELSE IF *fptr=='-' THEN adj='l'; fptr++; ELSE adj='r'; FI
		 width=convert(&fptr);
		 if	(*fptr == '*')
			{
			width = *vptr++;
			fptr++;
			}
		 if	(*fptr == '.')
			{
			fptr++;
			prec = convert(&fptr);
			if	(*fptr == '*')
				{
				prec = *vptr++;
				fptr++;
				}
			}
		else
			prec = -1;
		 digitptr=digits;
		 dptr=(long *)(rptr=(double *)vptr); lx = *dptr; x = *vptr++;
		 s=0;
		 switch (c = *fptr++) {
		    case 'd':
		    case 'u':
			printnum(x,c,10); break;
		    case 'o':
			printoct(0,x,0); break;
		    case 'q':
			lx=x; printoct(lx,-1); break;
		    case 'x':
			printdbl(0,x,c,16); break;
		    case 'Y':
			printdate(lx); vptr++; break;
		    case 'D':
		    case 'U':
			printdbl(lx,c,10); vptr++; break;
		    case 'O':
			printoct(lx,0); vptr++; break;
		    case 'Q':
			printoct(lx,-1); vptr++; break;
		    case 'X':
			printdbl(lx,'x',16); vptr++; break;
		    case 'c':
			printc(x); break;
		    case 's':
			s=(char *)x; break;
		    case 'f':
		    case 'F':
			vptr += 7;
			s=ecvt(*rptr, prec, &decpt, &n);
			*digitptr++=(n?'-':'+');
			*digitptr++ = (decpt<=0 ? '0' : *s++);
			IF decpt>0 THEN decpt--; FI
			*digitptr++ = '.';
			WHILE *s ANDF prec-- DO *digitptr++ = *s++; OD
			WHILE *--digitptr=='0' DONE
			digitptr += (digitptr-digits>=3 ? 1 : 2);
			IF decpt
			THEN *digitptr++ = 'e'; printnum(decpt,'d',10);
			FI
			s=0; prec = -1; break;
		    case 'm':
			vptr--; break;
		    case 'M':
			width=x; break;
		    case 'T':
		    case 't':
			IF c=='T'
			THEN width=x;
			ELSE vptr--;
			FI
			IF width
			THEN width -= ((printptr - printbuf) % width);
			FI
			break;
		    default:
			printc(c); vptr--;
		}

		IF s==0
		THEN *digitptr=0; s=digits;
		FI
		n = strlen(s);
		n=(prec<n ANDF prec>=0 ? prec : n);
		width -= n;
		IF adj=='r'
		THEN WHILE width-- > 0
		     DO printc(SP); OD
		FI
		WHILE n-- DO printc(*s++); OD
		WHILE width-- > 0 DO printc(SP); OD
		digitptr=digits;
	    FI
	OD
}

printdate(tvec)
	long	tvec;
{
	register int i;
	register char	*timeptr;
	extern	char	*ctime();

	timeptr = ctime(&tvec);
	FOR i=20; i<24; i++ DO *digitptr++ = *(timeptr+i); OD
	FOR i=3; i<19; i++ DO *digitptr++ = *(timeptr+i); OD
}

convert(cp)
	register char **cp;
{
	register char c;
	int	n = 0;

	WHILE ((c = *(*cp)++)>='0') ANDF (c<='9') DO n=n*10+c-'0'; OD
	(*cp)--;
	return(n);
}

printnum(n,fmat,base)
	register int n;
{
	register char k;
	register int *dptr;
	int digs[15];

	dptr=digs;
	IF n<0 ANDF fmat=='d' THEN n = -n; *digitptr++ = '-'; FI
	WHILE n
	DO  *dptr++ = ((u_int)n)%base;
	    n=((u_int)n)/base;
	OD
	IF dptr==digs THEN *dptr++=0; FI
	WHILE dptr!=digs
	DO  k = *--dptr;
	    *digitptr++ = (k+(k<=9 ? '0' : 'a'-10));
	OD
}

printoct(o,s)
	long	o;
	int	s;
{
	int	i;
	long	po = o;
	char	digs[12];

	IF s
	THEN IF po<0
	     THEN po = -po; *digitptr++='-';
	     ELSE IF s>0 THEN *digitptr++='+'; FI
	     FI
	FI
	FOR i=0;i<=11;i++
	DO digs[i] = po&7; po >>= 3; OD
	digs[10] &= 03; digs[11]=0;
	FOR i=11;i>=0;i--
	DO IF digs[i] THEN break; FI OD
	FOR i++;i>=0;i--
	DO *digitptr++=digs[i]+'0'; OD
}

printdbl(lx,ly,fmat,base)
	int	lx, ly;
	char	fmat;
	int	base;
{
	int digs[20], *dptr;
	char k;
	double	f ,g;
	long q;

	dptr=digs;
	IF fmat!='D'
	THEN	f=leng(lx); f *= itol(1,0); f += leng(ly);
		IF fmat=='x' THEN *digitptr++='#'; FI
	ELSE	f=itol(lx,ly);
		IF f<0 THEN *digitptr++='-'; f = -f; FI
	FI
	WHILE f
	DO  q=f/base; g=q;
	    *dptr++ = f-g*base;
	    f=q;
	OD
	IF dptr==digs THEN *dptr++=0; FI
	WHILE dptr!=digs
	DO  k = *--dptr;
	    *digitptr++ = (k+(k<=9 ? '0' : 'a'-10));
	OD
}

#define	MAXIFD	5
struct {
	int	fd;
	long	r9;
} istack[MAXIFD];

	int	ifiledepth;

iclose(stack, err)
{
	IF err
	THEN	IF infile
		THEN	close(infile); infile=0;
		FI
		WHILE	--ifiledepth >= 0
		DO	IF istack[ifiledepth].fd
			THEN	close(istack[ifiledepth].fd); infile=0;
			FI
		OD
		ifiledepth = 0;
	ELIF stack == 0
	THEN	IF infile
		THEN	close(infile); infile=0;
		FI
	ELIF stack > 0
	THEN	IF ifiledepth >= MAXIFD
		THEN	error(TOODEEP);
		FI
		istack[ifiledepth].fd = infile;
		istack[ifiledepth].r9 = var[9];
		ifiledepth++;
		infile = 0;
	ELSE	IF infile
		THEN	close(infile); infile=0;
		FI
		IF ifiledepth > 0
		THEN	infile = istack[--ifiledepth].fd;
			var[9] = istack[ifiledepth].r9;
		FI
	FI
}

oclose()
{
	IF outfile!=1
	THEN	flushbuf(); close(outfile); outfile=1;
	FI
}

endline()
{
	if	((printptr - printbuf) >= maxpos)
		printc('\n');
}
