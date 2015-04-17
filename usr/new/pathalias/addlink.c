/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)addlink.c	9.1 87/10/04";
#endif /* lint */

#include "def.h"

/* exports */
extern p_link addlink();
extern void deadlink(), atrace();
extern int tracelink();
char *Netchars = "!:@%";	/* sparse, but sufficient */
long Lcount;			/* how many edges? */


/* imports */
extern int Tflag, Dflag;
extern void freelink();
extern p_link newlink();
extern p_node addnode();
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
extern node *getnode();
extern link *getlink();
#endif /*TMPFILES*/
extern void yyerror(), die();

/* privates */
STATIC void netbits(), ltrace(), ltrprint();
static p_link	Trace[NTRACE];
static int	Tracecount;

p_link
addlink(from, to, cost, netchar, netdir)
	p_node from;
	register p_node to;
	Cost cost;
	char netchar, netdir;
{	register p_link l;
	register p_link prev = 0;

	if (Tflag)
		ltrace(from, to, cost, netchar, netdir);
	/*
	 * maintain uniqueness for dead links (only).
	 */
	for (l = getnode(from)->n_link; l; l = getlink(l)->l_next) {
		if (!(getlink(l)->l_flag & LDEAD))
			break;
		if (to == getlink(l)->l_to) {
			/* what the hell, use cheaper dead cost */
			if (cost < getlink(l)->l_cost) {
				getlink(l)->l_cost = cost;
				netbits(l, netchar, netdir);
			}
			return l;
		}
		prev = l;
	}

	/* allocate and link in the new link struct */
	l = newlink();
	if (cost != INF)	/* ignore back links */
		Lcount++;
	if (prev) {
		getlink(l)->l_next = getlink(prev)->l_next;
		getlink(prev)->l_next = l;
	} else {
		getlink(l)->l_next = getnode(from)->n_link;
		getnode(from)->n_link = l;
	}

	getlink(l)->l_to = to;
	getlink(l)->l_cost = cost + getnode(from)->n_cost;    /* add penalty */
	if (netchar == 0) {
		netchar = DEFNET;
		netdir = DEFDIR;
	}
	netbits(l, netchar, netdir);
	if (Dflag && ISADOMAIN(getnode(from)) && !ISADOMAIN(getnode(to)))
		getlink(l)->l_flag |= LTERMINAL;

	return l;
}

void
deadlink(nleft, nright) 
	p_node nleft;
	p_node nright;
{	p_link l;
	p_link lhold = 0;
	p_link lprev;
	p_link lnext;

	/* DEAD host */
	if (nright == 0) {
		getnode(nleft)->n_flag |= NDEAD;	/* DEAD host */
		return;
	}

	/* DEAD link */

	/* grab <nleft, nright> instances at head of nleft adjacency list */
	while ((l = getnode(nleft)->n_link) != 0 && 
					getlink(l)->l_to == nright) {
		getnode(nleft)->n_link = getlink(l)->l_next;	/* disconnect */
		getlink(l)->l_next = lhold;		/* terminate */
		lhold = l;				/* add to lhold */
	}

	/* move remaining <nleft, nright> instances */
	for (lprev = getnode(nleft)->n_link; lprev && getlink(lprev)->l_next;
					lprev = getlink(lprev)->l_next) {
		if (getlink(getlink(lprev)->l_next)->l_to == nright) {
			l = getlink(lprev)->l_next;
			getlink(lprev)->l_next = getlink(l)->l_next;	/* disconnect */
			getlink(l)->l_next = lhold;	/* terminate */
			lhold = l;
		}
	}

	/* check for emptiness */
	if (lhold == 0) {
		getlink(addlink(nleft, nright, INF / 2, DEFNET, DEFDIR))->l_flag |= LDEAD;
		return;
	}

	/* reinsert deleted edges as DEAD links */
	for (l = lhold; l; l = lnext) {
		lnext = getlink(l)->l_next;
		getlink(addlink(nleft, nright, getlink(l)->l_cost,
			NETCHAR(getlink(l)),
			NETDIR(getlink(l))))->l_flag |= LDEAD;
		freelink(l);
	}
}

STATIC void
netbits(l, netchar, netdir)
	register p_link l;
	char netchar, netdir;
{
	getlink(l)->l_flag &= ~LDIR;
	getlink(l)->l_flag |= netdir;
	getlink(l)->l_netop = netchar;
}

int
tracelink(arg) 
	char *arg;
{	char *bang;
	p_link l;
	register p_node tnode;

	if (Tracecount >= NTRACE)
		return -1;
	l = newlink();
	bang = index(arg, '!');
	if (bang) {
		*bang = 0;
		tnode = addnode(bang+1);
		getlink(l)->l_to = tnode;
	} else 
		getlink(l)->l_to = 0;

	tnode = addnode(arg);
	getlink(l)->l_from = tnode;
	Trace[Tracecount++] = l;
	return 0;
}

STATIC void
ltrace(from, to, cost, netchar, netdir)
	p_node from;
	p_node to;
	Cost cost;
	char netchar, netdir;
{	p_link l;
	int i;

	for (i = 0; i < Tracecount; i++) {
		l = Trace[i];
		/* overkill, but you asked for it! */
		if ((getlink(l)->l_to == 0
		 && (from == (p_node) getlink(l)->l_from
		 || to == (p_node) getlink(l)->l_from))
		 || (from == (p_node) getlink(l)->l_from
		 && to == getlink(l)->l_to)
		 || (to == (p_node) getlink(l)->l_from
		 && from == getlink(l)->l_to)) {
			ltrprint(from, to, cost, netchar, netdir);
			return;
		}
	}
}

/* print a trace item */
STATIC void
ltrprint(from, to, cost, netchar, netdir)
	p_node from;
	p_node to;
	Cost cost;
	char netchar;
	char netdir;
{	char buf[256], *bptr = buf;

	strcpy(bptr, getnode(from)->n_name);
	bptr += strlen(bptr);
	*bptr++ = ' ';
	if (netdir == LRIGHT)			/* @% */
		*bptr++ = netchar;
	strcpy(bptr, getnode(to)->n_name);
	bptr += strlen(bptr);
	if (netdir == LLEFT)			/* !: */
		*bptr++ = netchar;
	sprintf(bptr, "(%ld)", cost);
	yyerror(buf);
}

void
atrace(n1, n2)
	p_node n1;
	p_node n2;
{	p_link l;
	int i;
	char buf[256];

	for (i = 0; i < Tracecount; i++) {
		l = Trace[i];
		if (getlink(l)->l_to == 0
		    && ((p_node) getlink(l)->l_from == n1
		    || (p_node) getlink(l)->l_from == n2)) {
			sprintf(buf, "%s = %s", getnode(n1)->n_name,
				getnode(n2)->n_name);
			yyerror(buf);
			return;
		}
	}
}

#define EQ(n1, n2) strcmp(getnode(n1)->n_name, getnode(n2)->n_name) == 0
maptrace(from, to)
	register p_node from;
	register p_node to;
{	register p_link l;
	register int i;

	for (i = 0; i < Tracecount; i++) {
		l = Trace[i];
		if (getlink(l)->l_to == 0) {
			if (EQ(from, getlink(l)->l_from)
			    || EQ(to, getlink(l)->l_from))
				return 1;
		} else if (EQ(from, getlink(l)->l_from)
			   && EQ(to, getlink(l)->l_to))
				return 1;
	}
	return 0;
}
