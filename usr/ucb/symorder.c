/*
 *	Program Name:   symorder.c
 *	Date: January 21, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     21Jan94         1. Initial release into the public domain.
*/

/*
 * This program reorders the symbol table of an executable.  This is
 * done by moving symbols found in the second file argument (one symbol
 * per line) to the front of the symbol table.
 *
 * NOTE: This program needs to hold the string table in memory.
 * For the kernel which has not been 'strcompact'd this is about 21kb.
 * It is highly recommended that 'strcompact' be run first - that program 
 * removes redundant strings, significantly reducing the amount of memory 
 * needed.  Running 'symcompact' will reduce the run time needed by
 * this program by eliminating redundant non-overlaid text symbols.
*/

#include <stdio.h>
#include <a.out.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sysexits.h>
#include <sys/file.h>

#define NUMSYMS	125
	char	*order[NUMSYMS];
	int	nsorted; 
	char	*Pgm;
	void	cleanup();
static	char	sym1tmp[20], sym2tmp[20], strtmp[20];
static	char	*strtab, *oldname;

main(argc, argv)
	int	argc;
	char	**argv;
	{
	FILE	*fp, *fp2, *sym1fp, *sym2fp, *strfp;
	int	cnt, nsyms, len, c;
	char	fbuf1[BUFSIZ], fbuf2[BUFSIZ];
	off_t	symoff, stroff, ltmp;
	long	strsiz;
	struct	nlist	sym;
	struct	xexec	xhdr;

	Pgm = argv[0];

	signal(SIGQUIT, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	if	(argc != 3)
		{
		fprintf(stderr, "usage %s: symlist file\n", Pgm);
		exit(EX_USAGE);
		}
	fp = fopen(argv[2], "r+");
	if	(!fp)
		{
		fprintf(stderr, "%s: can't open '%s' for update\n", Pgm,
			argv[2]);
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
 * Now open the file containing the list of symbols to
 * relocate to the front of the symbol table.
*/
	fp2 = fopen(argv[1], "r");
	if	(!fp2)
		{
		fprintf(stderr, "%s: Can not open '%s'\n", Pgm, argv[1]);
		exit(EX_NOINPUT);
		}
	getsyms(fp2);

/*
 * Create the temporary files which will hold the new symbol table and the
 * new string table.  One temp file receives symbols _in_ the list,
 * another file receives all other symbols, and the last file receives the
 * new string table.
*/
	strcpy(sym1tmp, "/tmp/sym1XXXXXX");
	mktemp(sym1tmp);
	strcpy(sym2tmp, "/tmp/sym2XXXXXX");
	mktemp(sym2tmp);
	strcpy(strtmp, "/tmp/strXXXXXX");
	mktemp(strtmp);
	sym1fp = fopen(sym1tmp, "w+");
	sym2fp = fopen(sym2tmp, "w+");
	strfp = fopen(strtmp, "w+");
	if	(!sym1fp || !sym2fp || !strfp)
		{
		fprintf(stderr, "%s: Can't create %s, %s or %s\n", sym1tmp,
			sym2tmp, strtmp);
		exit(EX_CANTCREAT);
		}
	setbuf(sym1fp, fbuf2);
/*
 * Now position the executable to the start of the symbol table.  For each
 * symbol scan the list for a match on the symbol name.  If the
 * name matches write the symbol table entry to one tmp file, else write it
 * to the second symbol tmp file.
 *
 * NOTE: Since the symbol table is being rearranged the usefulness of
 * "local" symbols, especially 'register' symbols, is greatly diminished
 * Not that they are terribly useful in any event - especially the register
 * symbols, 'adb' claims to do something with them but doesn't.  In any
 * event this suite of programs is targeted at the kernel and the register
 * local symbols are of no use.  For this reason 'register' symbols are 
 * removed - this has the side effect of even further reducing the symbol 
 * and string tables that must be processed by 'nm', 'ps', 'adb' and so on.  
 * This removal probably should have been done earlier - in 'strcompact' or 
 * 'symcompact' and it may be in the future, but for now just do it here.
*/
	fseek(fp, symoff, L_SET);
	while	(nsyms--)
		{
		fread(&sym, sizeof (sym), 1, fp);
		if	(sym.n_type == N_REG)
			continue;
		if	(inlist(&sym))
			fwrite(&sym, sizeof (sym), 1, sym1fp);
		else
			fwrite(&sym, sizeof (sym), 1, sym2fp);
		}

/*
 * Position the executable file to where the symbol table starts.  Truncate
 * the file to the current position to remove the old symbols and strings.  Then
 * write the symbol table entries which are to appear at the front, followed
 * by the remainder of the symbols.  As each symbol is processed adjust the
 * string table offset and write the string to the strings tmp file.   
 *
 * It was either re-scan the tmp files with the symbols again to retrieve
 * the string offsets or simply write the strings to yet another tmp file.
 * The latter was chosen.
*/
	fseek(fp, symoff, L_SET);
	ftruncate(fileno(fp), ftell(fp));
	ltmp = sizeof (long);
	rewind(sym1fp);
	rewind(sym2fp);
	nsyms = 0;
	while	(fread(&sym, sizeof (sym), 1, sym1fp) == 1)
		{
		if	(ferror(sym1fp) || feof(sym1fp))
			break;
		oldname = strtab + (int)sym.n_un.n_strx;
		sym.n_un.n_strx = ltmp;
		len = strlen(oldname) + 1;
		ltmp += len;
		fwrite(&sym, sizeof (sym), 1, fp);
		fwrite(oldname, len, 1, strfp);
		nsyms++;
		}
	fclose(sym1fp);
	while	(fread(&sym, sizeof (sym), 1, sym2fp) == 1)
		{
		if	(ferror(sym2fp) || feof(sym2fp))
			break;
		oldname = strtab + (int)sym.n_un.n_strx;
		sym.n_un.n_strx = ltmp;
		len = strlen(oldname) + 1;
		ltmp += len;
		fwrite(&sym, sizeof (sym), 1, fp);
		fwrite(oldname, len, 1, strfp);
		nsyms++;
		}
	fclose(sym2fp);
/*
 * Next write the symbol table size longword followed by the
 * string table itself.
*/
	fwrite(&ltmp, sizeof (long), 1, fp);
	rewind(strfp);
	while	((c = getc(strfp)) != EOF)
		putc(c, fp);
	fclose(strfp);
/*
 * And last (but not least) we need to update the a.out header with
 * the correct size of the symbol table.
*/
	rewind(fp);
	xhdr.e.a_syms = nsyms * sizeof (struct nlist);
	fwrite(&xhdr.e, sizeof (xhdr.e), 1, fp);
	fclose(fp);
	free(strtab);
	cleanup();
	}

inlist(sp)
	register struct nlist *sp;
	{
	register int i;

	for	(i = 0; i < nsorted; i++)
		{
		if	(strcmp(strtab + (int)sp->n_un.n_strx, order[i]) == 0)
			return(1);
		}
	return(0);
	}

getsyms(fp)
	FILE	*fp;
	{
	char	asym[128], *start;
	register char *t, **p;

	for	(p = order; fgets(asym, sizeof(asym), fp) != NULL;)
		{
		if	(nsorted >= NUMSYMS)
			{
			fprintf(stderr, "%s: only doing %d symbols\n",
				Pgm, NUMSYMS);
			break;
			}
		for	(t = asym; isspace(*t); ++t)
			;
		if	(!*(start = t))
			continue;
		while	(*++t)
			;
		if	(*--t == '\n')
			*t = '\0';
		*p++ = strdup(start);
		++nsorted;
		}
	fclose(fp);
	}

void
cleanup()
	{
	if	(strtmp[0])
		unlink(strtmp);
	if	(sym1tmp[0])
		unlink(sym1tmp);
	if	(sym2tmp[0])
		unlink(sym2tmp);
	exit(EX_OK);
	}
