# include	<stdio.h>
# include "dextern"

extern FILE	*Fout, *Fin;
  /*     * * * *    y a c c     * * * *     */

  /*      features to be fixed up ...

  ***  Print estimate of total space needed for parser
  ***  Either list inputs on y.output, or list empty prdn's in states
  ***  Mention nonterms not used (or, rules. not reduced) as nonfatal error
  ***  Output states where conflicts were found by default on y.output
  ***  Engage in newspeak: production=>grammar rules, term=>token, etc.
  ***  handle # define, #ifdef, etc., in yacc actions, %{ %}
  */

  /*      new features to be added

  ***  reductions by single productions ( by request )
  ***  follow sets for start symbol
  ***  option to only do slr(1)
  ***  easily changed array names on output
  ***  allocate core, rather than predefined
  ***  input controlled by a grammar
  ***  support multiple choices for  conflicts
  ***  better conflict diagnostics
  */



main(argc,argv) int argc; char *argv[]; {
  auto int n;

  Fin = stdin;
  Fout = stdout;

  whereami();
  setup(argc,argv); /* initialize and read productions */
  tbitset = (nterms+16)/16;
  cpres(); /* make table of which productions yield a given nonterminal */
  cempty(); /* make a table of which nonterminals can match the empty string */
  cpfir(); /* make a table of e free first lists */
  stagen(); /* generate the states */
  output();  /* write the states and the tables */
  go2out();
  summary();
  windup();
  }

whereami(){ /* sets the variable machine to UNIX, GCOS, or IBM */

  int i;

  i = 1;
  i = i << 30;
  if( i == 0 ) {
    machine = UNIX;
    return;
    }
  i = i << 4;
  if( i == 0 ){
    machine = IBM;
    return;
    }
  machine = GCOS;
  }

windup(){
  /* no errors, do the optimization if appropriate */
  char *cp;
  int i;

  cflush(stdout);
  if( !oflag ) cexit(0);

  for( i=3; i<10; ++i ) cclose(i);
  switch( machine ){

  case GCOS:
    if( rflag ){
      if( Foutput==0 ) system( "./yopt -r" );
      else system( "./yopt -rv" );
      }
    else {
      if( Foutput==0 ) system( "./yopt" );
      else system( "./yopt -v" );
      }
    cexit(0);  /* terminate */

  case UNIX:
    cp = "/usr/yacc/yopti";
    if( rflag ) execl( cp, cp, (Foutput==0)?"-r":"-rv", 0 );
    else if( Foutput==0 ) execl( cp, cp, 0 );
    else execl( cp, cp, "-v", 0 );
    error( "optimization execl call fails" );

  case IBM:
    if( rflag ){
      if( Foutput==0 ) system( "MH2019.yaccopt -r" );
      else system( "MH2019.yaccopt -rv" );
      }
    else {
      if( Foutput==0 ) system( "MH2019.yaccopt" );
      else system( "MH2019.yaccopt -v" );
      }
    cexit(0);

    }

  }

settty()
/*	sets the output file to y.output */
{	
	cflush( Foutput );  /* a bit of a cheat */
	cout = Foutput;
	}

settab(){ /* sets the output file to y.tab.c */
	
	cflush( ftable );
	cout = ftable;
	}

char *chcopy( p, q )  char *p, *q; {
	/* copies string q into p, returning next free char ptr */
	while( *p = *q++ ) ++p;
	return( p );
	}

char *writem(pp) struct item *pp; { /* creates output string for item pointed to by pp */
	int i,*p;
	static char sarr[100];
	char *q;

	for( p=pp->pitem; *p>0 ; ++p ) ;
	p = prdptr[-*p];
	q = chcopy( sarr, nontrst[*p-NTBASE].name );
	q = chcopy( q, " : " );

	for(;;){
		*q++ = ++p==(pp->pitem) ? '_' : ' ';
		if((i = *p) <= 0) break;
		q = chcopy( q, symnam(i) );
		}

	*q = '\0' ;
	return( sarr );
	}

char *symnam(i){ /* return a pointer to the name of symbol i */
	char *cp;

	cp = (i>=NTBASE) ? nontrst[i-NTBASE].name : trmset[i].name ;
	if( *cp == ' ' ) ++cp;
	return( cp );
	}

summary(){ /* output the summary on the tty */

  int i, s, *pn;
  

	if (!rflag && !sflag)
	{
		settab();
		fprintf(Fout, "\nint nterms = %d;",nterms);
		fprintf(Fout, "\nint nnonter = %d;", nnonter);
		fprintf(Fout, "\nint nstate = %d;", nstate);
		fprintf(Fout, "\nchar *yysterm[] = {");
		for (i=1;i<=nterms;i++) if( trmset[i].value >= 0400 ) fprintf(Fout, "\n\"%s\",",symnam(i));
		fprintf(Fout,  "\n0 };\n" );
		fprintf(Fout, "\nchar *yysnter[] = {");
		for (i=0;i<nnonter;i++) fprintf(Fout, "\n\"%s\",",nontrst[i].name);
		fprintf(Fout, "\n\"%s\" };\n",nontrst[nnonter].name);
		}

  settty();
  fprintf(Fout, "\n%d/%d terminals, %d/%d nonterminals\n", nterms, tlim,
      nnonter, ntlim );
  fprintf(Fout, "%d/%d grammar rules, %d/%d states\n", nprod, prdlim, nstate, stsize );
  fprintf(Fout, "%d shift/reduce, %d reduce/reduce conflicts reported\n", zzsrconf, zzrrconf );
  pn = pstate[nstate+1];
  fprintf(Fout, "%d/%d working sets used\n", zzcwset,  wssize );
  fprintf(Fout, "memory: states,etc. %d/%d, parser %d/%d\n", pn-mem0, memsiz,
      memact, actsiz );
  fprintf(Fout, "%d/%d distinct lookahead sets\n", nlset, lsetsz );
  fprintf(Fout, "%d extra closures\n", zzclose - 2*nstate );
  fprintf(Fout, "%d action entries\n", zzacent );
  fprintf(Fout, "%d action entries saved through merging %d states\n",zzacsave,zznsave);
  fprintf(Fout, "%d goto entries\n", zzgoent );
  fprintf(Fout, "%d entries saved by goto default\n", zzgobest );
  if( zzsrconf!=0 || zzrrconf!=0 ){
    cflush( stderr );
    cout = stderr;
    fprintf(Fout, "\nconflicts: ");
    if( zzsrconf )fprintf(Fout,  "%d shift/reduce" , zzsrconf );
    if( zzsrconf && zzrrconf )fprintf(Fout,  ", " );
    if( zzrrconf )fprintf(Fout,  "%d reduce/reduce" , zzrrconf );
    fprintf(Fout,  "\n" );
    }
  }

error(s,a1){ /* write out error comment */
	
	int c;
	++nerrors;
	cflush( stderr );
	cout = stderr;   /* set output to tty */
	fprintf(Fout, "\n fatal error: ");
	fprintf(Fout, s,a1);
        fprintf(Fout, ", line %d\n", lineno );
	if( !fatfl ) return;
	summary();
	cexit(1);
	}

arrset(s) char s[]; {
	fprintf(Fout, "\nint %s[] = {0", s );
	arrndx = 1;
	}

arrval(n){
	fprintf(Fout, ",%d",n);
	if( (++arrndx%10) == 0 ) fprintf(Fout, "\n");
	}

arrdone(){
	fprintf(Fout, ",-1};\n");
	}

copy(v) char *v; {	/* copy ctokn to v */
	char *p;

	p=ctokn;
	while( *v++ = *p++ );
	}

compare(v) char *v; {	/* compare ctokn with v */
	char *p;

	for( p=ctokn; ; ++p ){
		if( *p != *v++ ) return( 0 );
		if( *p == 0 ) return(1);
		}
	}

int *yalloc(n){ /* allocate n+1 words from vector mem */
	int *omem;
	omem = mem;
	mem += n+1;
	if(mem-mem0 >= memsiz) error("memory overflow");
	return(omem);
	}

aryfil( v, n, c ) int *v,n,c; { /* set elements 0 through n-1 to c */
  int i;
  for( i=0; i<n; ++i ) v[i] = c;
  }

sunion( a, b, c ) int *a, *b, *c; {
  /* set a to the union of b and c */
  /* a may equal b */
  /* return 1 if c is not a subset of b, 0 otherwise */

  _REGISTER int i, x, sub;

  sub = 0;
  for( i=0; i<tbitset; ++i ){
    x = b[i] | c[i];
    if( x != b[i] ) sub=1;
    a[i] = x;
    }
  return( sub );
  }

prlook( pp ) int *pp;{
	int j;
	pp = pp->lset;
	if( pp == 0 ) fprintf(Fout, "\tNULL");
	else {
		fprintf(Fout, " { " );
		for( j=1; j<=nterms; ++j ){
			if( (pp[j>>4]>>(j&017) )&01 != 0 ) fprintf(Fout,  "%s ", symnam(j) );
			}
		fprintf(Fout,  "}" );
		}
	}
