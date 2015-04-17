/*
 *	Program Name:   strcompact.c
 *	Date: February 12, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     21Jan94         1. Initial release into the public domain.
 *	2.0	12Feb94		2. Rewrite.  Use new utility program 'symdump'
 *				   and a multi-key sort to not only create
 *				   shared symbol strings but remove identical
 *				   symbols (identical absolute local symbols 
 *				   are quite common).  Execution speed was
 *				   speed up by about a factor of 4.
*/

/*
 * This program compacts the string table of an executable image by
 * preserving only a single string definition of a symbol and updating
 * the symbol table string offsets.  Multiple symbols having the same
 * string are very common - local symbols in a function often have the
 * same name ('int error' inside a function for example).  This program
 * reduced the string table size of the kernel at least 25%!
 *
 * In addition, local symbols with the same value (frame offset within
 * a function) are very common.  By retaining only a single '~error=2'
 * for example the symbol table is reduced even further (about 500 symbols
 * are removed from a typical kernel).
*/

#include <stdio.h>
#include <a.out.h>
#include <signal.h>
#include <string.h>
#include <sysexits.h>
#include <sys/file.h>

	char	*Pgm;
	char	*Sort = "/usr/bin/sort";
	char	*Symdump = "/usr/ucb/symdump";
static	char	strfn[32], symfn[32];

main(argc, argv)
	int	argc;
	char	**argv;
	{
	struct	nlist	sym;
	char	buf1[128], symname[64], savedname[64];
	struct	xexec	xhdr;
	int	nsyms, len;
	FILE	*symfp, *strfp, *sortfp;
register FILE	*fpin;
	long	stroff;
	unsigned short	type, value, ovly;
	void	cleanup();

	Pgm = argv[0];
	signal(SIGQUIT, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	if	(argc != 2)
		{
		fprintf(stderr, "%s: missing filename argument\n", Pgm);
		exit(EX_USAGE);
		}
	fpin = fopen(argv[1], "r+");
	if	(!fpin)
		{
		fprintf(stderr, "%s: can not open '%s' for update\n", 
			Pgm, argv[1]);
		exit(EX_NOINPUT);
		}
	if	(fread(&xhdr, 1, sizeof (xhdr), fpin) < sizeof (xhdr.e))
		{
		fprintf(stderr, "%s: premature EOF\n", Pgm);
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
		fprintf(stderr, "%s: '%s' stripped\n", Pgm, argv[1]);
		exit(EX_OK);
		}

	strcpy(strfn, "/tmp/strXXXXXX");
	mktemp(strfn);
	strcpy(symfn, "/tmp/symXXXXXX");
	mktemp(symfn);

	sprintf(buf1, "%s %s | %s +0 -1 +1n -2 +2n -3 +3n -4 -u", Symdump,
		argv[1], Sort);
	sortfp = popen(buf1, "r");
	if	(!sortfp)
		{
		fprintf(stderr, "%s: symdump | sort failed\n", Pgm);
		exit(EX_SOFTWARE);
		}
	symfp = fopen(symfn, "w+");
	strfp = fopen(strfn, "w+");
	if	(!symfp || !strfp)
		{
		fprintf(stderr, "%s: can't create %s or %s\n", symfn, strfn);
		exit(EX_CANTCREAT);
		}

	stroff = sizeof (long);
	len = 0;
	nsyms = 0;
	while	(fscanf(sortfp, "%s %u %u %u\n", symname, &type, &ovly,
			&value) == 4)
		{
		if	(strcmp(symname, savedname))
			{
			stroff += len;
			len = strlen(symname) + 1;
			fwrite(symname, len, 1, strfp);
			strcpy(savedname, symname);
			}
		sym.n_un.n_strx = stroff;
		sym.n_type = type;
		sym.n_ovly = ovly;
		sym.n_value = value;
		fwrite(&sym, sizeof (sym), 1, symfp);
		nsyms++;
		}
	stroff += len;

	pclose(sortfp);
	rewind(symfp);
	rewind(strfp);

	if	(nsyms == 0)
		{
		fprintf(stderr, "%s: No symbols - %s not modified\n", argv[1]);
		cleanup();
		}

	fseek(fpin, N_SYMOFF(xhdr), L_SET);

/*
 * Now append the new symbol table.  Then write the string table length
 * followed by the string table.  Finally truncate the file to the new
 * length, reflecting the smaller string table.
*/
	copyfile(symfp, fpin);
	fwrite(&stroff, sizeof (long), 1, fpin);
	copyfile(strfp, fpin);
	ftruncate(fileno(fpin), ftell(fpin));

/*
 * Update the header with the correct symbol table size.
*/
	rewind(fpin);
	xhdr.e.a_syms = nsyms * sizeof (sym);
	fwrite(&xhdr, sizeof (xhdr.e), 1, fpin);

	fclose(fpin);
	fclose(symfp);
	fclose(strfp);
	cleanup();
	}

copyfile(in, out)
	register FILE *in, *out;
	{
	register int c;

	while	((c = getc(in)) != EOF)
		putc(c, out);
	}

fatal(str)
	char	*str;
	{

	if	(strfn[0])
		unlink(strfn);
	if	(symfn[0])
		unlink(symfn);
	if	(!str)
		exit(EX_OK);
	fprintf(stderr, "%s: %s\n", str);
	exit(EX_SOFTWARE);
	}

void
cleanup()
	{
	fatal((char *)NULL);
	}
