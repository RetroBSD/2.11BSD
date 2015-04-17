/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)printit.c	9.1 87/10/04";
#endif

#include "def.h"

/*
 * print the routes by traversing the shortest path tree in preorder.
 * use lots of char bufs -- profiling indicates this costs about 5 kbytes
 */

/* exports */
extern void printit();

/* imports */
extern int Cflag, Vflag, Dflag, Fflag;
extern p_node Home;
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
extern node *getnode();
extern link *getlink();
#endif /*TMPFILES*/
extern char *Netchars;
extern void die();

/* privates */
static p_link Ancestor;	/* for -f option */
STATIC void preorder(), setpath(), printhost(), printdomain();
STATIC char *hostpath();

/* in practice, even the longest paths are < 100 bytes */
#ifndef TMPFILES
#define PATHSIZE 512
#else /*TMPFILES*/
#define PATHSIZE 172	/* this saves a stack segment, which we use in heap */
#endif /*TMPFILES*/

void
printit()
{	p_link l;
	char pbuf[PATHSIZE];

	/* print home */
	if (Cflag)
		printf("%ld\t", (long) getnode(Home)->n_cost);
	printf("%s\t%%s\n", getnode(Home)->n_name);
	
	strcpy(pbuf, "%s");
	for (l = getnode(Home)->n_link; l; l = getlink(l)->l_next) {
		if (getlink(l)->l_flag & LTREE) {
			getlink(l)->l_flag &= ~LTREE;
			Ancestor = l;
			preorder(l, pbuf);
			strcpy(pbuf, "%s");
		}
	}
	fflush(stdout);
	fflush(stderr);
}

/*
 * preorder traversal of shortest path tree.
 */
STATIC void
preorder(l, ppath)
	register p_link l;
	char *ppath;
{	register p_node n;
	p_node ncp;		/* circular copy list */
	Cost cost;
	char npath[PATHSIZE];
	short p_dir;		/* DIR bits of parent (for nets) */
	char p_op;		/* net op of parent (for nets) */

	setpath(l, ppath, npath);
	n = getlink(l)->l_to;
	if (printable(n)) {
		if (Fflag)
			cost = getnode(getlink(Ancestor)->l_to)->n_cost;
		else
			cost = getnode(n)->n_cost;
		if (ISADOMAIN(getnode(n)))
			printdomain(n, npath, cost);
		else if (!(getnode(n)->n_flag & NNET)) {
			printhost(n, npath, cost);
		}
		getnode(n)->n_flag |= PRINTED;
		for (ncp = getnode(n)->n_copy; ncp != n;
		     ncp = getnode(ncp)->n_copy)
			getnode(ncp)->n_flag |= PRINTED;
	}

	/* prepare routing bits for domain members */
	p_dir = getlink(l)->l_flag & LDIR;
	p_op = getlink(l)->l_netop;

	/* recursion */
	for (l = getnode(n)->n_link; l; l = getlink(l)->l_next) {
		if (!(getlink(l)->l_flag & LTREE))
			continue;
		/* network member inherits the routing syntax of its gateway */
		if (ISANET(getnode(n))) {
			getlink(l)->l_flag = (getlink(l)->l_flag & ~LDIR) | p_dir;
			getlink(l)->l_netop = p_op;
		}
		getlink(l)->l_flag &= ~LTREE;
		preorder(l, npath);
	}
}

STATIC int
printable(n)
	register p_node n;
{	p_node ncp;
	p_link l;

	if (getnode(n)->n_flag & PRINTED)
		return 0;

	/* is there a cheaper copy? */
	for (ncp = getnode(n)->n_copy; n != ncp; ncp = getnode(ncp)->n_copy) {
		if (!(getnode(ncp)->n_flag & MAPPED))
			continue;	/* unmapped copy */

		if (getnode(n)->n_cost > getnode(ncp)->n_cost)
			return 0;	/* cheaper copy */

		if (getnode(n)->n_cost == getnode(ncp)->n_cost
					  && !(getnode(ncp)->n_flag & NTERMINAL))
			return 0;	/* synthetic copy */
	}

	/* will a domain route suffice? */
	if (Dflag && !ISANET(getnode(n)) && ISADOMAIN(getnode(getnode(n)->n_parent))) {
		/*
		 * are there any interesting links?  a link
		 * is interesting if it doesn't point back
		 * to the parent, and is not an alias.
		 */

		/* check n */
		for (l = getnode(n)->n_link; l; l = getlink(l)->l_next) {
			if (getlink(l)->l_to == getnode(n)->n_parent)
				continue;
			if ((!getlink(l)->l_flag & LALIAS))
				return 1;
		}

		/* check copies of n */
		for (ncp = getnode(n)->n_copy; ncp != n; ncp = getnode(ncp)->n_copy) {
			for (l = getnode(ncp)->n_link; l; l = getlink(l)->l_next) {
				if (getlink(l)->l_to == getnode(n)->n_parent)
					continue;
				if (!(getlink(l)->l_flag & LALIAS))
					return 1;
			}
		}

		/* domain route suffices */
		return 0;
	}
	return 1;
}

STATIC void
setpath(l, ppath, npath) 
	p_link l;
	register char *ppath, *npath;
{	register p_node next;
	register p_node parent;
	char netchar;

	next = getlink(l)->l_to;
	parent = getnode(next)->n_parent;
	netchar = NETCHAR(getlink(l));

	/* for magic @->% conversion */
	if (getnode(parent)->n_flag & ATSIGN)
		getnode(next)->n_flag |= ATSIGN;

	/*
	 * i've noticed that distant gateways to domains frequently get
	 * ...!gateway!user@dom.ain wrong.  ...!gateway!user%dom.ain
	 * seems to work, so if next is a domain and the parent is
	 * not the local host, force a magic %->@ conversion.  in this
	 * context, "parent" is the nearest ancestor that is not a net.
	 */
	while (ISANET(getnode(parent)))
		parent = getnode(parent)->n_parent;
	if (ISADOMAIN(getnode(next)) && parent != Home)
		getnode(next)->n_flag |= ATSIGN;

	/*
	 * special handling for nets (including domains) and aliases.
	 * part of the trick is to treat aliases to domains as 0 cost
	 * links.  (the author believes they should be declared as such
	 * in the input, but the world disagrees).
	 */
	if (ISANET(getnode(next)) || ((getlink(l)->l_flag & LALIAS)
	    && !ISADOMAIN(getnode(parent)))) {
		strcpy(npath, ppath);
		return;
	}
		
	if (netchar == '@')
		if (getnode(next)->n_flag & ATSIGN)
			netchar = '%';	/* shazam?  shaman? */
		else
			getnode(next)->n_flag |= ATSIGN;

	/* remainder should be a sprintf -- foo on '%' as an operator */
	for ( ; *npath = *ppath; ppath++) {
		if (*ppath == '%') {
			switch(ppath[1]) {
			case 's':
				ppath++;
				npath = hostpath(npath, l, netchar);
				break;

			case '%':
				*++npath = *++ppath;
				npath++;
				break;

			default:
				die("unknown escape in setpath");
				break;
			}
		} else
			npath++;
	}
}

STATIC char *
hostpath(path, l, netchar)
	register char *path;
	register p_link l;
	char netchar;
{	register p_node prev;

	prev = getnode(getlink(l)->l_to)->n_parent;
	if (NETDIR(getlink(l)) == LLEFT) {
		/* host!%s */
		strcpy(path, getnode(getlink(l)->l_to)->n_name);
		path += strlen(path);
		while (ISADOMAIN(getnode(prev))) {
			strcpy(path, getnode(prev)->n_name);
			path += strlen(path);
			prev = getnode(prev)->n_parent;
		}
		*path++ = netchar;
		if (netchar == '%')
			*path++ = '%';
		*path++ = '%';
		*path++ = 's';
	} else {
		/* %s@host */
		*path++ = '%';
		*path++ = 's';
		*path++ = netchar;
		if (netchar == '%')
			*path++ = '%';
		strcpy(path, getnode(getlink(l)->l_to)->n_name);
		path += strlen(path);
		while (ISADOMAIN(getnode(prev))) {
			strcpy(path, getnode(prev)->n_name);
			path += strlen(path);
			prev = getnode(prev)->n_parent;
		}
	}
	return path;
}

STATIC void
printhost(n, path, cost)
	register p_node n;
	char *path;
	Cost cost;
{
	if (getnode(n)->n_flag & PRINTED)
		die("printhost called twice");
	getnode(n)->n_flag |= PRINTED;
	/* skip private hosts */
	if ((getnode(n)->n_flag & ISPRIVATE) == 0) {
		if (Cflag)
			printf("%ld\t", (long) cost);
		fputs(getnode(n)->n_name, stdout);
		putchar('\t');
		puts(path);
	}
}

STATIC void
printdomain(n, path, cost)
	register p_node n;
	char *path;
	Cost cost;
{	p_node p;

	if (getnode(n)->n_flag & PRINTED)
		die("printdomain called twice");
	getnode(n)->n_flag |= PRINTED;

	/*
	 * print a route for dom.ain if it is a top-level domain, unless
	 * it is private.
	 *
	 * print a route for sub.dom.ain only if all its ancestor dom.ains
	 * are private and sub.dom.ain is not private.
	 */
	if (!ISADOMAIN(getnode(getnode(n)->n_parent))) {
		/* top-level domain */
		if (getnode(n)->n_flag & ISPRIVATE) {
			vprintf(stderr, "ignoring private top-level domain %s\n", getnode(n)->n_name);
			return;
		}
	} else {
		/* subdomain */
		for (p = getnode(n)->n_parent; ISADOMAIN(getnode(p));
		     p = getnode(p)->n_parent)
			if (!(getnode(p)->n_flag & ISPRIVATE))
				return;
		if (getnode(n)->n_flag & ISPRIVATE)
			return;
	}

	/* print it (at last!) */
	if (Cflag)
		printf("%ld\t", (long) cost);
	do {
		fputs(getnode(n)->n_name, stdout);
		n = getnode(n)->n_parent;
	} while (ISADOMAIN(getnode(n)));
	putchar('\t');
	puts(path);
}
