/*
 * Produce a set of preprocessor defines which guarantee that all identifiers
 * in the files are unique in the first symlen (default 7) characters.
 * Include the output into each file (or into a common header file).  Since
 * the symbols being redefined are ambiguous within symlen chars (that was the
 * problem in the first place), the files must be compiled using a flexnames
 * version of cpp.
 *
 * Lacking that, turn the output into a sed script and massage the source
 * files.  In this case, you may need to specify -p to parse preprocessor
 * lines, but watch for things like include-file names.  If using cpp,
 * preprocessor symbols should be weeded out by hand; otherwise they will
 * cause (innocuous) redefinition messages.
 *
 * Hacked by m.d. marquez to allow pipe into stdin and add -s (sed) option.
 */

#if	!defined(lint) && !defined(pdp11)
static char rcsid[] =
    "$Header: shortc.c,v 1.2 88/09/06 01:30:58 bin Exp Locker: bin $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <strings.h>

#define SYMLEN  7           /* symbols must be unique within ... chars */
#define MAXLEN  128         /* need a limit for tokenizing */
#define HASHSIZ 512        /* power of 2; not an upper limit */

typedef struct Symbol symbol;
struct Symbol {
	symbol  *link;          /* hash chain */
	union {
	    char   *xprefix;   /* prefix for mapped name if flag > SEEN */
	    symbol *xtrunc;    /* symbol which truncates to this one
				  if flag == TRUNC */
	} x;
	char    flag;
	char    inname[1];
};
#define prefix  x.xprefix
#define trunc   x.xtrunc
#define NOTSEEN 0           /* symbol never seen */
#define TRUNC   1           /* trunc points to symbol which truncates to
			       this one */
#define SEEN    2           /* there is no conflict with this symbol */
/*              > SEEN */   /* prefix must be added to resolve conflict */

symbol  *symtab[HASHSIZ];

int     symlen  = SYMLEN;
char    parsepp;            /* if set, parse preprocessor lines */
int	sedout = 0;	    /* want sed output */
int	read_file = 0;	    /* flag if read file arguments */

symbol  *lookup();
char    *token(), *truncname(), *curarg = "stdin";
char    *myalloc();

extern  char *malloc();
extern	int  errno;

/*
 * entry point
 */
main(argc, argv)
register argc;
register char **argv;
{
	char	obuf[BUFSIZ];

	setbuf(stdout, obuf);

	while (--argc > 0)
	    doarg(*++argv);

	if (!read_file)
	    process();

	dump();
	exit(0);
}

/*
 * process one file or flag arg
 */
doarg(arg)
char *arg;
{
	char	ibuf[BUFSIZ];

	if (*arg == '-') {
	    arg++;
	    if (isdigit(*arg))
		symlen = atoi(arg);
	    else if (*arg == 'p')
		parsepp = 1;
	    else if (*arg == 's')
		sedout = 1;
	    else {
		fputs("usage: shortc [-symlen] [-s] [-p] file ...\n", stderr);
		exit(1);
	    }
	    return;
	}

	if (freopen(arg, "r", stdin) == NULL) {
	    fprintf(stderr, "freopen(%s) err: %d\n", errno);
	    return;
	}
	setbuf(stdin, ibuf);

	curarg = arg;
	process();

	read_file++;
}

process()
{
	register char *s;
	register symbol *y;

	while (s = token())
	    if ((y = lookup(s))->flag < SEEN)
		newname(y);
}

/*
 * pick a new non-colliding name
 */
newname(y)
register symbol *y;
{
	register symbol *a;

	/* repeat until no collision */
	for (;;) {
	    /* pick another name */
	    nextname(y);
	    /* check its truncation for conflicts */
	    a = lookup(truncname(y));
	    if (a->flag == NOTSEEN)
		break;
	    /*
	     * if this is an original symbol and it collides with another
	     * (maybe modified) symbol, fix the other one instead of this one
	     */
	    if (a->flag == TRUNC && y->flag == SEEN) {
		newname(a->trunc);
		break;
	    }
	    /* if already a short name, ok */
	    if (a == y)
		return;
	}
	/* flag what this truncates to */
	a->trunc = y;
	a->flag = TRUNC;
}

/*
 * find next possible name for this symbol
 */
nextname(y)
register symbol *y;
{
	register char *s, *p;
	register n;

	switch (y->flag) {
	    case TRUNC:
		/* if another symbol truncates to this one, fix it not to */
		newname(y->trunc);
	    case NOTSEEN:
		/* this symbol's name is available, so use it */
		y->flag = SEEN;
		return;
	}
	/* prefix++ in base 26 */
	for (n = y->flag-SEEN, p = y->prefix, s = p+n; s > p; *s = 'A')
	    if (++*--s <= 'Z')     /* love that syntax */
		return;

	/* overflow; need new digit */
	y->prefix = s = myalloc(n+2);
	*s++ = 'A';
	if (y->flag++ == SEEN)
	    *s = '\0';
	else {
	    strcpy(s, p);
	    free(p);
	}
}

/*
 * return symbol name truncated to symlen chars
 */
char *
truncname(y)
register symbol *y;
{
	static char buf[MAXLEN+10];
	register n = y->flag-SEEN;

	/*
	 * sprintf(buf, "%.*s%.*s", n, y->prefix, symlen-n, y->inname);
	 */

	strncpy(buf, y->prefix, n);
	buf[n] = '\0';
	strncat(buf, y->inname, symlen - n);

	return buf;
}

/*
 * find name in symbol table
 */
symbol *
lookup(s)
char *s;
{
	register h;

	{
	    register char *p;
	    register c;

	    for (h = 0, p = s; (c = *p++);)
		h += h + c;
	}
	{
	    register symbol *y, **yy;

	    for (y = *(yy = &symtab[h & HASHSIZ-1]);; y = y->link) {
		if (!y) {
		    y = (symbol *)myalloc(sizeof *y + strlen(s));
		    strcpy(y->inname, s);
		    y->flag = NOTSEEN;
		    y->link = *yy;
		    *yy = y;
		    break;
		}

		if (strcmp(y->inname, s) == 0)
		    break;
	    }
	    return y;
	}
}

/*
 * output all mappings
 */
dump()
{
	register symbol *y;
	register n;

	for (n = HASHSIZ; --n >= 0;)
	    for (y = symtab[n]; y; y = y->link)
		if (y->flag > SEEN) {
		    if (sedout)
		    	printf("/%s/s//%s%s/g\n", y->inname,
						y->prefix, y->inname);
		   else
		    	printf("#define %s %s%s\n", y->inname,
						y->prefix, y->inname);
		}
}

/*
 * return next interesting identifier
 */
char *
token()
{
	register c, state = 0;
	register char *p;
	static char buf[MAXLEN+1];

	for (p = buf; (c = getchar()) != EOF;) {
	    if (state)
	    switch (state) {
		case '/':
		    if (c != '*') {
			state = 0;
			break;
		    }
		    state = c;
		    continue;

		case 'S':
		    if (c == '/')
			state = 0;
		    else
			state = '*';
		case '*':
		    if (c == state)
			state = 'S';
		    continue;

		default:
		    if (c == '\\')
			(void) getchar();
		    else if (c == state)
			state = 0;
		    continue;
	    }
	    if (isalnum(c) || c == '_') {
		if (p < &buf[sizeof buf - 1])
		    *p++ = c;
		continue;
	    }
	    if (p > buf) {
		if (p-buf >= symlen && !isdigit(*buf)) {
		    *p = '\0';
		    ungetc(c, stdin);
		    return buf;
		}
		p = buf;
	    }
	    if (c == '"' || c == '\'' || c == '/')
		state = c;
	    else if (c == '#' && !parsepp)
		state = '\n';
	}
	return NULL;
}

/*
 * malloc with error detection
 */
char *
myalloc(n)
{
	register char *p;

	if (!(p = malloc((unsigned)n))) {
	    fprintf(stderr, "Out of space in %s\n", curarg);
	    exit(1);
	}
	return p;
}
