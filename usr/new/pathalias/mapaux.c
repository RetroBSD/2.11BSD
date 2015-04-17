/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)mapaux.c	9.1 87/10/04";
#endif /* lint */

#include "def.h"

/* imports */
extern long Nheap, Hashpart, Tabsize;
extern p_node *Table;
extern p_node Home;
extern char *Graphout, *Linkout, *Netchars, **Argv;
extern void freelink(), die();
extern long pack();
extern p_link newlink();
extern p_node newnode();
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
extern node *getnode();
extern link *getlink();
#endif /*TMPFILES*/
extern char *strsave();

/* exports */
long pack();
void resetnodes(), dumpgraph(), showlinks();
int tiebreaker();
p_node ncopy();

/* privates */
static FILE *Gstream;	/* for dumping graph */
STATIC void dumpnode(), untangle(), dfs();
STATIC int height();
STATIC p_link lcopy();

/*
 * slide everything from Table[low] to Table[high]
 * up toward the high end.  make room!  make room!
 */
long
pack(low, high)
	long low, high;
{	long hole, next;

	/* find first "hole " */
	for (hole = high; hole >= low && Table[hole] != 0; --hole)
		;

	/* repeatedly move next filled entry into last hole */
	for (next = hole - 1; next >= low; --next) {
		if (Table[next] != 0) {
			Table[hole] = Table[next];
			getnode(Table[hole])->n_tloc = hole;
			Table[next] = 0;
			while (Table[--hole] != 0)	/* find next hole */
				;
		}
	}
	return hole + 1;
}

void
resetnodes()
{	register long i;
	register p_node n;

	for (i = Hashpart; i < Tabsize; i++)
		if ((n = Table[i]) != 0) {
			getnode(n)->n_cost = (Cost) 0;
			getnode(n)->n_flag &= ~(NALIAS|ATSIGN|MAPPED|HASLEFT|HASRIGHT|NTERMINAL);
			getnode(n)->n_copy = n;
		}
		
	getnode(Home)->n_cost = (Cost) 0;
	getnode(Home)->n_flag &= ~(NALIAS|ATSIGN|MAPPED|HASLEFT|HASRIGHT|NTERMINAL);
	getnode(Home)->n_copy = Home;
}

void	
dumpgraph()
{	register long i;
	register p_node n;

	if ((Gstream = fopen(Graphout, "w")) == NULL) {
		fprintf(stderr, "%s: ", Argv[0]);
		perror(Graphout);
		return;
	}

	untangle();	/* untangle net cycles for proper output */

	for (i = Hashpart; i < Tabsize; i++) {
		n = Table[i];
		if (n == 0)
			continue;	/* impossible, but ... */
		/* a minor optimization ... */
		if (getnode(n)->n_link == 0)
			continue;
		/* pathparse doesn't need these */
		if (getnode(n)->n_flag & NNET)
			continue;
		dumpnode(n);
	}
}

STATIC void
dumpnode(from)
	register p_node from;
{	register p_node to;
	register p_link l;
	p_link lnet = 0;
	p_link ll;
	p_link lnext;

	for (l = getnode(from)->n_link ; l; l = getlink(l)->l_next) {
		to = getlink(l)->l_to;
		if (from == to)
			continue;	/* oops -- it's me! */

		if ((getnode(to)->n_flag & NNET) == 0) {
			/* host -> host -- print host>host */
			if (getlink(l)->l_cost == INF)
				continue;	/* phoney link */
			fputs(getnode(from)->n_name, Gstream);
			putc('>', Gstream);
			fputs(getnode(to)->n_name, Gstream);
			putc('\n', Gstream);
		} else {
			/*
			 * host -> net -- just cache it for now.
			 * first check for dups.  (quadratic, but
			 * n is small here.)
			 */
			while (getnode(to)->n_root && to != getnode(to)->n_root)
				to = getnode(to)->n_root;
			for (ll = lnet; ll; ll = getlink(ll)->l_next)
				if (strcmp(getnode(getlink(ll)->l_to)->n_name,
					   getnode(to)->n_name) == 0)
					break;
			if (ll)
				continue;	/* dup */
			ll = newlink();
			getlink(ll)->l_next = lnet;
			getlink(ll)->l_to = to;
			lnet = ll;
		}
	}

	/* dump nets */
	if (lnet) {
		/* nets -- print host@\tnet,net, ... */
		fputs(getnode(from)->n_name, Gstream);
		putc('@', Gstream);
		putc('\t', Gstream);
		for (ll = lnet; ll; ll = lnext) {
			lnext = getlink(ll)->l_next;
			fputs(getnode(getlink(ll)->l_to)->n_name, Gstream);
			if (lnext)
				fputc(',', Gstream);
			freelink(ll);
		}
		putc('\n', Gstream);
	}
}

/*
 * remove cycles in net definitions. 
 *
 * depth-first search
 *
 * for each net, run dfs on its neighbors (nets only).  if we return to
 * a visited node, that's a net cycle.  mark the cycle with a pointer
 * in the n_root field (which gets us closer to the root of this
 * portion of the dfs tree).
 */
STATIC void
untangle()
{	register long i;
	register p_node n;

	for (i = Hashpart; i < Tabsize; i++) {
		n = Table[i];
		if (n == 0 || (getnode(n)->n_flag & NNET) == 0
		    || getnode(n)->n_root)
			continue;
		dfs(n);
	}
}

STATIC void
dfs(n)
	register p_node n;
{	register p_link l;
	register p_node next;

	getnode(n)->n_flag |= INDFS;
	getnode(n)->n_root = n;
	for (l = getnode(n)->n_link; l; l = getlink(l)->l_next) {
		next = getlink(l)->l_to;
		if ((getnode(next)->n_flag & NNET) == 0)
			continue;
		if ((getnode(next)->n_flag & INDFS) == 0) {
			dfs(next);
			if (getnode(next)->n_root != next)
				getnode(n)->n_root = getnode(next)->n_root;
		} else
			getnode(n)->n_root = getnode(next)->n_root;
	}
	getnode(n)->n_flag &= ~INDFS;
}

void
showlinks() 
{	register p_link l;
	register p_node n;
	register long i;
	FILE	*estream;

	if ((estream = fopen(Linkout, "w")) == 0)
		return;

	for (i = Hashpart; i < Tabsize; i++) {
		n = Table[i];
		if (n == 0 || getnode(n)->n_link == 0)
			continue;
		for (l = getnode(n)->n_link; l; l = getlink(l)->l_next) {
			fputs(getnode(n)->n_name, estream);
			putc('\t', estream);
			if (NETDIR(getlink(l)) == LRIGHT)
				putc(NETCHAR(getlink(l)), estream);
			fputs(getnode(getlink(l)->l_to)->n_name, estream);
			if (NETDIR(getlink(l)) == LLEFT)
				putc(NETCHAR(getlink(l)), estream);
			fprintf(estream, "(%ld)\n", getlink(l)->l_cost);
		}
	}
	(void) fclose(estream);
}

/*
 * n is node in heap, newp is candidate for new parent.
 * choose between newp and n->n_parent for parent.
 * return 0 to use newp, non-zero o.w.
 */
#define NEWP 0
#define OLDP 1
int
tiebreaker(n, newp)
	p_node n;
	register p_node newp;
{	register char *opname, *npname, *name;
	register p_node oldp;
	int metric;

	oldp = getnode(n)->n_parent;

	/* given the choice, avoid gatewayed nets */
	if (GATEWAYED(getnode(newp)) && !GATEWAYED(getnode(oldp)))
		return OLDP;
	if (!GATEWAYED(getnode(newp)) && GATEWAYED(getnode(oldp)))
		return NEWP;

	/* look at real parents, not nets */
	while ((getnode(oldp)->n_flag & NNET) && getnode(oldp)->n_parent)
		oldp = getnode(oldp)->n_parent;
	while ((getnode(newp)->n_flag & NNET) && getnode(newp)->n_parent)
		newp = getnode(newp)->n_parent;

	/* use fewer hops, if possible */
	metric = height(oldp) - height(newp);
	if (metric < 0)
		return OLDP;
	if (metric > 0)
		return NEWP;

	/*
	 * compare names
	 */
	opname = getnode(oldp)->n_name;
	npname = getnode(newp)->n_name;
	name = getnode(n)->n_name;

	/* use longest common prefix with parent */
	while (*opname == *name && *npname == *name && *name) {
		opname++;
		npname++;
		name++;
	}
	if (*opname == *name)
		return OLDP;
	if (*npname == *name)
		return NEWP;

	/* use shorter host name */
	metric = strlen(opname) - strlen(npname);
	if (metric < 0)
		return OLDP;
	if (metric > 0)
		return NEWP;

	/* use larger lexicographically */
	metric = strcmp(opname, npname);
	if (metric < 0)
		return NEWP;
	return OLDP;
}

STATIC int
height(n)
	register p_node n;
{	register int i = 0;

	if (n == 0)
		return 0;
	while ((n = getnode(n)->n_parent) != 0)
		if (ISADOMAIN(getnode(n)) || !(getnode(n)->n_flag & NNET))
			i++;
	return i;
}
	
/* l is a terminal edge from n -> next; return a copy of next */
p_node
ncopy(n)
	register p_node n;
{	register p_node ncp;
	register p_link dummy;

	ncp = newnode();
#ifndef TMPFILES
	getnode(ncp)->n_name = getnode(n)->n_name;	/* nonvolatile */
#else /*TMPFILES*/	    /* It's not very nonvolatile in the cache! */
	getnode(ncp)->n_name = strsave(getnode(n)->n_name);
#endif /*TMPFILES*/
	getnode(ncp)->n_tloc = --Hashpart; /* better not be > 20% of total ... */
	if (Hashpart == Nheap)
		die("too many terminal links");
	Table[Hashpart] = ncp;
	getnode(ncp)->n_copy = getnode(n)->n_copy;	/* circular list */
	getnode(n)->n_copy = ncp;
	dummy = lcopy(getnode(n)->n_link);
	getnode(ncp)->n_link = dummy;
	getnode(ncp)->n_flag = (getnode(n)->n_flag & ~(NALIAS|ATSIGN|MAPPED|HASLEFT|HASRIGHT)) | NTERMINAL;
	return ncp;
}

STATIC p_link
lcopy(l)
	register p_link l;
{	register p_link lcp;
	register p_link ltp;

	if (l == 0)
		return 0;
	lcp = newlink();
#ifndef TMPFILES
	*getlink(lcp) = *getlink(l);
#else /*TMPFILES*/	/* inefficient, but we must preserve l_seq */
	getlink(lcp)->l_to = getlink(l)->l_to;
	getlink(lcp)->l_cost = getlink(l)->l_cost;
	getlink(lcp)->l_from = getlink(l)->l_from;
	getlink(lcp)->l_flag = getlink(l)->l_flag;
	getlink(lcp)->l_netop = getlink(l)->l_netop;
#endif /*TMPFILES*/
	getlink(lcp)->l_flag &= ~LTREE;
#ifndef TMPFILES
	getlink(lcp)->l_next = lcopy(getlink(l)->l_next);
#else
	/*
	 * we could avoid recursive cache flushing by breaking
	 * the above statement into two assignments, but the
	 * recursion overflows the stack for the giant att link chain.
	 */
	ltp = lcp;
	while ((l = getlink(l)->l_next)) {
		ltp = getlink(ltp)->l_next = newlink();
		getlink(ltp)->l_to = getlink(l)->l_to;
		getlink(ltp)->l_cost = getlink(l)->l_cost;
		getlink(ltp)->l_from = getlink(l)->l_from;
		getlink(ltp)->l_flag = getlink(l)->l_flag;
		getlink(ltp)->l_netop = getlink(l)->l_netop;
		getlink(ltp)->l_flag &= ~LTREE;
	}
#endif
	return lcp;
}
