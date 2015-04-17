/* putnode -- store node and link structrures temporarily in a disk file */

#include "def.h"
#ifdef TMPFILES
/*
 * Sizes of node and link caches.  These must be big enough to prevent entries
 * being flushed during assignments (e.g. getlink(l)->l_to = function(x),
 * where function(x) pulls more than LINKCACHE entries into the cache).  This
 * is because the left hand side of the assignment is evaluated first and
 * subsequent evaluation of the right hand side may replace the contents of
 * the address being written into by a more recently used link (at least in
 * Ritchie's compiler).  This causes obscure errors.  Avoid assignments of
 * this type if the function makes lots of calls to getlink() or getnode().
 * Ten is certainly big enough, but hardly optimal.  If you need more memory,
 * shrink the hash table in addnode.c, don't change these.
 */
#define NODECACHE 35	/* tradeoff between user and sys time */
#define LINKCACHE 35

/*
 * Linked lists and other miscellaneous cache debris.
 */
static struct nodecache {
	struct nodecache *next;
	struct nodecache *prev;
	struct node node;
} nodecache[NODECACHE];

static struct linkcache {
	struct linkcache *next;
	struct linkcache *prev;
	struct link link;
} linkcache[LINKCACHE];

static struct nodecache *topnode;
static struct linkcache *toplink;

/*
 * We set up offsets of zero to map to NULL for simplicity.
 */
node	nullnode;
link	nulllink;

/* exports */
void initstruct();
extern node *getnode();
extern link *getlink();
char *nfile, *lfile, *sfile;
int fdnode, fdlink, fdname;
long nhits, lhits, nmisses, lmisses;

/* imports */
extern void nomem(), die();
char *mktemp();
off_t lseek();

/* privates */
STATIC void nodewrite(), noderead(), linkwrite(), linkread();

/*
 * Opens and initializes temporary files and caches.
 */
void
initstruct()
{
	register int i;

	/* Open the temporary files. */
	nfile = mktemp("/tmp/nodeXXXXXX");
	if ((fdnode = open(nfile, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror(nfile);
		exit(1);
	}
	lfile = mktemp("/tmp/linkXXXXXX");
	if ((fdlink = open(lfile, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror(lfile);
		exit(1);
	}
	sfile = mktemp("/tmp/nameXXXXXX");
	if ((fdname = open(sfile, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror(sfile);
		exit(1);
	}

	/* Put the handy null nodes, etc. at offset zero. */
	(void) write(fdnode, (char *) &nullnode, sizeof(nullnode));
	(void) write(fdlink, (char *) &nulllink, sizeof(nulllink));
	(void) write(fdname, "\0", 1);

	/* Link the caches together. */
	topnode = &nodecache[0];
	topnode->prev = (struct nodecache *) NULL;
	topnode->next = &nodecache[1];
	for (i = 1; i < NODECACHE - 1; i++) {
		nodecache[i].next = &nodecache[i+1];
		nodecache[i].prev = &nodecache[i-1];
	}
	nodecache[NODECACHE - 1].prev = &nodecache[NODECACHE - 2];
	nodecache[NODECACHE - 1].next = (struct nodecache *) NULL;

	toplink = &linkcache[0];
	toplink->prev = (struct linkcache *) NULL;
	toplink->next = &linkcache[1];
	for (i = 1; i < LINKCACHE - 1; i++) {
		linkcache[i].next = &linkcache[i+1];
		linkcache[i].prev = &linkcache[i-1];
	}
	linkcache[LINKCACHE - 1].prev = &linkcache[LINKCACHE - 2];
	linkcache[LINKCACHE - 1].next = (struct linkcache *) NULL;
}

/*
 * Returns pointer to node; takes sequence number of node.
 */
node *
getnode(n_seq)
register p_node n_seq;
{
	register struct nodecache *tnode;

	/* Don't bother to retrieve the null entry. */
	if (n_seq == 0)
		return((node *) NULL);

	/*
	 * First search for the correct entry in the cache.  If we find it,
	 * move it to the top of the cache.
	 */
	for (tnode = topnode; tnode->next; tnode = tnode->next) {
		if (n_seq == tnode->node.n_seq) {
found:			if (tnode->prev) {
				tnode->prev->next = tnode->next;
				if (tnode->next)
					tnode->next->prev = tnode->prev;
				tnode->prev = (struct nodecache *) NULL;
				tnode->next = topnode;
				topnode->prev = tnode;
				topnode = tnode;
			}
			nhits++;
			return(&tnode->node);
		}
	}
	if (n_seq == tnode->node.n_seq)
		goto found;

	/*
	 * If it isn't in the cache, see if the last one in the
	 * cache has valid data.  If it does, write it out, free
	 * the name pointer, fill it with the data we want, and
	 * move it to the top of the cache.
	 */
	if (tnode->node.n_seq)
		nodewrite(&tnode->node);
	if (tnode->node.n_name)
		free((char *) tnode->node.n_name);
	tnode->prev->next = (struct nodecache *) NULL;
	tnode->prev = (struct nodecache *) NULL;
	tnode->next = topnode;
	topnode->prev = tnode;
	topnode = tnode;

	noderead(&tnode->node, n_seq);

	nmisses++;
	return(&tnode->node);
}

/*
 * Write out a node in cache to disk.  Takes a pointer to the node to be
 * written.  If the name of the node hasn't ever been written to disk (as
 * will be the case with new nodes), n_fname will be null; we fill it in and
 * write the name to the name temporary file.
 */
STATIC void
nodewrite(n)
node *n;
{
	if (n->n_fname == 0) {
		n->n_fname = lseek(fdname, (off_t) 0, L_XTND);
		if (write(fdname, n->n_name, strlen(n->n_name) + 1) < 0) {
			perror("writename");
			exit(1);
		}
	}
	(void) lseek(fdnode, (off_t) n->n_seq * sizeof(node), L_SET);
	if (write(fdnode, (char *) n, sizeof(node)) < 0) {
		perror("nodewrite");
		exit(1);
	}
}

/*
 * Read a node from the disk into the cache.  Takes the sequence number of the
 * node to be read.  We first read in the node, then set and fill in the
 * pointer to the name of the node.
 */
STATIC void
noderead(n, n_seq)
node *n;
p_node n_seq;
{
	(void) lseek(fdnode, (off_t) n_seq * sizeof(node), L_SET);
	if (read(fdnode, (char *) n, sizeof(node)) < 0) {
		perror("noderead");
		exit(1);
	}
	if (n->n_fname) {
		if ((n->n_name = calloc(1, MAXNAME + 1)) == 0)
			nomem();
		(void) lseek(fdname, n->n_fname, L_SET);
		if (read(fdname, n->n_name, MAXNAME + 1) < 0) {
			perror("readname");
			exit(1);
		}
	}
	if (n->n_seq && n->n_seq != n_seq) {		/* sanity check */
		fprintf(stderr, "noderead %u gives %u\n", n_seq, n->n_seq);
		die("impossible retrieval error");
	}
}

/*
 * Returns pointer to link; takes sequence number of link.
 */
link *
getlink(l_seq)
register p_link l_seq;
{
	register struct linkcache *tlink;

	/* Don't bother to retrieve the null entry. */
	if (l_seq == 0)
		return((link *) NULL);

	/*
	 * First search for the correct entry in the cache.  If we find it,
	 * move it to the top of the cache.
	 */
	for (tlink = toplink; tlink->next; tlink = tlink->next) {
		if (l_seq == tlink->link.l_seq) {
found:			if (tlink->prev) {
				tlink->prev->next = tlink->next;
				if (tlink->next)
					tlink->next->prev = tlink->prev;
				tlink->prev = (struct linkcache *) NULL;
				tlink->next = toplink;
				toplink->prev = tlink;
				toplink = tlink;
			}
			lhits++;
			return(&tlink->link);
		}
	}
	if (l_seq == tlink->link.l_seq)
		goto found;

	/*
	 * If it isn't in the cache, see if the last one in the cache has
	 * valid data.  If it does, write it out, fill it with the data we
	 * want, and move it to the top of the cache.
	 */
	if (tlink->link.l_seq)
		linkwrite(&tlink->link);
	tlink->prev->next = (struct linkcache *) NULL;
	tlink->prev = (struct linkcache *) NULL;
	tlink->next = toplink;
	toplink->prev = tlink;
	toplink = tlink;

	linkread(&tlink->link, l_seq);

	lmisses++;
	return(&tlink->link);
}

/*
 * Write out a link in cache to disk.  Takes a pointer to the link to be
 * written.
 */
STATIC void
linkwrite(l)
link *l;
{
	(void) lseek(fdlink, (off_t) l->l_seq * sizeof(link), L_SET);
	if (write(fdlink, (char *) l, sizeof(link)) < 0) {
		perror("linkwrite");
		exit(1);
	}
}

/*
 * Read a link from the disk into the cache.  Takes the sequence number of the
 * link to be read.
 */
STATIC void
linkread(l, l_seq)
link *l;
p_link l_seq;
{
	(void) lseek(fdlink, (off_t) l_seq * sizeof(link), L_SET);
	if (read(fdlink, (char *) l, sizeof(link)) < 0) {
		perror("linkread");
		exit(1);
	}
}
#endif /*TMPFILES*/
