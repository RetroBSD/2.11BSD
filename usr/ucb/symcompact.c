/*
 *	Program Name:   symcompact.c
 *	Date: January 21, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     21Jan94         1. Initial release into the public domain.
 *	1.1	11Feb94		2. Remove register symbols to save memory.
*/

/*
 * This program compacts the symbol table of an executable.  This is
 * done by removing '~symbol' references when _both_ the '~symbol' and
 * '_symbol' have an overlay number of 0.  The assembler always generates
 * both forms.  The only time both forms are needed is in an overlaid 
 * program and the routine has been relocated by the linker, in that event
 * the '_' form is the overlay "thunk" and the '~' form is the actual 
 * routine itself.  Only 'text' symbols have both forms.  Reducing the
 * number of symbols greatly speeds up 'nlist' processing as well as 
 * cutting down memory requirements for programs such as 'adb' and 'nm'.
 *
 * NOTE: This program attempts to hold both the string and symbol tables
 * in memory.  For the kernel which has not been 'strcompact'd this
 * amounts to about 49kb.  IF this program runs out of memory you should
 * run 'strcompact' first - that program removes redundant strings, 
 * significantly reducing the amount of memory needed.  Alas, this program
 * will undo some of strcompact's work and you may/will need to run
 * strcompact once more after removing excess symbols.
 *
 * Register symbols are removed to save memory.  This program was initially
 * used with a smaller kernel, adding an additional driver caused the symbol
 * table to grow enough that memory couldn't be allocated for strings.  See
 * the comments in 'symorder.c' - they explain why register variables are
 * no big loss.
*/

#include <stdio.h>
#include <a.out.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sysexits.h>
#include <sys/file.h>

	char	*Pgm;
static	char	strtmp[20];

main(argc, argv)
	int	argc;
	char	**argv;
	{
	FILE	*fp, *strfp;
	int	cnt, nsyms, len, c, symsremoved = 0, i;
	void	cleanup();
	char	*strtab;
	char	fbuf1[BUFSIZ], fbuf2[BUFSIZ];
	off_t	symoff, stroff, ltmp;
	long	strsiz;
	register struct	nlist	*sp, *sp2;
	struct	nlist	*symtab, *symtabend, syment;
	struct	xexec	xhdr;

	Pgm = argv[0];
	signal(SIGQUIT, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	if	(argc != 2)
		{
		fprintf(stderr, "%s: filename argument missing\n", Pgm);
		exit(EX_USAGE);
		}
	fp = fopen(argv[1], "r+");
	if	(!fp)
		{
		fprintf(stderr, "%s: can't open '%s' for update\n", Pgm,
			argv[1]);
		exit(EX_NOINPUT);
		}
	setbuf(fp, fbuf1);
	cnt = fread(&xhdr, 1, sizeof (xhdr), fp);
	if	(cnt < sizeof (xhdr.e))
		{
		fprintf(stderr, "%s: Premature EOF reading header\n", Pgm);
		exit(EX_DATAERR);
		}
	if	(N_BADMAG(xhdr.e))
		{
		fprintf(stderr, "%s: Bad magic number\n", Pgm);
		exit(EX_DATAERR);
		}
	nsyms = xhdr.e.a_syms / sizeof (struct nlist);
	if	(!nsyms)
		{
		fprintf(stderr, "%s: '%s' stripped\n", Pgm);
		exit(EX_OK);
		}
	stroff = N_STROFF(xhdr);
	symoff = N_SYMOFF(xhdr);
/*
 * Seek to the string table size longword and read it.  Then attempt to
 * malloc memory to hold the string table.  First make a sanity check on
 * the size.
*/
	fseek(fp, stroff, L_SET);
	fread(&strsiz, sizeof (long), 1, fp);
	if	(strsiz > 48 * 1024L)
		{
		fprintf(stderr, "%s: string table > 48kb\n", Pgm);
		exit(EX_DATAERR);
		}
	strtab = (char *)malloc((int)strsiz);
	if	(!strtab)
		{
		fprintf(stderr, "%s: no memory for strings\n", Pgm);
		exit(EX_OSERR);
		}
/*
 * Now read the string table into memory.  Reduce the size read because
 * we've already retrieved the string table size longword.  Adjust the
 * address used so that we don't have to adjust each symbol table entry's
 * string offset.
*/
	cnt = fread(strtab + sizeof (long), 1, (int)strsiz - sizeof (long), fp);
	if	(cnt != (int)strsiz - sizeof (long))
		{
		fprintf(stderr, "%s: Premature EOF reading strings\n", Pgm);
		exit(EX_DATAERR);
		}
/*
 * Seek to the symbol table.  Scan it and count how many symbols are 
 * significant.
*/
	fseek(fp, symoff, L_SET);
	cnt = 0;
	for	(i = 0; i < nsyms; i++)
		{
		fread(&syment, sizeof (syment), 1, fp);
		if	(exclude(&syment))
			continue;
		cnt++;
		}

/*
 * Allocate memory for the symbol table.
*/
	symtab = (struct nlist *)malloc(cnt * sizeof (struct nlist));
	if	(!symtab)
		{
		fprintf(stderr, "%s: no memory for symbols\n", Pgm);
		exit(EX_OSERR);
		}

/*
 * Now read the symbols in, excluding the same ones as before, and
 * assign the in-memory string addresses at the same time
*/
	sp = symtab;
	fseek(fp, symoff, L_SET);

	for	(i = 0; i < nsyms; i++)
		{
		fread(&syment, sizeof (syment), 1, fp);
		if	(exclude(&syment))
			continue;
		bcopy(&syment, sp, sizeof (syment));
		sp->n_un.n_name = strtab + (int)sp->n_un.n_strx;
		sp++;
		}
	symtabend = &symtab[cnt];

/*
 * Now look for symbols with overlay numbers of 0 (root/base segment) and
 * of type 'text'.  For each symbol found check if there exists both a '~'
 * and '_' prefixed form of the symbol.  Preserve the '_' form and clear
 * the '~' entry by zeroing the string address of the '~' symbol.
*/
	for	(sp = symtab; sp < symtabend; sp++)
		{
		if	(sp->n_ovly)
			continue;
		if	((sp->n_type & N_TYPE) != N_TEXT)
			continue;
		if	(sp->n_un.n_name[0] != '~')
			continue;
/*
 * At this point we have the '~' form of a non overlaid text symbol.  Look
 * thru the symbol table for the '_' form.  All of 1) symbol type, 2) Symbol
 * value and 3) symbol name (starting after the first character) must match.
*/
		for	(sp2 = symtab; sp2 < symtabend; sp2++)
			{
			if	(sp2->n_ovly)
				continue;
			if	((sp2->n_type & N_TYPE) != N_TEXT)
				continue;
			if	(sp2->n_un.n_name[0] != '_')
				continue;
			if	(sp2->n_value != sp->n_value)
				continue;
			if	(strcmp(sp->n_un.n_name+1, sp2->n_un.n_name+1))
				continue;
/*
 * Found a match.  Null out the '~' symbol's string address.
*/
			symsremoved++;
			sp->n_un.n_strx = NULL;
			break;
			}
		}
/*
 * Done with the nested scanning of the symbol table.  Now create a new
 * string table (from the remaining symbols) in a temporary file.
*/
	strcpy(strtmp, "/tmp/strXXXXXX");
	mktemp(strtmp);
	strfp = fopen(strtmp, "w+");
	if	(!strfp)
		{
		fprintf(stderr, "%s: can't create '%s'\n", Pgm, strtmp);
		exit(EX_CANTCREAT);
		}
	setbuf(strfp, fbuf2);

/*
 * As each symbol is written to the tmp file the symbol's string offset
 * is updated with the new file string table offset.
*/
	ltmp = sizeof (long);
	for	(sp = symtab; sp < symtabend; sp++)
		{
		if	(!sp->n_un.n_name)
			continue;
		len = strlen(sp->n_un.n_name) + 1;
		fwrite(sp->n_un.n_name, len, 1, strfp);
		sp->n_un.n_strx = ltmp;
		ltmp += len;
		}
/*
 * We're done with the memory string table - give it back.  Then reposition
 * the new string table file to the beginning.
*/
	free(strtab);
	rewind(strfp);

/*
 * Position the executable file to where the symbol table begins.  Truncate
 * the file.  Write out the valid symbols, counting each one so that the 
 * a.out header can be updated when we're done.
*/
	nsyms = 0;
	fseek(fp, symoff, L_SET);
	ftruncate(fileno(fp), ftell(fp));
	for	(sp = symtab; sp < symtabend; sp++)
		{
		if	(sp->n_un.n_strx == 0)
			continue;
		nsyms++;
		fwrite(sp, sizeof (struct nlist), 1, fp);
		}
/*
 * Next write out the string table size longword.
*/
	fwrite(&ltmp, sizeof (long), 1, fp);
/*
 * We're done with the in memory symbol table, release it.  Then append
 * the string table to the executable file.
*/
	free(symtab);
	while	((c = getc(strfp)) != EOF)
		putc(c, fp);
	fclose(strfp);
	rewind(fp);
	xhdr.e.a_syms = nsyms * sizeof (struct nlist);
	fwrite(&xhdr.e, sizeof (xhdr.e), 1, fp);
	fclose(fp);
	printf("%s: %d symbols removed\n", Pgm, symsremoved);
	cleanup();
	}

void
cleanup()
	{
	if	(strtmp[0])
		unlink(strtmp);
	exit(EX_OK);
	}

/*
 * Place any symbol exclusion rules in this routine, return 1 if the
 * symbol is to be excluded, 0 if the symbol is to be retained.
*/

exclude(sp)
	register struct nlist *sp;
	{

	if	(sp->n_type == N_REG)
		return(1);
	if	(sp->n_un.n_strx == 0)
		return(1);
	return(0);
	}
