/*
 * netbind
 *
 * 1/6/95 -- remove 8 character limit on undefined symbol printf. sms.
 * 1/8/94 -- revised for new object file format. sms.
 *
 * Resolve undefined inter-address-space references.
 *
 * Usage:
 *	netbind unix.o netnix.o
 *
 * Produces two files as output:
 *
 *	d.unix.s	-- definitions of all symbols referenced by
 *			   unix.o but defined in netnix.o
 *	d.netnix.s	-- definitions of all symbols referenced by
 *			   netnix.o but defined in unix.o
 */
#include <sys/param.h>
#include <sys/file.h>
#include <a.out.h>
#include <stdio.h>

extern	char	*strdup(), *rindex();

	struct	xexec	refExec, defExec;

#define	MAXSYMLEN 32

#define	NSYMS	200
struct symbol {
	char	*s_name;
	u_int	s_value;
	u_int	s_type;
} symtab[NSYMS], *symfree;

main(argc, argv)
	int argc;
	char **argv;
{
	if (argc != 3) {
		printf("usage: %s unix.bo netnix.bo\n", *argv);
		exit(2);
	}
	resolve(argv[1], argv[2]);	/* Produces d.unix.s */
	resolve(argv[2], argv[1]);	/* Produces d.netnix.s */
	exit(0);
}

/*
 * resolve --
 *
 * Resolve references made by ref to symbols defined in def.
 *
 * Side effects:
 *	Prints a list of undefined symbols if there are any.
 *	Produces the .s file corresponding to ref.
 *	Adds the number of undefined symbols encountered to undef.
 */
resolve(ref, def)
	char *ref;		/* Name of file referencing symbols */
	char *def;		/* Name of file defining symbols */
{
	FILE *refIN, *defIN, *refSTR, *defSTR, *refOUT, *openobj();
	struct nlist syment;
	off_t stroff_ref, stroff_def;
	int nundef;
	char *rp, refout[MAXPATHLEN], name[MAXSYMLEN + 2];

	bzero(name, sizeof (name));
	if (rp = rindex(ref, '.'))
		*rp = '\0';
	(void)sprintf(refout, "d.%s.s", ref);
	if (rp)
		*rp = '.';
	if ((refIN = openobj(ref, &refExec, &refSTR)) == NULL)
		return;
	if ((defIN = openobj(def, &defExec, &defSTR)) == NULL) {
		fclose(refIN);
		fclose(refSTR);
		return;
	}
	if ((refOUT = fopen(refout, "w")) == NULL) {
		perror(refout);
		fclose(refIN);
		fclose(refSTR);
		fclose(defIN);
		fclose(defSTR);
		return;
	}

	/*
	 * Actually generate the .s file needed.
	 * Assumes that refExec and defExec have been set up to
	 * hold the a.out headers for the reference and definition
	 * object files.
	 *
	 * Algorithm:
	 *	Build a table of undefined symbols in refIN.
	 *	Define them by reading the symbol table of defIN.
	 *	Generate the .s file to refOUT.
	 */
	syminit();
	nundef = 0;

	/* Find the undefined symbols */
	stroff_ref = N_STROFF(refExec);
	fseek(refIN, N_SYMOFF(refExec), L_SET);
	while (fread(&syment, sizeof(syment), 1, refIN) > 0) {
		if (syment.n_type == (N_EXT|N_UNDF) && syment.n_value == 0)
			{
			fseek(refSTR, stroff_ref + syment.n_un.n_strx, L_SET);
			fread(name, sizeof (name), 1, refSTR);
			if (strcmp(name, "_end") &&
			    strcmp(name, "_etext") &&
			    strcmp(name, "_edata"))
				{
				nundef++;
				syment.n_un.n_name = name;
				symadd(&syment);
				}
			}
	}

	/* Define the undefined symbols */
	stroff_def = N_STROFF(defExec);
	fseek(defIN, N_SYMOFF(defExec), L_SET);
	while (fread(&syment, sizeof(syment), 1, defIN) > 0) {
		if ((syment.n_type & N_EXT) == 0)
			continue;
		fseek(defSTR, stroff_def + syment.n_un.n_strx, L_SET);
		fread(name, sizeof (name), 1, defSTR);
		syment.n_un.n_name = name;
		nundef -= symdef(&syment, &defExec.e);
	}

	/* Any undefined symbols left? */
	if (nundef > 0) {
		printf("%s: %d symbols undefined:\n", ref, nundef);
		symundef();
	}
	symprdef(refOUT);

	fclose(refSTR);
	fclose(defSTR);
	fclose(refIN);
	fclose(defIN);
	fclose(refOUT);
}

/*
 * openobj --
 *
 * Open the indicated file, check to make sure that it is a
 * valid object file with a symbol table, and read in the a.out header.
 *
 * Returns a pointer to an open FILE if successful, NULL if not.
 * Prints its own error messages.
 */
FILE *
openobj(filename, exechdr, strfp)
	char *filename;
	struct xexec *exechdr;
	FILE **strfp;
{
	register FILE *f;

	if (!(f = fopen(filename, "r"))) {
		perror(filename);
		return((FILE *)NULL);
	}
	*strfp = fopen(filename, "r");		/* for strings */
	if (fread(exechdr, sizeof(*exechdr), 1, f) <= 0) {
		printf("%s: no a.out header\n", filename);
		goto bad;
	}
	if (N_BADMAG(exechdr->e)) {
		printf("%s: bad magic number\n", filename);
		goto bad;
	}
	if (exechdr->e.a_syms == 0) {
		printf("%s: no symbol table\n", filename);
		goto bad;
	}
	return(f);
bad:	fclose(f);
	fclose(*strfp);
	return((FILE *)NULL);
}

/* -------------------- Symbol table management ----------------------- */

/*
 * syminit --
 *
 * Clear and initialize the symbol table.
 */
syminit()
{
	symfree = symtab;
}

/*
 * symadd --
 *
 * Add a symbol to the table.
 * We store both the symbol name and the symbol number.
 */
symadd(np)
	struct nlist *np;
{
	if (symfree >= &symtab[NSYMS]) {
		printf("Symbol table overflow.  Increase NSYMS.\n");
		exit (1);
	}
	symfree->s_name = strdup(np->n_un.n_name);
	if	(!symfree->s_name) 
		{
		printf("netbind: out of memory for symbol strings\n");
		exit(1);
		}
	symfree->s_type = N_UNDF;
	symfree->s_value = 0;
	symfree++;
}

/*
 * symdef --
 *
 * Use the supplied symbol to define an undefined entry in
 * the symbol table.
 *
 * Returns 1 if the name of the symbol supplied was found in
 * the table, 0 otherwise.
 */
symdef(np, ep)
	struct nlist *np;
	struct exec *ep;
{
	register struct symbol *sp;

	for (sp = symtab; sp < symfree; sp++)
		if (!strcmp(sp->s_name, np->n_un.n_name)) {
			int type = (np->n_type & N_TYPE);

			switch (type) {
			case N_TEXT:
			case N_ABS:
				sp->s_type = N_EXT|N_ABS;
				sp->s_value = np->n_value;
				break;
			case N_DATA:
			case N_BSS:
				sp->s_type = N_EXT|N_ABS;
				if (ep->a_flag)
					sp->s_value = np->n_value;
				else
					sp->s_value = np->n_value - ep->a_text;
				break;
			case N_UNDF:
				return(0);
			default:
				printf("netbind: symbol %s, bad type 0x%x\n",
				    np->n_un.n_name, np->n_type);
				exit(1);
			}
			return (1);
		}
	return(0);
}

/*
 * symundef --
 *
 * Print all undefined symbols in the symbol table.
 * First sorts the table before printing it.
 */
symundef()
{
	register struct symbol *sp;
	int scmp();

	qsort(symtab, symfree - symtab, sizeof(struct symbol), scmp);
	for (sp = symtab; sp < symfree; sp++)
		if ((sp->s_type & N_TYPE) == N_UNDF)
			printf("%s\n", sp->s_name);
}

scmp(s1, s2)
	register struct symbol *s1, *s2;
{
	return(strcmp(s1->s_name, s2->s_name));
}

/*
 * symprdef --
 *
 * Output all defined symbols in the symbol table.
 * First sorts the table before printing it.
 */
symprdef(refOUT)
	FILE *refOUT;
{
	register struct symbol *sp;
	int scmp();

	qsort(symtab, symfree - symtab, sizeof (struct symbol), scmp);
	for (sp = symtab; sp < symfree; sp++)
		if ((sp->s_type & N_TYPE) != N_UNDF) {
			fprintf(refOUT, ".globl %s\n", sp->s_name);
			fprintf(refOUT, "%s = %o\n", sp->s_name, sp->s_value);
		}
}
