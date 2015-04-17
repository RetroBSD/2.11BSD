/* pathalias -- by steve bellovin, as told to peter honeyman */

#ifndef lint
#ifdef MAIN
static char	*h_sccsid = "@(#)def.h	9.1 87/10/04";
#endif /*MAIN*/
#endif /*lint*/

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#ifdef TMPFILES
#include <sys/types.h>
#include <sys/file.h>
#endif /*TMPFILES*/

typedef	long Cost;
typedef struct node node;
typedef struct link link;

#ifndef TMPFILES
#define p_node node *
#define p_link link *
#else  /*TMPFILES*/
#define p_node unsigned int
#define p_link unsigned int
#endif /*TMPFILES*/

#ifdef lint
#define vprintf fprintf
#else /*!lint -- this gives null effect warning*/
/* because it's there ... */
#define vprintf		!Vflag ? 0 : fprintf
#endif /*lint*/

#define NTRACE	16	/* can trace up to NTRACE hosts/links */
#ifdef TMPFILES
#define MAXNAME 127	/* Maximum length of node name. */
#endif /*TMPFILES*/


/* scanner states (yylex, parse) */
#define OTHER	0
#define COSTING	1
#define NEWLINE	2

/* flags for n_flag */
#define ISPRIVATE  0x0001 /* invisible outside its definition file */
#define NALIAS	   0x0002 /* heaped as an alias */
#define ATSIGN	   0x0004 /* seen an at sign?  used for magic @/% rules */
#define MAPPED	   0x0008 /* extracted from heap */
#define	NDEAD	   0x0010 /* out links are dead */
#define HASLEFT	   0x0020 /* has a left side net character */
#define HASRIGHT   0x0040 /* route has a right side net character */
#define	NNET	   0x0080 /* network pseudo-host */
#define INDFS	   0x0100 /* used when removing net cycles (for -g) */
#define DUMP	   0x0200 /* we have dumped this net's edges (for -g) */
#define PRINTED	   0x0400 /* this host has been printed */
#define NTERMINAL  0x0800 /* heaped as terminal edge (or alias thereto) */

#define ISADOMAIN(n) ((n)->n_name[0] == '.')
#define ISANET(n) (((n)->n_flag & NNET) || ISADOMAIN(n))
#define DEADHOST(n) ((((n)->n_flag & (NNET | NDEAD)) == NDEAD) || (n->n_flag & NTERMINAL))
#define GATEWAYED(n) (((n)->n_flag & (NNET | NDEAD)) == (NNET | NDEAD) || ISADOMAIN(n))

#ifndef DEBUG
/*
 * save some space in nodes -- there are > 10,000 allocated!
 */

#define n_root un1.nu_root
#define n_net un1.nu_net
#define n_copy un1.nu_copy

#define n_private un2.nu_priv
#define n_parent  un2.nu_par

/* WARNING: if > 2^16 nodes, type of n_tloc must change */
struct node {
	char	*n_name;	/* host name */
#ifdef TMPFILES
	off_t	n_fname;	/* offset to name in name temp file */
	p_node	n_seq;		/* sequence number of node */
#endif /*TMPFILES*/
	p_link	n_link;		/* adjacency list */
	Cost	n_cost;		/* cost to this host */
	union {
		p_node nu_net;	/* others in this network (parsing) */
		p_node nu_root;	/* root of net cycle (graph dumping) */
		p_node nu_copy;	/* circular copy list (mapping) */
	} un1;
	union {
		p_node nu_priv;	/* other privates in this file (parsing) */
		p_node nu_par;	/* parent in shortest path tree (mapping) */
	} un2;
	unsigned short n_tloc;	/* back ptr to heap/hash table */
	unsigned short n_flag;		/* see manifests above */
};

#endif /*DEBUG*/

#define	DEFNET	'!'			/* default network operator */
#define	DEFDIR	LLEFT			/* host on left is default */
#define	DEFCOST	((Cost) 4000)		/* default cost of a link */
#define	INF	((Cost) 30000000)	/* infinitely expensive link */
#define DEFPENALTY ((Cost) 200)		/* default avoidance cost */

/* data structure for adjacency list representation */

/* flags for l_dir */

#define NETDIR(l)	((l)->l_flag & LDIR)
#define NETCHAR(l)	((l)->l_netop)

#define LDIR	  0x0008	/* 0 for left, 1 for right */
#define LRIGHT	  0x0000	/* user@host style */
#define LLEFT	  0x0008	/* host!user style */

#define LDEAD	  0x0010	/* this link is dead */
#define LALIAS	  0x0020	/* this link is an alias */
#define LTREE	  0x0040	/* member of shortest path tree */
#define LGATEWAY  0x0080	/* this link is a gateway */
#define LTERMINAL 0x0100	/* this link is terminal */

#ifndef DEBUG
/*
 * borrow a field for link/node tracing.  there's a shitload of
 * edges -- every word counts.  only so much squishing is possible:
 * alignment dictates that the size be a multiple of four.
 */

#define l_next un.lu_next
#define l_from un.lu_from

struct link {
	p_node	l_to;		/* adjacent node */
#ifdef TMPFILES
	p_link	l_seq;		/* link sequence number */
#endif /*TMPFILES*/
	Cost	l_cost;		/* edge cost */
	union {
		p_link lu_next;	/* rest of adjacency list (not tracing) */
		p_node lu_from;	/* source node (tracing) */
	} un;
	short	l_flag;		/* right/left syntax, flags */
	char	l_netop;	/* network operator */
};

#endif /*DEBUG*/

#ifdef DEBUG
/*
 * flattening out the unions makes it easier
 * to debug (when pi is unavailable).
 */
struct node {
	char	*n_name;
#ifdef TMPFILES
	off_t	n_fname;
	p_node	n_seq;
#endif /*TMPFILES*/
	p_link	n_link;
	Cost	n_cost;
	p_node	n_net;
	p_node	n_root;
	p_node	n_copy;
	p_node	n_private;
	p_node	n_parent;
	unsigned short n_tloc;
	unsigned short n_flag;
};
struct link {
	p_node	l_to;
#ifdef TMPFILES
	p_link	l_seq;
#endif /*TMPFILES*/
	Cost	l_cost;
	p_link	l_next;
	p_node	l_from;
	short	l_flag;
	char	l_netop;
};
#endif /*DEBUG*/
