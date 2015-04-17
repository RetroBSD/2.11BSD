/*
 * macro-11 cross reference generator
 * Jim Reeds for use with Harvard m11.
 *
 * Usage:
 *	macxrf -[olsmpcer] infile [outfile]
 *
 *	-l	spool output to lpr
 *	-o	if specified, send output to stdout.  else to av[2]
 *	-s	make a user-defined symbol cross reference
 * 	-m	make a macro cross reference
 *	-p	make a permanent symbol cross reference
 *	-c	make a .pcsect cross reference. ['c' is holdover from rt-11,
 *					where the only .psects are .csects]
 *	-e	make an error cross reference. [keyed by one-letter codes]
 *	-r	make a register use cross reference
 *	
 *	If none of [smpcer] are specified the default of "sme" is used.
 *
 *	Effciency bug: should take input ('infile') from a pipe.
 */

#include	<stdio.h>
#define	LPR	"/usr/ucb/bin/lpr"
#define	NCOLS	8

/*
 * Codes emitted by m11
 */

#define	PAGE	02
#define	LINE	03

#include	<ctype.h>
char	*lets = "smpcer";
int	want[6];

char	*desire;
int	outflag;
int	lprflag;
int	debug;
char	*Roll[] = {
	"Symbols",
	"Macros",
	"Permanent symbols",
	"Psects",
	"Errors",
	"Registers"
};
/*
 * # means a definition reference
 * * means a destructive reference
 */
char	*Code[] = {"  ", "# ", "* ", "#*"};
long	tcount[6];		/* keep track of how many of each type */
int	tany  = 0;

/*
 *	Structure to store a reference.
 */

struct item {
	char	*i_name;
	int	i_code;
	int	i_page;
	int	i_line;
	struct item	*i_left, *i_right;
} *ihead = NULL;

/*
 *	Text strings are stored in a tree.  This way
 *	duplicate references to the same string use the same core.
 *	This whole scheme is of doubtful cost-effectiveness.
 */
 struct string {
	char *s_data;
	struct string *s_left, *s_right;
} *shead = NULL;

int	Maxpage;
char	*symnam;

main(ac, av)
	char **av;
{
	char *s;
	int i;
	char	symbol[7];
	int	code;
	int page, line, c;
	FILE	*x = fopen(av[2], "r");
	int	Type;
	if(x == NULL)
		perror(av[2]),exit(1);
	lprflag = any('l', av[1]);
	outflag = any('o', av[1]);
	debug = any('x', av[1]);
	if(join(lets, av[1]))
		desire = av[1];
	else
		desire = "sme";		/* default cref request */

	for(i=0;i<6;i++)
		want[i] = any(lets[i], desire);

	if(!outflag)
		freopen(av[3], "a", stdout), fseek(stdout, 0L, 2);
	
	unlink(av[2]);
	line = 0;
	page = 0;
	while((c = getc(x)) != EOF)
	{
		if (c==0)
			break;
		else if(c == PAGE)
			line = 0, page++;
		else if(c == LINE)
			line++;
		else if (c<040 && (c >= 020))
		{
			Type = c - 020;
			if(Type>5)
				fprintf(stderr, "*** CREF TYPE %o ***\n", c);
		}
		else if (c >= 040)
		{
			ungetc(c, x);
			s = symbol;
			while( (c = getc(x)) >= 040)
			{
				if(isupper(c))
					c =tolower(c);
				*s++ = c;
			}
			*s = 0;

			if(c <020 || c > 023 )
				fprintf(stderr, "*** CREF CODE %o ***\n", c);
			process(Type, symbol, c & 3, page, line);	
		}
	}



	if(debug)
		dump(ihead);
	if(debug)
	{
		fprintf(stderr, "tany=%d\n", tany);
		for(i=0; i<ac; i++)fprintf(stderr, "%s\n", av[i]);
		for(i=0; i<6; i++) fprintf(stderr, "want = %d, tcount = %D\n",
					want[i], tcount[i]);
	}
	if(tany) {
		printf("\nMacro-11 Cross Reference Listing.\n");
		for(i=0; i<6; i++) {
			if(tcount[i] == 0L) continue;
			if(want[i] == 0) continue;
			printf("\n%s:\n", Roll[i]);
			symnam = 0;
			crprint(ihead, i);
			printf("\n");
		}
	}
	if(lprflag && outflag) {
		execl(LPR, LPR, av[3], 0);
		perror(LPR);
		exit(1);
	}
	exit(0);
}

char *__s;

int	nocore = 0;
process(t, s, c, page, l)
	char *s;
{
	register struct item *p;
	struct item *addtree();
	char *calloc(), *stash();

	if(page>Maxpage)
		Maxpage = page;
	if(debug)
		fprintf(stderr, "process(%o, %s, %o, %d, %d)\n",
					t,  s,  c, page,l);
	if(nocore)
		return;
	if(want[t] == 0) return;
	p = (struct item *) calloc(1, sizeof (struct item));
	if(p == NULL)
	{
		fprintf(stderr, "Cref: Ran out of core, page %d, line %d\n",p, l);
		nocore++;
		return;
	}
	tcount[t]++;
	tany |= 1;
	p->i_code = t | (c<<8);
	p->i_name = stash(s);
	p->i_page = page;
	p->i_line = l;

	ihead = addtree(ihead, p);
}

comp(p, q)
	register struct item *p, *q;
{
	register x;

	if(p->i_name != q->i_name)
		x = strcmp(p->i_name, q->i_name);
	else
		x = 0;
	if(x == 0) {
		x = p->i_page - q->i_page;
		if(x == 0)
			x = p->i_line - q->i_line;
		if(x == 0)
			x= p->i_code - q->i_code;
	}
	return(x);
}

struct item *
addtree(b, p)
	struct item *b, *p;
{
	register c;

	if(b == NULL)
		return(b = p);
	c = comp(b, p);
	if(c < 0)
		b->i_left = addtree(b->i_left, p);
	else if (c>0)
		b->i_right= addtree(b->i_right, p);
	return(b);
}

char *stash(s)
	register char *s;
{
	shead = store(shead, s);
	return(__s);
}
store(b, s)
	register struct string *b;
	register char *s;
{
	register x;

	if(b == 0) {
		b = (struct string *) calloc(1, sizeof(struct string));
		__s = b->s_data = strsave(s);
	} else if((x=strcmp(b->s_data, s)) == 0)
		__s = b->s_data;
	else if(x<0)
		b->s_left = store(b->s_left, s);
	else
		b->s_right= store(b->s_right,s);
	return(b);
}
strsave(s)
	register char *s;
{
	register char *t;
	char  *calloc();

	t = calloc(strlen(s)+1, 1);
	strcpy(t, s);
	return(t);
}
int	crcol = 0;
crprint(p, i)
	register struct item *p;
{
	if(p == NULL) return;

	crprint(p->i_right, i);
	if ((p->i_code & 07) == i) {
		char *code;
		if (symnam != p->i_name) 
			crcol = 0, printf("\n%-6.6s\t", symnam = p->i_name);
		if(crcol == NCOLS) {
			crcol = 1;
			printf("\n\t");
		}
		else crcol++;


		if(i == 4 || i == 2)
			code = 0;
		else
			code = Code[3 & (p->i_code >>8)];
		
		prx(p->i_page, p->i_line, code, crcol);
	}
	crprint(p->i_left, i);
}
prx(page, line, code, col)
	char *code;
{
	char buf[15];

	if(Maxpage>1)
		sprintf(buf, "%d-%d", page, line);
	else
		sprintf(buf, "%d", line);
	if(code)
		strcat(buf, code);
	if(col == NCOLS)
		printf("%s", buf);
	else {
		printf("%8s", buf);
		if(strlen(buf) > 7) printf(" ");
	}
}
join(s,t)
	register char *s, *t;
{
	for(;*s;s++)
		if(any(*s, t))
			return(1);
	return(0);
}

any(c, s)
	register char c, *s;
{
	for(;*s;s++)
		if(*s == c)
			return(1);
	return(0);
}
minuspr(x, y)
{
	char buf[30];
	sprintf(buf,"%d-%d", x, y);
	printf(" %4s", buf);
}
dump(p)
	struct item *p;
{
	if(p==NULL)return;
	dump(p->i_left);
	fprintf(stderr, "%s: %o %d %d\n", p->i_name, p->i_code, p->i_page, p->i_line);
	dump(p->i_right);
}
