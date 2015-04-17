/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)mapit.c	9.1 87/10/04";
#endif

#include "def.h"

#define chkheap(i)	/* void */

/* exports */
/* invariant while mapping: Nheap < Hashpart */
long Hashpart;		/* start of unreached nodes */
long Nheap;		/* end of heap */

void mapit();

/* imports */
extern long Nheap, Hashpart, Tabsize;
extern int Tflag, Vflag;
extern p_node *Table;
extern p_node Home;
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
extern node *getnode();
extern link *getlink();
#endif /*TMPFILES*/
extern char *Linkout, *Graphout;

extern void freelink(), resetnodes(), printit(), dumpgraph();
extern void showlinks(), die();
extern long pack(), allocation();
extern p_link newlink();
extern p_link addlink();
extern int maptrace();
extern p_node ncopy();


/* privates */
static long	Heaphighwater;
static p_link	*Heap;

STATIC void insert(), heapup(), heapdown(), heapswap(), backlinks();
STATIC void setheapbits(), mtracereport(), heapchildren();
STATIC p_link min_node();
STATIC int dehash(), skiplink(), skipterminalalias();
STATIC Cost costof();

/* transform the graph to a shortest-path tree by marking tree edges */
void
mapit()
{	register p_node n;
	register p_link l;
	p_link lparent;
	static int firsttime = 0;

	vprintf(stderr, "*** mapping\n");
	Tflag = Tflag && Vflag;		/* tracing here only if verbose */
	/* re-use the hash table space for the heap */
	Heap = (p_link *) Table;
	Hashpart = pack(0L, Tabsize - 1);

	/* expunge penalties from -a option and make circular copy lists */
	resetnodes();

	if (firsttime++) {
		if (Linkout && *Linkout)	/* dump cheapest links */
			showlinks();
		if (Graphout && *Graphout)	/* dump the edge list */
			dumpgraph();
	}

	/* insert Home to get things started */
	l = newlink();		/* link to get things started */
	getlink(l)->l_to = Home;
	(void) dehash(Home);
	insert(l);

	/* main mapping loop */
remap:
	Heaphighwater = Nheap;
	while ((lparent = min_node()) != 0) {
		chkheap(1);
		getlink(lparent)->l_flag |= LTREE;
		n = getlink(lparent)->l_to;
		if (Tflag && maptrace(n, n))
			fprintf(stderr, "%s -> %s mapped\n",
				getnode(getnode(n)->n_parent)->n_name,
				getnode(n)->n_name);
		if (getnode(n)->n_flag & MAPPED)
			die("mapped node in heap");
		getnode(n)->n_flag |= MAPPED;

		/* add children to heap */
		heapchildren(n);
	}
	vprintf(stderr, "heap high water mark was %ld\n", Heaphighwater);

	/* sanity check on implementation */
	if (Nheap != 0)
		die("null entry in heap");

	if (Hashpart < Tabsize) {
		/*
		 * add back links from unreachable hosts to
		 * reachable neighbors, then remap.
		 *
		 * asymptotically, this is quadratic; in
		 * practice, this is done once or twice.
		 */
		backlinks();
		if (Nheap)
			goto remap;
	}
	if (Hashpart < Tabsize) {
		fputs("You can't get there from here:\n", stderr);
		for ( ; Hashpart < Tabsize; Hashpart++) {
			fprintf(stderr, "\t%s", getnode(Table[Hashpart])->n_name);
			if (getnode(Table[Hashpart])->n_flag & ISPRIVATE)
				fputs(" (private)", stderr);
			putc('\n', stderr);
		}
	}
}

STATIC void
heapchildren(n)
	register p_node n;
{	register p_link l;
	register p_node next;
	register int mtrace;
	register Cost cost;

	for (l = getnode(n)->n_link; l; l = getlink(l)->l_next) {

		next = getlink(l)->l_to;	/* neighboring node */
		mtrace = Tflag && maptrace(n, next);

		if (getnode(next)->n_flag & MAPPED) {
			if (mtrace)
				mtracereport(n, l, "-\talready mapped");
			continue;
		}
		if (getlink(l)->l_flag & LTREE)
			continue;

		if (getlink(l)->l_flag & LTERMINAL) {
			next = ncopy(next);
			getlink(l)->l_to = next;
		}

		if ((getnode(n)->n_flag & NTERMINAL)
		     && (getlink(l)->l_flag & LALIAS))
			if (skipterminalalias(n, next))
				continue;
			else {
				next = ncopy(next);
				getlink(l)->l_to = next;
			}

		cost = costof(n, l);

		if (skiplink(l, n, cost, mtrace))
			continue;

		/*
		 * put this link in the heap and restore the
		 * heap property.
		 */
		if (mtrace) {
			if (getnode(next)->n_parent)
				mtracereport(getnode(next)->n_parent, l, "*\tdrop");
			mtracereport(n, l, "+\tadd");
		}
		getnode(next)->n_parent = n;
		if (dehash(next) == 0) {  /* first time */
			getnode(next)->n_cost = cost;
			insert(l);	  /* insert at end */
			heapup(l);
		} else {
			/* replace inferior path */
			Heap[getnode(next)->n_tloc] = l;
			if (cost > getnode(next)->n_cost) {
				/* increase cost (gateway) */
				getnode(next)->n_cost = cost;
				heapdown(l);
			} else if (cost < getnode(next)->n_cost) {
				/* cheaper route */
				getnode(next)->n_cost = cost;

				heapup(l);
			}
		}
		setheapbits(l);
		chkheap(1);

	}
}

/* bizarro special case */
STATIC
skipterminalalias(n, next)
	p_node n;
	p_node next;
{	p_node ncp;

	while (getnode(n)->n_flag & NALIAS) {
		n = getnode(n)->n_parent;
		for (ncp = getnode(n)->n_copy; ncp != n; ncp = getnode(ncp)->n_copy)
			if (next == ncp)
				return 1;
	}
	return 0;
}

/*
 * return 1 if we definitely don't want want this link in the
 * shortest path tree, 0 if we might want it, i.e., best so far.
 *
 * if tracing is turned on, report only if this node is being skipped.
 */
STATIC int
skiplink(l, parent, cost, trace)
	p_link l;		/* new link to this node */
	p_node parent;		/* (potential) new parent of this node */
	register Cost cost;	/* new cost to this node */
	int trace;		/* trace this link? */
{	register p_node n;	/* this node */
	register p_link lheap;		/* old link to this node */

	n = getlink(l)->l_to;

	/* first time we've reached this node? */
	if (getnode(n)->n_tloc >= Hashpart)
		return 0;

	lheap = Heap[getnode(n)->n_tloc];

	/* examine links to nets that require gateways */
	if (GATEWAYED(getnode(n))) {
		/* if exactly one is a gateway, use it */
		if ((getlink(lheap)->l_flag & LGATEWAY)
		     && !(getlink(l)->l_flag & LGATEWAY)) {
			if (trace)
				mtracereport(parent, l, "-\told gateway");
			return 1;	/* old is gateway */
		}
		if (!(getlink(lheap)->l_flag & LGATEWAY)
		      && (getlink(l)->l_flag & LGATEWAY))
			return 0;	/* new is gateway */

		/* no gateway or both gateways;  resolve in standard way ... */
	}

	/* examine dup link (sanity check) */
	if (getnode(n)->n_parent == parent && ((getlink(lheap)->l_flag & LDEAD)
	    || (getlink(l)->l_flag & LDEAD)))
		die("dup dead link");

	/*  examine cost */
	if (cost < getnode(n)->n_cost)
		return 0;
	if (cost > getnode(n)->n_cost) {
		if (trace)
			mtracereport(parent, l, "-\tcheaper");
		return 1;
	}

	/* all other things being equal, ask the oracle */
	if (tiebreaker(n, parent)) {
		if (trace)
			mtracereport(parent, l, "-\ttiebreaker");
		return 1;
	}
	return 0;
}

STATIC Cost
costof(prev, l)
	register p_node prev;
	register p_link l;
{	register p_node next;
	register Cost cost;

	if (getlink(l)->l_flag & LALIAS)
		return getnode(prev)->n_cost;	/* by definition */

	next = getlink(l)->l_to;
	cost = getnode(prev)->n_cost + getlink(l)->l_cost; /* basic cost */

	/*
	 * heuristics:
	 *    charge for a dead link.
	 *    charge for getting past a terminal edge.
	 *    charge for getting out of a dead host.
	 *    charge for getting into a gatewayed net (except at a gateway).
	 *    discourage mixing of left and right syntax when prev is a host.
	 *
	 * life was simpler when pathalias computed true shortest paths.
	 */
	if (getlink(l)->l_flag & LDEAD)			/* dead link */
		cost += INF;
	if (getnode(prev)->n_flag & NTERMINAL)		/* terminal parent */
		cost += INF;
	if (DEADHOST(getnode(prev)))			/* dead host */
		cost += INF;
	if (GATEWAYED(getnode(next)) && !(getlink(l)->l_flag & LGATEWAY))
		cost += INF;				/* not gateway */
	if (!ISANET(getnode(prev))) {			/* mixed syntax? */
		if ((NETDIR(getlink(l)) == LLEFT
		     && (getnode(prev)->n_flag & HASRIGHT))
		 || (NETDIR(getlink(l)) == LRIGHT
		     && (getnode(prev)->n_flag & HASLEFT)))
			cost += INF;
	}

	return cost;
}

/* binary heap implementation of priority queue */

STATIC void
insert(l)
	p_link l;
{	register p_node n;

	n = getlink(l)->l_to;
	if (getnode(n)->n_flag & MAPPED)
		die("insert mapped node");

	Heap[getnode(n)->n_tloc] = 0;
	if (Heap[Nheap+1] != 0)
		die("heap error in insert");
	if (Nheap++ == 0) {
		Heap[1] = l;
		getnode(n)->n_tloc = 1;
		return;
	}
	if (Vflag && Nheap > Heaphighwater)
		Heaphighwater = Nheap;	/* diagnostics */

	/* insert at the end.  caller must heapup(l). */
	Heap[Nheap] = l;
	getnode(n)->n_tloc = Nheap;
}

/*
 * "percolate" up the heap by exchanging with the parent.  as in
 * min_node(), give tiebreaker() a chance to produce better, stable
 * routes by moving nets and domains close to the root, nets closer
 * than domains.
 *
 * i know this seems obscure, but it's harmless and cheap.  trust me.
 */
STATIC void
heapup(l)
	p_link l;
{	register long cindx, pindx;	/* child, parent indices */
	register Cost cost;
	register p_node child;
	register p_node parent;

	child = getlink(l)->l_to;

	cost = getnode(child)->n_cost;
	for (cindx = getnode(child)->n_tloc; cindx > 1; cindx = pindx) {
		pindx = cindx / 2;
		if (Heap[pindx] == 0)	/* sanity check */
			die("impossible error in heapup");
		parent = getlink(Heap[pindx])->l_to;
		if (cost > getnode(parent)->n_cost)
			return;

		/* net/domain heuristic */
		if (cost == getnode(parent)->n_cost) {
			if (!ISANET(getnode(child)))
				return;
			if (!ISADOMAIN(getnode(parent)))
				return;
			if (ISADOMAIN(getnode(child)))
				return;
		}
		heapswap(cindx, pindx);
	}
}

/* extract min (== Heap[1]) from heap */
STATIC p_link
min_node()
{	p_link rval;
	p_link lastlink;
	register p_link *rheap;

	if (Nheap == 0)
		return 0;

	rheap = Heap;		/* in register -- heavily used */
	rval = rheap[1];	/* return this one */

	/* move last entry into root and reheap */
	lastlink = rheap[Nheap];
	rheap[Nheap] = 0;

	if (--Nheap) {
		rheap[1] = lastlink;
		getnode(getlink(lastlink)->l_to)->n_tloc = 1;
		heapdown(lastlink);	/* restore heap property */
	}

	return rval;
}

/*
 * swap Heap[i] with smaller child, iteratively down the tree.
 *
 * given the opportunity, attempt to place nets and domains
 * near the root.  this helps tiebreaker() shun domain routes.
 */

STATIC void
heapdown(l)
	p_link l;
{	register long pindx, cindx;
	register p_link *rheap = Heap;	/* in register -- heavily used */
	p_node child;
	p_node rchild;
	p_node parent;

	pindx = getnode(getlink(l)->l_to)->n_tloc;
	parent = getlink(rheap[pindx])->l_to;	/* invariant */
	for ( ; (cindx = pindx * 2) <= Nheap; pindx = cindx) {
		/* pick lhs or rhs child */
		child = getlink(rheap[cindx])->l_to;
		if (cindx < Nheap) {
			/* compare with rhs child */
			rchild = getlink(rheap[cindx+1])->l_to;
			/*
			 * use rhs child if smaller than lhs child.
			 * if equal, use rhs if net or domain.
			 */
			if (getnode(child)->n_cost > getnode(rchild)->n_cost) {
				child = rchild;
				cindx++;
			} else if (getnode(child)->n_cost == getnode(rchild)->n_cost)
				if (ISANET(getnode(rchild))) {
					child = rchild;
					cindx++;
				}
		}

		/* child is the candidate for swapping */
		if (getnode(parent)->n_cost < getnode(child)->n_cost)
			break;

		/*
		 * heuristics:
		 *	move nets/domains up
		 *	move nets above domains
		 */
		if (getnode(parent)->n_cost == getnode(child)->n_cost) {
			if (!ISANET(getnode(child)))
				break;
			if (ISANET(getnode(parent)) && ISADOMAIN(getnode(child)))
				break;
		}

		heapswap(pindx, cindx);
	}
}

/* exchange Heap[i] and Heap[j] pointers */
STATIC void
heapswap(i, j)
	long i, j;
{	register p_link temp;
	register p_link *rheap;

	rheap = Heap;	/* heavily used -- put in register */
	temp = rheap[i];
	rheap[i] = rheap[j];
	rheap[j] = temp;
	getnode(getlink(rheap[j])->l_to)->n_tloc = j;
	getnode(getlink(rheap[i])->l_to)->n_tloc = i;
}

/* return 1 if n is already de-hashed (n_tloc < Hashpart), 0 o.w. */
STATIC int
dehash(n)
	register p_node n;
{
	if (getnode(n)->n_tloc < Hashpart)
		return 1;

	/* swap with entry in Table[Hashpart] */
	getnode(Table[Hashpart])->n_tloc = getnode(n)->n_tloc;
	Table[getnode(n)->n_tloc] = Table[Hashpart];
	Table[Hashpart] = n;

	getnode(n)->n_tloc = Hashpart++;
	return 0;
}

STATIC void
backlinks()
{	register p_link l;
	register p_node n;
	register p_node parent;
	p_node nomap;
	p_node ncp;
	long i;

	for (i = Hashpart; i < Tabsize; i++) {
		nomap = Table[i];
		for (ncp = getnode(nomap)->n_copy; ncp != nomap;
		     ncp = getnode(ncp)->n_copy) {
			if (getnode(ncp)->n_flag & MAPPED) {
				(void) dehash(nomap);
				Table[getnode(nomap)->n_tloc] = 0;
				getnode(nomap)->n_tloc = 0;
				goto nexti;
			}
		}

		/* TODO: simplify this */		
		parent = 0;
		for (l = getnode(nomap)->n_link; l; l = getlink(l)->l_next) {
			n = getlink(l)->l_to;
			if (!(getnode(n)->n_flag & MAPPED))
				continue;
			if (parent == 0) {
				parent = n;
				continue;
			}
			if (getnode(n)->n_cost > getnode(parent)->n_cost)
				continue;
			if (getnode(n)->n_cost == getnode(parent)->n_cost) {
				getnode(nomap)->n_parent = parent;
				if (tiebreaker(nomap, n))
					continue;
			}
			parent = n;
		}
		if (parent == 0)
			continue;
		(void) dehash(nomap);
		l = addlink(parent, nomap, INF, DEFNET, DEFDIR);
		getnode(nomap)->n_parent = parent;
		getnode(nomap)->n_cost = costof(parent, l);
		insert(l);
		heapup(l);
nexti:
		;
	}
	vprintf(stderr, "%ld backlinks\n", Nheap);
}

STATIC void
setheapbits(l)
	register p_link l;
{	register p_node n;
	register p_node parent;

	n = getlink(l)->l_to;
	parent = getnode(n)->n_parent;
	getnode(n)->n_flag &= ~(NALIAS|HASLEFT|HASRIGHT);	/* reset */

	/* record whether link is an alias */
	if (getlink(l)->l_flag & LALIAS) {
		getnode(n)->n_flag |= NALIAS;
		if (getnode(parent)->n_flag & NTERMINAL)
			getnode(n)->n_flag |= NTERMINAL;
	}

	/* set left/right bits */
	if (NETDIR(getlink(l)) == LLEFT || (getnode(parent)->n_flag & HASLEFT))
		getnode(n)->n_flag |= HASLEFT;
	if (NETDIR(getlink(l)) == LRIGHT || (getnode(parent)->n_flag & HASRIGHT))
		getnode(n)->n_flag |= HASRIGHT;
}

STATIC void
mtracereport(from, l, excuse)
	p_node from;
	p_link l;
	char *excuse;
{	p_node to = getlink(l)->l_to;

	fprintf(stderr, "%-16s ", excuse);
	fputs(getnode(from)->n_name, stderr);
	if (getnode(from)->n_flag & NTERMINAL)
		putc('\'', stderr);
	fputs(" -> ", stderr);
	fputs(getnode(to)->n_name, stderr);
	if (getnode(to)->n_flag & NTERMINAL)
		putc('\'', stderr);
	fprintf(stderr, " (%ld, %ld, %ld) ", getnode(from)->n_cost,
		getlink(l)->l_cost, getnode(to)->n_cost);
	if (getnode(to)->n_parent) {
		fputs(getnode(getnode(to)->n_parent)->n_name, stderr);
		if (getnode(getnode(to)->n_parent)->n_flag & NTERMINAL)
			putc('\'', stderr);
		fprintf(stderr, " (%ld)", getnode(getnode(to)->n_parent)->n_cost);
	}
	putc('\n', stderr);
}
	
#if 0
chkheap(i)
{	int lhs, rhs;

	lhs = i * 2;
	rhs = lhs + 1;

	if (lhs <= Nheap) {
		if (getnode(getlink(Heap[i])->l_to)->n_cost
		    > getnode(getlink(Heap[lhs])->l_to)->n_cost)
			die("chkheap failure on lhs");
		chkheap(lhs);
	}
	if (rhs <= Nheap) {
		if (getnode(getlink(Heap[i])->l_to)->n_cost
		     > getnode(getlink(Heap[rhs])->l_to)->n_cost)
			die("chkheap failure on rhs");
		chkheap(rhs);
	}
#if 00
	for (i = 1; i < Nheap; i++) {
		p_link l;

		vprintf(stderr, "%5d %-16s", i, getnode(getlink(Heap[i])->l_to)->n_name);
		if ((l = getnode(getlink(Heap[i])->l_to)->n_link) != 0) do {
			vprintf(stderr, "%-16s", getnode(getlink(l)->l_to)->n_name);
		} while ((l = getlink(l)->l_next) != 0);
		vprintf(stderr, "\n");
	}
	for (i = Hashpart; i < Tabsize; i++) {
		p_link l;
		p_node n;

		vprintf(stderr, "%5d %-16s", i, getnode(Table[i])->n_name);
		if ((l = getnode(Table[i])->n_link) != 0) do {
			vprintf(stderr, "%-16s", getnode(getlink(l)->l_to)->n_name);
		} while ((l = getlink(l)->l_next) != 0);
		vprintf(stderr, "\n");
	}
#endif /*00*/
		
}
#endif /*0*/
