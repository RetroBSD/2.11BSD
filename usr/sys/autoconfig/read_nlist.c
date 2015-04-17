/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)read_nlist.c	2.2 (2.11BSD GTE) 1/10/94
 */

/*
 * Read all the symbols we'll need for the devices we want to configure
 * These are -- The probe, attach and handler routines for each device,
 * and a few symbols that we'll need later on.
 */

#include <machine/autoconfig.h>
#include <sys/param.h>
#include <sys/file.h>
#include <a.out.h>
#include <stdio.h>
#include "dtab.h"

extern	char	*strdup();
extern	char	*nlist_name;	/* file we read the namelist from */
extern int	guess_ndev;	/* number of lines read from dtab */
extern int	debug;
extern int	kmem;
extern int	pflag;
NLIST	*nl, *np, *int_nl, *good_nl, *bad_nl, *trap_nl, *sep_nl, *vers_nl,
	*add_nlist(), *end_vector, *next_nl;

#define END_NAME	"endvec"
#define	NEXTIV_NAME	"_nextiv"	/* floating vector allocator */
#define INT_NAME	"_conf_int"
#define GOOD_NAME	"CGOOD"
#define BAD_NAME	"CBAD"
#define TRAP_NAME	"trap"
#define SEPID_NAME	"KERN_NONSEP"	/* KERN_NONSEP */
#define VERSION		"_version"

read_nlist()
{
	register NLIST	**lp,
			*nnp;
	register DTAB	*dp;
	struct exec	head;
	struct ovlhdr	ovhead;
	HAND	*already = (HAND *)NULL,
		*sp;
	off_t	offst;
	int	unix_fd;
	short	cnt;
	char	tname[20],
		unix_vers[100],
		core_vers[100],
		*calloc();
	NLIST	*add_nlist();

	np = nl = (NLIST *)calloc(guess_ndev + 10,sizeof(NLIST));
	for (dp = devs; dp != NULL; dp = dp->dt_next) {
		sprintf(tname, "_%sprobe", dp->dt_name);
		dp->dt_probe = add_nlist(tname);
		sprintf(tname, "_%sattach", dp->dt_name);
		dp->dt_attach = add_nlist(tname);
		sprintf(tname, "_%sVec", dp->dt_name);
		dp->dt_setvec = add_nlist(tname);
		for (sp = dp->dt_handlers;sp;sp = sp->s_next)
			sp->s_nl = add_nlist(sp->s_str);
	}
	end_vector = np++;
	end_vector->n_un.n_name = END_NAME;
	int_nl = np++;
	int_nl->n_un.n_name = INT_NAME;
	good_nl = np++;
	good_nl->n_un.n_name = GOOD_NAME;
	bad_nl = np++;
	bad_nl->n_un.n_name = BAD_NAME;
	next_nl = np++;
	next_nl->n_un.n_name = NEXTIV_NAME;
	trap_nl = np++;
	trap_nl->n_un.n_name = TRAP_NAME;
	vers_nl = np++;
	vers_nl->n_un.n_name = VERSION;
	sep_nl = np++;
	sep_nl->n_un.n_name = SEPID_NAME;

	if ((unix_fd = open(nlist_name,O_RDONLY)) < 0) {
		perror(nlist_name);
		exit(AC_SETUP);
	}
	nlist(nlist_name,nl);
	if (debug || pflag)
		for (np = nl; np->n_un.n_name; np++)
			printf("%s = %o\n", np->n_un.n_name, np->n_value);
	for (np = end_vector; np <= trap_nl; np++)
		if (!np->n_value) {
			fprintf(stderr,"%s: couldn't find symbols in %s.\n",myname,nlist_name);
			exit(AC_SETUP);
		}
	if (!debug) {
#define round(x) (ctob(stoc(ctos(btoc(x)))))
		lseek(unix_fd,(off_t)0,L_SET);
		read(unix_fd,(char *)&head,sizeof(head));
		offst = (off_t)vers_nl->n_value + (off_t)head.a_text + sizeof(head);
		if (head.a_magic == A_MAGIC2 || head.a_magic == A_MAGIC5)
			offst -= (off_t)round(head.a_text);
		if (head.a_magic == A_MAGIC5 || head.a_magic == A_MAGIC6) {
			read(unix_fd,(char *)&ovhead,sizeof(ovhead));
			offst += sizeof(ovhead);
			if (head.a_magic == A_MAGIC5)
				offst -= (off_t)round(ovhead.max_ovl);
			for (cnt = 0;cnt < NOVL;++cnt)
				offst += (off_t)ovhead.ov_siz[cnt];
		}
		lseek(unix_fd,offst,L_SET);
		read(unix_fd,unix_vers,sizeof(unix_vers));
		lseek(kmem,(off_t)vers_nl->n_value,L_SET);
		read(kmem,core_vers,sizeof(core_vers));
		unix_vers[99] = core_vers[99] = EOS;	/* Just in case! */
		if (strcmp(unix_vers,core_vers)) {
			fprintf(stderr,"%s: %s is not the running version.\n",myname,nlist_name);
			exit(AC_SETUP);
		}
	}
	close(unix_fd);
}

/*
 * If the passed symbol is in the nlist table, return pointer to it,
 * otherwise add it to the table and return a pointer to new entry.
 */
NLIST *
add_nlist(name)
char	*name;
{
	register NLIST	*n;

	for (n = nl;n < np;++n)
		if (!strcmp(n->n_un.n_name, name))
			return(n);
	np->n_un.n_name = strdup(name);
	return(np++);
}
