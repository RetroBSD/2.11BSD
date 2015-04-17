/*
 *	Program Name:   strdump.c
 *	Date: January 21, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     12Feb94         1. Initial release into the public domain.
*/

/*
 * Dump the symbol table of a program to stdout, one symbol per line in
 * the form:
 *
 *	symbol_string  type  overlay  value
 *
 * Typical use is to feed the output of this program into:
 *
 *    "sort +0 -1 +1n -2 +2n -3 +3n -4 -u"
 *
 * This program is used by 'strcompact' to compress the string (and
 * symbol) tables of an executable. 
*/

#include <sys/types.h>
#include <sys/dir.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
#include <sys/file.h>
#include <string.h>

	char	**xargv;	/* global copy of argv */
	char	*strp;		/* pointer to in-memory string table */
	struct	xexec xhdr;	/* the extended a.out header */

extern	char	*malloc();

main(argc, argv)
	int	argc;
	char	**argv;
	{

	if	(argc != 2)
		{
		fprintf(stderr, "%s:  need a file name\n", argv[0]);
		exit(1);
		}
	xargv = ++argv;
	namelist();
	exit(0);
	}

namelist()
	{
	char	ibuf[BUFSIZ];
	register FILE	*fi;
	off_t	o, stroff;
	long	strsiz;
	register int n;

	fi = fopen(*xargv, "r");
	if	(fi == NULL)
		error("cannot open");
	setbuf(fi, ibuf);

	fread((char *)&xhdr, 1, sizeof(xhdr), fi);
	if	(N_BADMAG(xhdr.e))
		error("bad format");
	rewind(fi);

	o = N_SYMOFF(xhdr);
	fseek(fi, o, L_SET);
	n = xhdr.e.a_syms / sizeof(struct nlist);
	if	(n == 0)
		error("no name list");

	stroff = N_STROFF(xhdr);
	fseek(fi, stroff, L_SET);
	if	(fread(&strsiz, sizeof (long), 1, fi) != 1)
		error("no string table");
	strp = (char *)malloc((int)strsiz);
	if	(strp == NULL || strsiz > 48 * 1024L)
		error("no memory for strings");
	if	(fread(strp+sizeof(strsiz),(int)strsiz-sizeof(strsiz),1,fi)!=1)
		error("error reading strings");

	fseek(fi, o, L_SET);
	dumpsyms(fi, n);
	free((char *)strp);
	fclose(fi);
	}

dumpsyms(fi, nsyms)
	register FILE *fi;
	int	nsyms;
	{
	register int n;
	struct	nlist sym;
	register struct nlist *sp;

	sp = &sym;
	for	(n = 0; n < nsyms; n++)
		{
		fread(&sym, sizeof sym, 1, fi);
		printf("%s %u %u %u\n", strp + (int)sp->n_un.n_strx, sp->n_type,
			sp->n_ovly, sp->n_value);
		}	
	}

error(s)
	char *s;
	{
	fprintf(stderr, "syms: %s: %s\n", *xargv, s);
	exit(1);
	}
