# include	<stdio.h>
# include "dextern"

extern FILE	*Fout, *Fin;

# define IDENTIFIER 257
# define MARK 258
# define TERM 259
# define LEFT 260
# define BINARY 261
# define RIGHT 262
# define PREC 263
# define LCURLY 264
# define C_IDENTIFIER 265  /* name followed by colon */
# define NUMBER 266

setup(argc,argv) int argc; char *argv[];
{	int i,j,lev,t;
	int c;

	Foutput = NULL;
	i = 1;
	while( argc >= 2  && argv[1][0] == '-' ) {
		while( *++(argv[1]) ){
			switch( *argv[1] ){
			case 'v':
			case 'V':
				Foutput = copen("y.output", 'w' );
				if( Foutput == 0 ) error( "cannot open y.output");
				continue;
			case 'o':
			case 'O':
				oflag = 1;
				continue;
			case 'r':
			case 'R':
				rflag = 1;
				oflag = 1;
				continue;
			case 's':
			case 'S':
				sflag = 1;
				continue;
			default:  error( "illegal option: %c", *argv[1]);
				}
			}
		argv++;
		argc--;
		}

	if (Foutput == 0)
		Foutput = copen("/dev/null", 'w');
	ftable = copen( oflag ? "yacc.tmp" : "y.tab.c" , 'w' );
	if( ftable<0 ) error( "cannot open table file" );
	if( argc > 1 ) cin = copen( argv[1], 'r' );
	if( cin < 0 ) error( "cannot open input" );
	settab();
	fprintf(Fout, "#\n");
	ctokn = "$end";
	defin(0);  /* eof */
	extval = 0400;  /* beginning of assigned values */
	ctokn = "error";
	defin(0);
	ctokn = "$accept";
	defin(1);
	mem=mem0;
	cnamp = cnames;
	lev=0;
	i=0;

	while( t = gettok() ){
		switch( t ){
			case IDENTIFIER:	j = chFind(0);
					trmlev[j] = lev;
					continue;
			case ',':
			case ';':		continue;
			case TERM:		lev=0; continue;
			case LEFT:		lev=(++i<<3)|01; continue;
			case BINARY:	lev=(++i<<3)|02; continue;
			case RIGHT:	lev=(++i<<3)|03; continue;
			case MARK:
					deFout();
					if( rflag ){ /* RATFOR */
						fprintf(Fout,  "define yyerrok yyerrf = 0\n" );
						fprintf(Fout,  "define yyclearin yychar = -1\n" );
						fprintf(Fout,  "subroutine yyactr(yyprdn)\n");
						fprintf(Fout,  "common/yycomn/yylval,yyval,yypv,yyvalv(150)\n" );
						fprintf(Fout,  "common/yylcom/yychar,yyerrf,yydebu\n" );
						fprintf(Fout,  "integer yychar, yyerrf, yydebu\n" );
						fprintf(Fout,  "integer yyprdn,yyval,yylval,yypv,yyvalv\n" );
						}
					else {
						fprintf(Fout,  "#define yyclearin yychar = -1\n" );
						fprintf(Fout,  "#define yyerrok yyerrflag = 0\n" );
						fprintf(Fout,  "extern int yychar, yyerrflag;\n" );
						fprintf(Fout, "\nint yyval = 0;\nint *yypv;\nint yylval = 0;");
						fprintf(Fout, "\nyyactr(__np__){\n");
						}
					break;
			case LCURLY:	deFout();
					cpycode();
					continue;
			case NUMBER:
				trmset[j].value = numbval;
				if( j < ndeFout && j>2 ) 
					error("please define type # of %s earlier", trmset[j].name );
				continue;
			default:	error("bad precedence syntax, input %d", t );
			}
		break;
		}
	prdptr[0]=mem;
	/* added production */
	*mem++ = NTBASE;
	*mem++ = NTBASE+1;
	*mem++ = 1;
	*mem++ = 0;
	prdptr[1]=mem;
	i=0;

	/* i is 0 when a rule can begin, 1 otherwise */

	for(;;) switch( t=gettok() ) {
	case C_IDENTIFIER:		if( mem == prdptr[1] ) {  /* first time */
						if( rflag ){
							fprintf(Fout,  "goto 1000\n" );
							}
						else fprintf(Fout, "\nswitch(__np__){\n");
						}
				if( i != 0 ) error( "previous rule not terminated" );
				*mem = chFind(1);
				if( *mem < NTBASE )error( "token illegal on lhs of grammar rule" );
				i=1;
				++mem;
				continue;
	case IDENTIFIER:
			*mem=chFind(1);
			if(*mem < NTBASE)levprd[nprod]=trmlev[*mem];
			mem++;
			if(i==0) error("missing :");
			continue;
	case '=':		levprd[nprod] |= 04;
				if( i==0 ) error("semicolon preceeds action");
			fprintf(Fout,  rflag?"\n%d ":"\ncase %d:", nprod );
			cpyact();
			fprintf(Fout,  rflag ? " return" : " break;" );
	case '|':
	case ';':		if(i){
				*mem++ = -nprod;
				prdptr[++nprod] = mem;
				levprd[nprod]=0;
				i=0;}
			if (t=='|'){i=1;*mem++ = *prdptr[nprod-1];}
			continue;
	case 0:		/* End Of File */
	case MARK:	if( i != 0 ) error( "rule not terminated before %%%% or EOF" );
			settab();
			Finact();
			/* copy the programs which follow the rules */
			if( t == MARK ){
				while ((c=getc(Fin)) > 0) putc(c, Fout);
				}
			return;
	case PREC:	
		if( i==0 ) error( "%%prec must appear inside rule" );
		if( gettok()!=IDENTIFIER)error("illegal %%prec syntax" );
		j=chFind(2);
		if(j>=NTBASE)error("nonterminal %s illegal after %%prec", nontrst[j-NTBASE].name);
		levprd[nprod]=trmlev[j];
		continue;
	case LCURLY:	
		if( i!=0 ) error( "%%{ appears within a rule" );
		cpycode();
		continue;
	default: error( "syntax error, input %d", t  );
	}
}

Finact(){
	/* Finish action routine */
	register i;

	if( rflag ){

		fprintf(Fout,  "\n1000 goto(" );
		for( i=1; i<nprod; ++i ){
			fprintf(Fout,  "%d,", (levprd[i]&04)==0?999:i );
			}
		fprintf(Fout,  "999),yyprdn\n" );
		fprintf(Fout,  "999 return\nend\n" );
		fprintf(Fout,  "define YYERRCODE %d\n", trmset[2].value );
		}
	else {
		/* ------- The next line is an INGRES customization ------- */
		fprintf(Fout,  "\n}\nreturn(0);\n}\n" );
		fprintf(Fout,  "int yyerrval = %d;\n", trmset[2].value );
		}
	}
defin(t) {
/*	define ctokn to be a terminal if t=0
	or a nonterminal if t=1		*/
	char *cp,*p;
	int c;


        if (t) {
          if( ++nnonter >= ntlim ) error("too many nonterminals, limit %d",ntlim);
	  nontrst[nnonter].name = ctokn;
	  return( NTBASE + nnonter );
          }
        else {
          if( ++nterms >= tlim ) error("too many terminals, limit %d",tlim );
          trmset[nterms].name = ctokn;
	if( ctokn[0]==' ' && ctokn[2]=='\0' ) /* single character literal */
		trmset[nterms].value = ctokn[1];
	else if ( ctokn[0]==' ' && ctokn[1]=='\\' ) { /* escape sequence */
		if( ctokn[3] == '\0' ){ /* single character escape sequence */
			switch ( ctokn[2] ){
				 /* character which is escaped */
			case 'n': trmset[nterms].value = '\n'; break;
			case 'r': trmset[nterms].value = '\r'; break;
			case 'b': trmset[nterms].value = '\b'; break;
			case 't': trmset[nterms].value = '\t'; break;
			case '\'': trmset[nterms].value = '\''; break;
			case '"': trmset[nterms].value = '"'; break;
			case '\\': trmset[nterms].value = '\\'; break;
			default: error( "invalid escape" );
				}
			}
		else if( ctokn[2] <= '7' && ctokn[2]>='0' ){ /* \nnn sequence */
			if( ctokn[3]<'0' || ctokn[3] > '7' || ctokn[4]<'0' ||
				ctokn[4]>'7' || ctokn[5] != '\0' ) error("illegal \\nnn construction" );
			trmset[nterms].value = 64*(ctokn[2]-'0')+8*(ctokn[3]-'0')+ctokn[4]-'0';
			if( trmset[nterms].value == 0 ) error( "'\\000' is illegal" );
			}
		}
	else {
		trmset[nterms].value = extval++;

		}
	trmlev[nterms] = 0;
	return( nterms );
          }
}

deFout(){ /* write out the defines (at the end of the declaration section) */

	_REGISTER int i, c;
	_REGISTER char *cp;

	for( i=ndeFout; i<=nterms; ++i ){

		cp = trmset[i].name;
		if( *cp == ' ' ) ++cp;  /* literals */

		for( ; (c= *cp)!='\0'; ++cp ){

			if( c>='a' && c<='z' ||
			    c>='A' && c<='Z' ||
			    c>='0' && c<='9' ||
			    c=='_' )  ; /* VOID */
			else goto nodef;
			}

		/* define it */

		fprintf(Fout,  "%c define %s %d\n", rflag?' ':'#', trmset[i].name, trmset[i].value );

	nodef:	;
		}

	ndeFout = nterms+1;

	}

chstash( c ){
  /* put character away into cnames */
  if( cnamp >= &cnames[cnamsz] ) error("too many characters in id's and literals" );
  else *cnamp++ = c;
  }

int gettok() {
	int j, base;
	static int peekline; /* number of '\n' seen in lookahead */
	auto int c, match, reserve;

begin:
	reserve = 0;
        if( peekc>=0 ) {
		c = peekc;
		lineno += peekline;
		peekc = -1;
		peekline = 0;
		}
        else c = getc(Fin);
        while( c==' ' || c=='\n' || c=='\t' ){
          if( c == '\n' ) ++lineno;
          c=getc(Fin);
          }
	if (c=='/')
		{if (getc(Fin)!='*')error("illegal /");
		c=getc(Fin);
		while(c) {
			if( c == '\n' ) ++lineno;
			if (c=='*')
				{if((c=getc(Fin))=='/')break;}
			else c=getc(Fin);}
		if (!c) return(0);
		goto begin;}
	j=0;
	switch(c){
	case '"':	
	case '\'':	match = c;
			ctokn = cnamp;
			chstash( ' ' );
			while(1){
				c = getc(Fin);
				if( c == '\n' || c == '\0' )
					error("illegal or missing ' or \"");
				if( c == '\\' ){
					c = getc(Fin);
					chstash( '\\' );
					}
				else if( c == match ) break;
				chstash( c );
				}
			break;
	case '%':
	case '\\':	switch(c=getc(Fin))
		{case '0':	return(TERM);
		case '<':	return(LEFT);
		case '2':	return(BINARY);
		case '>':	return(RIGHT);
		case '%':
		case '\\':	return(MARK);
		case '=':	return(PREC);
		case '{':	return(LCURLY);
		default:	reserve = 1;
		}
	default:	if( c >= '0' && c <= '9' ){ /* number */
				numbval = c-'0' ;
				base = (c=='0') ? 8 : 10 ;
				for( c=getc(Fin); c>='0' && c<='9'; c=getc(Fin) ){
					numbval = numbval*base + c - '0';
					}
				peekc = c;
				return(NUMBER);
				}
			else if( (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='.'||c=='$'){
				ctokn = cnamp;
				while(	(c>='a'&&c<='z') ||
					(c>='A'&&c<='Z') ||
					(c>='0'&&c<='9') ||
					c=='_' || c=='.' || c=='$' ) {
					chstash( c );
					if( peekc>=0 ) { c = peekc; peekc = -1; }
					else c = getc(Fin);
					}
				}
			else return(c);

			peekc=c;
			}
	chstash( '\0' );

	if( reserve ){ /* Find a reserved word */
		if( compare("term")) return( TERM );
		if( compare("TERM")) return( TERM );
		if( compare("token")) return( TERM );
		if( compare("TOKEN")) return( TERM );
		if( compare("left")) return( LEFT );
		if( compare("LEFT")) return( LEFT );
		if( compare("nonassoc")) return( BINARY );
		if( compare("NONASSOC")) return( BINARY );
		if( compare("binary")) return( BINARY );
		if( compare("BINARY")) return( BINARY );
		if( compare("right")) return( RIGHT );
		if( compare("RIGHT")) return( RIGHT );
		if( compare("prec")) return( PREC );
		if( compare("PREC")) return( PREC );
		error("invalid escape, or illegal reserved word: %s", ctokn );
		}

	/* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

  look:
	while( peekc==' ' || peekc=='\t' || peekc == '\n' ) {
		if( peekc == '\n' ) ++peekline;
		peekc = getc(Fin);
		}

	if( peekc != ':' ) return( IDENTIFIER );
	peekc = -1;
	lineno += peekline;
	peekline = 0;
	return( C_IDENTIFIER );
}
chFind(t)

{	int i,j;

	if (ctokn[0]==' ')t=0;
	for(i=1;i<=nterms;i++)
		if(compare(trmset[i].name)){
			cnamp = ctokn;
			return( i );
			}
	for(i=1;i<=nnonter;i++)
		if(compare(nontrst[i].name)) {
			cnamp = ctokn;
			return( i+NTBASE );
			}
	/* cannot Find name */
	if( t>1 && ctokn[0] != ' ' )
		error( "%s should have been defined earlier", ctokn );
	return( defin( t ) );
	}

cpycode(){ /* copies code between \{ and \} */

	int c;
	c = getc(Fin);
	if( c == '\n' ) {
		c = getc(Fin);
		lineno++;
		}
	while( c ){
		if( c=='\\' )
			if( (c=getc(Fin)) == '}' ) return;
			else putc('\\', Fout);
		if( c=='%' )
			if( (c=getc(Fin)) == '}' ) return;
			else putc('%', Fout);
		putc( c , Fout);
		if( c == '\n' ) ++lineno;
		c = getc(Fin);
		}
	error("eof before %%}");
	}

cpyact(){ /* copy C action to the next ; or closing } */
	int brac, c, match, *i, j, s;

	brac = 0;

loop:
	c = getc(Fin);
swt:
	switch( c ){

case ';':
		if( brac == 0 ){
			putc( c , Fout);
			return;
			}
		goto lcopy;

case '{':
		brac++;
		goto lcopy;

case '$':
		s = 1;
		c = getc(Fin);
		if( c == '$' ){
			fprintf(Fout, "yyval");
			goto loop;
			}
		if( c == '-' ){
			s = -s;
			c = getc(Fin);
			}
		if( c>='0' && c <= '9' ){
			j=0;
			while( c>='0' && c<= '9' ){
				j= j*10+c-'0';
				c = getc(Fin);
				}
			if( rflag ) fprintf(Fout,  "yyvalv(yypv%c%d)", s==1?'+':'-', j );
			else fprintf(Fout, "yypv[%d]", s*j );
			goto swt;
			}
		putc( '$' , Fout);
		if( s<0 ) putc('-', Fout);
		goto swt;

case '}':
		brac--;
		if( brac == 0 ){
			putc( c , Fout);
			return;
			}
		goto lcopy;

case '/':	/* look for comments */
		putc( c , Fout);
		c = getc(Fin);
		if( c != '*' ) goto swt;

		/* it really is a comment */

		putc( c , Fout);
		while( (c=getc(Fin)) > 0 ){
			if( c=='*' ){
				putc( c , Fout);
				if( (c=getc(Fin)) == '/' ) goto lcopy;
				}
			putc( c , Fout);
			}
		error( "EOF inside comment" );

case '\'':	/* character constant */
		match = '\'';
		goto string;

case '"':	/* character string */
		match = '"';

	string:

		putc( c , Fout);
		while( (c=getc(Fin)) > 0 ){

			if( c=='\\' ){
				putc( c , Fout);
				c=getc(Fin);
				}
			else if( c==match ) goto lcopy;
			putc( c , Fout);
			}
		error( "EOF in string or character constant" );

case '\0':
		error("action does not terminate");
case '\n':	++lineno;
		goto lcopy;

		}

lcopy:
	putc( c , Fout);
	goto loop;
	}
