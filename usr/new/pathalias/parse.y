%{
/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)parse.y	9.1 87/10/04";
#endif /* lint */

#include "def.h"

/* exports */
extern void yyerror();

/* imports */
extern p_node addnode();
extern p_node addprivate();
extern void fixprivate(), alias(), deadlink();
extern p_link addlink();
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
extern node *getnode();
extern link *getlink();
#endif /*TMPFILES*/
extern int optind;
extern char *Cfile, *Netchars, **Argv;
extern int Argc;
extern long Lineno;

/* privates */
STATIC void fixnet();
STATIC int yylex(), yywrap(), getword();
static int Scanstate = NEWLINE;	/* scanner (yylex) state */

/* flags for ys_flags */
#define TERMINAL 1
%}

%union {
	p_node	y_node;
	Cost	y_cost;
	char	y_net;
	char	*y_name;
	struct {
		p_node ys_node;
		Cost ys_cost;
		short ys_flag;
		char ys_net;
		char ys_dir;
	} y_s;
}

%type <y_s>	site asite
%type <y_node>	links aliases plist network nlist host pelem snode delem dlist
%type <y_cost>	cost cexpr

%token <y_name>	SITE HOST
%token <y_cost>	COST
%token <y_net>	NET
%token EOL PRIVATE DEAD

%left	'+' '-'
%left	'*' '/'

%%
map	:	/* empty */
	|	map		EOL
	|	map links	EOL
	|	map aliases	EOL
	|	map network	EOL
	|	map private	EOL
	|	map dead	EOL
	|	error		EOL
	;

links	: host site cost {
		p_link l;

		l = addlink($1, $2.ys_node, $3, $2.ys_net, $2.ys_dir);
		if (GATEWAYED(getnode($2.ys_node)))
			getlink(l)->l_flag |= LGATEWAY;
		if ($2.ys_flag & TERMINAL)
			getlink(l)->l_flag |= LTERMINAL;
	  }			
	| links ',' site cost {
		p_link l;

		l = addlink($1, $3.ys_node, $4, $3.ys_net, $3.ys_dir);
		if (GATEWAYED(getnode($3.ys_node)))
			getlink(l)->l_flag |= LGATEWAY;
		if ($3.ys_flag & TERMINAL)
			getlink(l)->l_flag |= LTERMINAL;
	  }
	| links ','	/* permit this benign error */
	;

aliases	: host '=' SITE	{alias($1, addnode($3));}
	| aliases ',' SITE	{alias($1, addnode($3));}
	| aliases ','	/* permit this benign error */
	;

network	: host '=' '{' nlist '}' cost	{fixnet($1, $4, $6, DEFNET, DEFDIR);}
	| host '=' NET '{' nlist '}' cost	{fixnet($1, $5, $7, $3, LRIGHT);}
	| host '=' '{' nlist '}' NET cost	{fixnet($1, $4, $7, $6, LLEFT);}
	;

private	: PRIVATE '{' plist '}'			/* list of privates */
	| PRIVATE '{' '}'	{fixprivate();}	/* end scope of privates */
	;

dead	: DEAD '{' dlist '}';

host	: HOST		{$$ = addnode($1);}
	| PRIVATE	{$$ = addnode("private");}
	| DEAD		{$$ = addnode("dead");}
	;

snode	: SITE	{$$ = addnode($1);} ;

asite	: SITE {
		$$.ys_node = addnode($1);
		$$.ys_flag = 0;
	  }
	| '<' SITE '>' {
		$$.ys_node = addnode($2);
		$$.ys_flag = TERMINAL;
	  }
	;

site	: asite {
		$$ = $1;
		$$.ys_net = DEFNET;
		$$.ys_dir = DEFDIR;
	  }
	| NET asite {
		$$ = $2;
		$$.ys_net = $1;
		$$.ys_dir = LRIGHT;
	  }
	| asite NET {
		$$ = $1;
		$$.ys_net = $2;
		$$.ys_dir = LLEFT;
	  }
	;

pelem	: SITE	{$$ = addprivate($1);} ;

plist	: pelem			{getnode($1)->n_flag |= ISPRIVATE;}
	| plist ',' pelem	{getnode($3)->n_flag |= ISPRIVATE;}
	| plist ','		/* permit this benign error */
	;

delem	: SITE			{deadlink(addnode($1), (p_node) 0);}
	| snode NET snode	{deadlink($1, $3);}
	;

dlist	: delem
	| dlist ',' delem
	| dlist ','		/* permit this benign error */
	;

nlist	: SITE		{$$ = addnode($1);}
	| nlist ',' snode {
			if (getnode($3)->n_net == 0) {
				getnode($3)->n_net = $1;
				$$ = $3;
			}
	  }
	| nlist ','	/* permit this benign error */
	;
		
cost	: {$$ = DEFCOST;	/* empty -- cost is always optional */}
	| '(' {Scanstate = COSTING;} cexpr {Scanstate = OTHER;} ')'
		{$$ = $3;}
	;

cexpr	: COST
	| '(' cexpr ')'   {$$ = $2;}
	| cexpr '+' cexpr {$$ = $1 + $3;}
	| cexpr '-' cexpr {$$ = $1 - $3;}
	| cexpr '*' cexpr {$$ = $1 * $3;}
	| cexpr '/' cexpr {
		if ($3 == 0)
			yyerror("zero divisor\n");
		else
			$$ = $1 / $3;
	  }
	;
%%

void
yyerror(s)
	char *s;
{
	/* a concession to bsd error(1) */
	if (Cfile)
		fprintf(stderr, "\"%s\", ", Cfile);
	else
		fprintf(stderr, "%s: ", Argv[0]);
	fprintf(stderr, "line %ld: %s\n", Lineno, s);
}

/*
 * patch in the costs of getting on/off the network.
 *
 * for each network member on netlist, add links:
 *	network -> member	cost = 0;
 *	member -> network	cost = parameter.
 *
 * if network and member both require gateways, assume network
 * is a gateway to member (but not v.v., to avoid such travesties
 * as topaz!seismo.css.gov.edu.rutgers).
 *
 * note that members can have varying costs to a network, by suitable
 * multiple declarations.  this is a feechur, albeit a useless one.
 */
STATIC void
fixnet(network, nlist, cost, netchar, netdir)
	register p_node network;
	p_node nlist;
	Cost cost;
	char netchar, netdir;
{	register p_node member;
	register p_node nextnet;
	p_link l;

	getnode(network)->n_flag |= NNET;

	/* now insert the links */
	for (member = nlist ; member; member = nextnet) {
		/* network -> member, cost is 0 */
		l = addlink(network, member, (Cost) 0, netchar, netdir);
		if (GATEWAYED(getnode(network)) && GATEWAYED(getnode(member)))
			getlink(l)->l_flag |= LGATEWAY;

		/* member -> network, cost is parameter */
		(void) addlink(member, network, cost, netchar, netdir);
		nextnet = getnode(member)->n_net;
		getnode(member)->n_net = 0;	/* clear for later use */
	}
}

/* scanner */

#define QUOTE '"'
#define STR_EQ(s1, s2) (s1[2] == s2[2] && strcmp(s1, s2) == 0)
#define NLRETURN() {Scanstate = NEWLINE; return EOL;}

static struct ctable {
	char *cname;
	Cost cval;
} ctable[] = {
	/* ordered by frequency of appearance in a "typical" dataset */
	{"DIRECT", 200},
	{"DEMAND", 300},
	{"DAILY", 5000},
	{"HOURLY", 500},
	{"DEDICATED", 95},
	{"EVENING", 1800},
	{"LOCAL", 25},
	{"LOW", 5},	/* baud rate, quality penalty */
	{"DEAD", INF/2},
	{"POLLED", 5000},
	{"WEEKLY", 30000},
	{"HIGH", -5},	/* baud rate, quality bonus */
	{"FAST", -80},	/* high speed (>= 9.6 kbps) modem */
	/* deprecated */
	{"ARPA", 100},
	{"DIALED", 300},
	{0, 0}
};

STATIC int
yylex()
{	static char retbuf[128];	/* for return to yacc part */
	register int c;
	register char *buf = retbuf;
	register struct ctable *ct;
	register Cost cost;
	char errbuf[128];

	if (feof(stdin) && yywrap())
		return EOF;

	/* count lines, skip over space and comments */
	if ((c = getchar()) == EOF)
		NLRETURN();
    
continuation:
	while (c == ' ' || c == '\t')
		if ((c = getchar()) == EOF)
			NLRETURN();

	if (c == '#')
		while ((c = getchar()) != '\n')
			if (c == EOF)
				NLRETURN();

	/* scan token */
	if (c == '\n') {
		Lineno++;
		if ((c = getchar()) != EOF) {
			if (c == ' ' || c == '\t')
				goto continuation;
			ungetc(c, stdin);
		}
		NLRETURN();
	}

	switch(Scanstate) {
	case COSTING:
		if (isdigit(c)) {
			cost = c - '0';
			for (c = getchar(); isdigit(c); c = getchar())
				cost = (cost * 10) + c - '0';
			ungetc(c, stdin);
			yylval.y_cost = cost;
			return COST;
		}

		if (getword(buf, c) == 0) {
			for (ct = ctable; ct->cname; ct++)
				if (STR_EQ(buf, ct->cname)) {
					yylval.y_cost = ct->cval;
					return COST;
				}
			sprintf(errbuf, "unknown cost (%s), using default", buf);
			yyerror(errbuf);
			yylval.y_cost = DEFCOST;
			return COST;
		}

		return c;	/* pass the buck */

	case NEWLINE:
		Scanstate = OTHER;
		if (getword(buf, c) != 0)
			return c;
		/* `private' (but not `"private"')? */
		if (c == 'p' && STR_EQ(buf, "private"))
			return PRIVATE;
		/* `dead' (but not `"dead"')? */
		if (c == 'd' && STR_EQ(buf, "dead"))
			return DEAD;

		yylval.y_name = buf;
		return HOST;
	}

	if (getword(buf, c) == 0) {
		yylval.y_name = buf;
		return SITE;
	}

	if (index(Netchars, c)) {
		yylval.y_net = c;
		return NET;
	}

	return c;
}

/*
 * fill str with the next word in [0-9A-Za-z][-._0-9A-Za-z]+ or a quoted
 * string that contains no newline.  return -1 on failure or EOF, 0 o.w.
 */ 
STATIC int
getword(str, c)
	register char *str;
	register int c;
{
	if (c == QUOTE) {
		while ((c = getchar()) != QUOTE) {
			if (c == '\n') {
				yyerror("newline in quoted string\n");
				ungetc(c, stdin);
				return -1;
			}
			if (c == EOF) {
				yyerror("EOF in quoted string\n");
				return -1;
			}
			*str++ = c;
		}
		*str = 0;
		return 0;
	}

	/* host name must start with alphanumeric or `.' */
	if (!isalnum(c) && c != '.')
		return -1;

yymore:
	do {
		*str++ = c;
		c = getchar();
	} while (isalnum(c) || c == '.' || c == '_');

	if (c == '-' && Scanstate != COSTING)
		goto yymore;

	ungetc(c, stdin);
	*str = 0;
	return 0;
}

STATIC int
yywrap()
{	char errbuf[100];

	fixprivate();	/* munge private host definitions */
	Lineno = 1;
	while (optind < Argc) {
		if (freopen((Cfile = Argv[optind++]), "r", stdin) != 0)
			return 0;
		sprintf(errbuf, "%s: %s", Argv[0], Cfile);
		perror(errbuf);
	}
	freopen("/dev/null", "r", stdin);
	return -1;
}
