#ifndef lint
static char *sccsid = "@(#)arpatxt.c	9.1 87/10/04";
#endif

/*
 * convert hosts.txt into pathalias format.
 *
 * alias rule: "host.dom.ain,nickname.arpa,..." -> host = nickname, ...
 */

/* remove the next line for standard or research unix */
#define BSD

#ifdef BSD
#define strchr index
#endif /* BSD */

#include <stdio.h>
#include <ctype.h>

typedef struct node node;

struct node {
	node *child;	/* subdomain or member host */
	node *parent;	/* parent domain */
	node *next;	/* sibling in domain tree or host list */
	char *name;
	node *alias;
	node *bucket;
	node *gateway;
	int  flag;
};

node *Top;
int Atflag;
int Fflag, Iflag;
char *DotArpa = ".ARPA";
char Fname[256], *Fstart;

node *newnode(), *find();
char *strsave(), *lowercase();
void crcinit();
long fold();
FILE *mkfile();

extern char *malloc(), *strchr(), *calloc(), *gets(), *strcpy(), *fgets();
extern FILE *fopen();

#define ISADOMAIN(n) ((n) && *((n)->name) == '.')

/* for node.flag */
#define COLLISION 1

/* for formatprint() */
#define PRIVATE		0
#define HOSTS		1
#define SUBDOMAINS	2

/* for usage() */
#define USAGE "usage: %s [-i@] [-g gateway] [-p privatefile] [-f | -d directory] [file ...]\n"

main(argc, argv)
	char **argv;
{	int c;
	char *progname;
	extern char *optarg;
	extern int optind;

	if ((progname = strchr(argv[0], '/')) != 0)
		progname++;
	else
		progname = argv[0];
	crcinit();

	Top = newnode();	/* needed for adding gateways */
	while ((c = getopt(argc, argv, "d:fg:ip:@")) != EOF)
		switch(c) {
		case 'd':
			strcpy(Fname, optarg);
			break;
		case 'f':	/* filter mode -- write on stdout */
			Fflag++;
			break;
		case 'g':
			gateway(optarg);
			break;
		case 'i':
			Iflag++;
			break;
		case 'p':
			readprivates(optarg);
			break;
		case '@':
			Atflag++;
			break;
		default:
			usage(progname);
		}

	if (Fflag && *Fname)
		usage(progname);
	if (Iflag)
		(void) lowercase(DotArpa);
	if (Top->gateway == 0)
		fprintf(stderr, "%s: warning: no gateway to top level domains\n", progname);

	Fstart = Fname + strlen(Fname);
	if (Fstart != Fname) {
		*Fstart++ = '/';
		*Fstart = 0;
	}
	/* should do the mkdir here instead of the shell script ...*/
		
	Top->name = "internet";
	if (optind == argc)
		scan();
	else for ( ; optind < argc; optind++) {
		if (freopen(argv[optind], "r", stdin) == 0) {
			perror(argv[optind]);
			continue;
		}
		scan();
	}
	merge();
	dump(Top);
	return 0;
}

scan()
{	static first;
	char buf0[BUFSIZ], buf1[BUFSIZ], buf2[BUFSIZ];

	if (first++ == 0)
		(void) re_comp("^HOST.*SMTP");
	while (gets(buf0) != 0) {
		if (re_exec(buf0) == 0)
			continue;
		if (sscanf(buf0, "HOST : %[^:] : %[^: ]", buf1, buf2) != 2)
			continue;
		if (Iflag)
			(void) lowercase(buf2);
		insert(buf2);
	}
}
/*
 * format of private file:
 *	one per line, optionally followed by white space and comments
 *	line starting with # is comment
 */
readprivates(pfile)
	char *pfile;
{	FILE *f;
	node *n;
	char buf[BUFSIZ], *bptr;

	if ((f = fopen(pfile, "r")) == 0) {
		perror(pfile);
		return;
	}
	while (fgets(buf, BUFSIZ, f) != 0) {
		if (*buf == '#')
			continue;
		if ((bptr = strchr(buf, ' ')) != 0)
			*bptr = 0;
		if ((bptr = strchr(buf, '\t')) != 0)
			*bptr = 0;
		if (*buf == 0)
			continue;
		n = newnode();
		n->name = strsave(buf);
		hash(n);
	}
	(void) fclose(f);
}
usage(progname)
	char *progname;
{
	fprintf(stderr, USAGE, progname);
	exit(1);
}
dumpgateways(ndom, f)
	node *ndom;
	FILE *f;
{	node *n;

	for (n = ndom->gateway; n; n = n->next) {
		fprintf(f, "%s ", n->name);
		if (Atflag)
			putc('@', f);
		fprintf(f, "%s(%s)\t# gateway\n", ndom->name,
				ndom == Top ? "DEDICATED" : "LOCAL");
	}
}

gateway(buf)
	char *buf;
{	node *n, *dom;
	char *dot;

	dot = strchr(buf, '.');
	if (dot) {
		dom = find(dot);
		*dot = 0;
	} else
		dom = Top;

	n = newnode();
	n->name = strsave(buf);
	n->next = dom->gateway;
	dom->gateway = n;
}
	
insert(buf)
	char *buf;
{	char host[BUFSIZ], *hptr, *dot;
	node *n;

	for (hptr = host; *hptr = *buf++; hptr++)
		if (*hptr == ',')
			break;

	if (*hptr == ',')
		*hptr = 0;
	else
		buf = 0;	/* no aliases */

	if ((dot = strchr(host, '.')) == 0)
		abort();	/* can't happen */
	
	if (strcmp(dot, DotArpa) == 0)
		buf = 0;		/* no aliases */

	n = find(dot);
	*dot = 0;

	addchild(n, host, buf);
}

node *
find(domain)
	char *domain;
{	char *dot;
	node *parent, *child;

	if (domain == 0)
		return Top;
	if ((dot = strchr(domain+1, '.')) != 0) {
		parent = find(dot);
		*dot = 0;
	} else
		parent = Top;

	for (child = parent->child; child; child = child->next)
		if (strcmp(domain, child->name) == 0)
			break;
	if (child == 0) {
		child = newnode();
		child->next = parent->child;
		parent->child = child;
		child->parent = parent;
		child->name = strsave(domain);
	}
	return child;
}

node *
newnode()
{
	node *n;

	if ((n = (node *) calloc(1, sizeof(node))) == 0)
		abort();
	return n;
}

char *
strsave(buf)
	char *buf;
{	char *mstr;

	if ((mstr = malloc(strlen(buf)+1)) == 0)
		abort();
	strcpy(mstr, buf);
	return mstr;
}

addchild(n, host, aliases)
	node *n;
	char *host, *aliases;
{	node *child;

	/* check for dups?  nah! */
	child = newnode();
	child->name = strsave(host);
	child->parent = n;
	child->next = n->child;
	makealiases(child, aliases);
	n->child = child;
}

/* yer basic tree walk */
dump(n)
	node *n;
{	node *child;
	FILE *f;
	int hadprivates = 0;

	if (n->child == 0)
		return;

	f = mkfile(n);

	if (n != Top && ! ISADOMAIN(n))
		abort();

	hadprivates = domprint(n, f);
	dumpgateways(n, f);
	if (hadprivates || n == Top)
		fputs("private {}\n", f);	/* end scope of privates */
	if (!Fflag)
		(void) fclose(f);
	else
		putc('\n', f);
	for (child = n->child; child; child = child->next)
		dump(child);
}

qcmp(a, b)
	node **a, **b;
{
	return strcmp((*a)->name, (*b)->name);
}

domprint(n, f)
	node *n;
	FILE *f;
{	node *table[10240], *child, *alias;
	char *cost = 0;
	int nelem, i, rval = 0;

	/* dump private definitions */
	/* sort hosts and aliases in table */
	i = 0;
	for (child = n->child; child; child = child->next) {
		table[i++] = child;
		for (alias = child->alias; alias; alias = alias->next)
			table[i++] = alias;
	}

	qsort((char *) table, i, sizeof(table[0]), qcmp);
	formatprint(f, table, i, PRIVATE, "private", cost);

	/* rval == 0 IFF no privates */
	while (i-- > 0)
		if (table[i]->flag & COLLISION) {
			rval = 1;
			break;
		}

	/* dump domains and aliases */
	/* sort hosts (only) in table */
	i = 0;
	for (child = n->child; child; child = child->next)
		table[i++] = child;
	qsort((char *) table, i, sizeof(table[0]), qcmp);

	/* cost is DEDICATED for hosts in top-level domains, LOCAL o.w. */
	if (n->parent == Top && strchr(n->name + 1, '.') == 0)
		cost = "DEDICATED";
	else
		cost = "LOCAL";
	formatprint(f, table, i, HOSTS, n->name, cost);

	formatprint(f, table, i, SUBDOMAINS, n->name, "0");

	/* dump aliases */
	nelem = i;
	for (i = 0; i < nelem; i++) {
		if ((alias = table[i]->alias) == 0)
			continue;
		fprintf(f, "%s = %s", table[i]->name, alias->name);
		for (alias = alias->next; alias; alias = alias->next)
			fprintf(f, ", %s", alias->name);
		putc('\n', f);
	}

	return rval;
}

/* for debugging */
dtable(comment, table, nelem)
	char *comment;
	node **table;
{	int	i;

	fprintf(stderr, "\n%s\n", comment);
	for (i = 0; i < nelem; i++)
		fprintf(stderr, "%3d\t%s\n", i, table[i]->name);
}

formatprint(f, table, nelem, type, lhs, cost)
	FILE *f;
	node **table;
	char *lhs, *cost;
{	int i, didprint;
	char buf[128], *bptr;

	sprintf(buf, "%s%s{" /*}*/, lhs, type == PRIVATE ? " " : " = ");
	bptr = buf + strlen(buf);

	didprint = 0;
	for (i = 0; i < nelem; i++) {
		if (type == PRIVATE && ! (table[i]->flag & COLLISION))
			continue;
		else if (type == HOSTS && ISADOMAIN(table[i]) )
			continue;
		else if (type == SUBDOMAINS && ! ISADOMAIN(table[i]) )
			continue;

		if ((bptr - buf) + strlen(table[i]->name) > 69) {
			*bptr = 0;
			fprintf(f, "%s\n ", buf);	/* continuation */
			bptr = buf;
		}
		sprintf(bptr, "%s, ", table[i]->name);
		bptr += strlen(bptr);
		didprint++;
	}
	*bptr = 0;

	if ( ! didprint )
		return;

	fprintf(f, /*{*/ "%s}", buf);
	if (type != PRIVATE)
		fprintf(f, "(%s)", cost);
	putc('\n', f);
}

FILE *				
mkfile(n)
	node *n;
{	node *parent;
	char *bptr;
	FILE *f;

	/* build up the domain name in Fname[] */
	bptr = Fstart;
	if (n == Top)
		strcpy(bptr, n->name);
	else {
		strcpy(bptr, n->name + 1);	/* skip leading dot */
		bptr = bptr + strlen(bptr);
		parent = n->parent;
		while (ISADOMAIN(parent)) {
			strcpy(bptr, parent->name);
			bptr += strlen(bptr);
			parent = parent->parent;
		}
		*bptr = 0;
	}

	/* get a stream descriptor */
	if (Fflag) {
		printf("# %s\n", Fstart);
		f = stdout;
	} else {
#ifndef BSD
		Fstart[14] = 0;
#endif
		if ((f = fopen(Fname, "w")) == 0) {
			perror(Fname);
			exit(1);
		}
	}
	if (n == Top)
		fprintf(f, "private {%s}\ndead {%s}\n", Top->name, Top->name);
	return f;
}

/* map to lower case in place.  return parameter for convenience */
char *
lowercase(buf)
	char *buf;
{	char *str;

	for (str = buf ; *str; str++)
		if (isupper(*str))
			*str -= 'A' - 'a';
	return buf;
}

/* get the interesting aliases, attach to n->alias */
makealiases(n, line)
	node *n;
	char *line;
{	char *next, *dot;
	node *a;

	if (line == 0 || *line == 0)
		return;

	for ( ; line; line = next) {
		next = strchr(line, ',');
		if (next)
			*next++ = 0;
		if ((dot = strchr(line, '.')) == 0)
			continue;
		if (strcmp(dot, DotArpa) != 0)
			continue;
		*dot = 0;

		if (strcmp(n->name, line) == 0)
			continue;

		a = newnode();
		a->name = strsave(line);
		a->next = n->alias;
		n->alias = a;
	}
}

#define NHASH 13309
node *htable[NHASH];

merge()
{	node *n;

	setuniqflag(Top);
	for (n = Top->child; n; n = n->next) {
		if (n->flag & COLLISION) {
			fprintf(stderr, "illegal subdomain: %s\n", n->name);
			abort();
		}
		promote(n);
	}
}

/*
 * for domains like cc.umich.edu and cc.berkeley.edu, it's inaccurate
 * to describe cc as a member of umich.edu or berkeley.edu.  (i.e., the
 * lousy scoping rules for privates don't permit a clean syntax.)  so.
 *
 * to prevent confusion, tack on to any such domain name its parent domain
 * and promote it in the tree.  e.g., make cc.umich and cc.berkeley
 * subdomains of edu.
 */

promote(parent)
	node *parent;
{	node *prev, *child, *next;
	char buf[BUFSIZ];

	prev = 0;
	for (child = parent->child; child; child = next) {
		next = child->next;
		if (!ISADOMAIN(child)) {
			prev = child;
			continue;
		}
		if ((child->flag & COLLISION) || child->gateway) {
			/*
			 * dup domain name or gateway found.  don't bump
			 * prev: this node is moving up the tree.
			 */

			/* qualify child domain name */
			sprintf(buf, "%s%s", child->name, parent->name);
			cfree(child->name);
			child->name = strsave(buf);

			/* unlink child out of sibling chain */
			if (prev)
				prev->next = child->next;
			else
				parent->child = child->next;

			/* link child in as peer of parent */
			child->next = parent->next;
			parent->next = child;
			child->parent = parent->parent;

			/*
			 * reset collision flag; may promote again on
			 * return to caller.
			 */
			child->flag &= ~COLLISION;
			hash(child);
		} else {
			/* this domain is uninteresting (so bump prev) */
			promote(child);
			prev = child;
		}
	}
	
}

setuniqflag(n)
	node *n;
{	node *child, *alias;

	/* mark this node in the hash table */
	hash(n);
	/* mark the aliases of this node */
	for (alias = n->alias; alias; alias = alias->next)
		hash(alias);
	/* recursively mark this node's children */
	for (child = n->child; child; child = child->next)
		setuniqflag(child);
}

hash(n)
	node *n;
{	node **bucket, *b;

	bucket = &htable[fold(n->name) % NHASH];
	for (b = *bucket; b; b = b->bucket) {
		if (strcmp(n->name, b->name) == 0) {
			b->flag |= COLLISION;
			n->flag |= COLLISION;
			return;
		}
	}
	n->bucket = *bucket;
	*bucket = n;
}

/* stolen from pathalias:addnode.c, q.v. */
#define POLY	0x48000000		/* 31-bit polynomial */
long CrcTable[128];

void
crcinit()
{	register int i,j;
	register long sum;

	for (i = 0; i < 128; i++) {
		sum = 0;
		for (j = 7-1; j >= 0; --j)
			if (i & (1 << j))
				sum ^= POLY >> j;
		CrcTable[i] = sum;
	}
}

long
fold(s)
	register char *s;
{	register long sum = 0;
	register int c;

	while (c = *s++)
		sum = (sum >> 7) ^ CrcTable[(sum ^ c) & 0x7f];
	return sum;
}
