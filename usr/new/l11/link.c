# include	"link.h"
char *sprintf(),*strcpy(), *ctime(), *strsub(), *tack(), *lalloc();
WORD	getword();
#include	<signal.h>

/******************** variables with global scope ************************/


struct psect	*Tex_root = NULL;	/* root of ins psects */
struct psect	*Tex_end = NULL;	/* last structure of ins list */
struct psect	*Dat_root = NULL;	/* root of data psects */
struct psect	*Dat_end = NULL;	/* last structure of data list */
struct psect	*Bss_root = NULL;	/* root of bss psects */
struct psect	*Bss_end = NULL;	/* last structure of bss list */
struct objfile	*File_root = NULL;	/* root of object file link-list */
struct symbol	*Sym_root = NULL;	/* root of symbol table */
struct g_sect	*Gsect_root = NULL;	/* root of tree of global psects */
WORD 		Maxabs = 0;		/* maximum size of absolute files,
					** also low limit of relocatable code */
WORD	R_counter;			/* relocation counter, used in 
					** assigning relocation constants,
					** also high limit of relocatable
					** code after relocation */
long		Seekoff;		/* offset for seeking in out file */
char		Do_410 = 0;		/* boolean for out file format */
int		Verbose	=0;		/* Boolean for verbose commentary */
char		Do_411 = 0;		/* boolean for out file format */
char		Do_map = 0;		/* boolean for printing load map */
char		Do_lpr_map = 0;		/* boolean for line printing map */
char		Do_bits = 0;		/* boolean for including relocation 
					** bits in .out file */ 
char		C_rel = 0;		/* boolean for having out file end
					** with '.o' */
char		Do_kludge = 0;		/* boolean for writing global sybmbols
					** in the out file with a preceding
					** underscore character */
char		Do_silent = 0;		/* boolean for not printing zero
					** errors message */
char		Do_table = 1;		/* boolean for including symbol table
					** in out file */
char		No_locals = 0;		/* boolean for not including local
					** symbols in the out table */
char		Do_odt = 0;		/* boolean for including odt module */
WORD		Transadd = 1;		/* transfer address */
struct objfile  *Transfile;		/* object file of transfer address */
char		Transsect[7];		/* program section of trans address */
WORD Tex_size;				/* for out file */
WORD Dat_size;				/* for out file */
WORD Bss_size;				/* for out file */
char		*Outbase;		/* out file name without '.out' */
char		*Outname = NULL;	/* name of out file */
char 		*Mapname;		/* name of map file */
FILE		*Mapp = NULL;		/* pointer for map file */
char		Erstring[80];		/* buffer for error messages */
int		Nerrors = 0;		/* the number of user errors */
char		No_out = 0;		/* boolean for no out file */
char		Scanerr = 0;		/* boolean for error in arguments */

/**********************  main  ********************************************/


main(argc, argv)
int	argc;
char	*argv[];
{
	scanargs(argc, argv);
	pass1();
	relocate();
	relsyms(Sym_root);
	post_bail();
	if (Do_map)
		printmap();
	if (No_out)
		exit(0);
	warmup();
	pass2();
	loose_ends();
}


/*********************  scanargs  ******************************************/


scanargs(argc, argv)
int	argc;
char	*argv[];
{
	register char			*s;
	char				*r;
	register struct objfile		*end;	/* last file in link-list */

	struct objfile			*newfile();	/* allocaters */

	while (--argc)
	{
		s = *++argv;
		if (*s == '-')
		{
			s++;
			if ((r = strsub(s, "na:")) != NULL)
				Outname = tack(r, ".out");
			else if (!strcmp(s, "ls") || !strcmp(s, "mp"))
				Do_map = 1;
			else if (!strcmp(s, "lp"))
				Do_map = Do_lpr_map = 1;
			else if (!strcmp(s, "od"))
				Do_odt = 1;
			else if (!strcmp(s, "r"))
				Do_bits = 1;
			else if (!strcmp(s, "c"))
				Do_bits = C_rel = 1;
			else if (!strcmp(s, "K"))
				Do_bits = C_rel = Do_kludge = 1;
			else if (!strcmp(s, ""))
				Do_silent = 1;
			else if (!strcmp(s, "ns"))
				Do_table = 0;
			else if (!strcmp(s, "go"))
				No_locals = 1;
			else if (!strcmp(s, "no"))
				No_out = 1;
			else if (!strcmp(s, "n"))
				Do_410 = 1;
			else if (!strcmp(s, "i"))
				Do_411 = 1;
			else if(!strcmp(s, "v"))
				Verbose = 1;
			else 
			{
				fprintf(stderr, "Unrecognizable argument: -%s\n", s);
				Scanerr = 1;
			}
		}
		else
		{
			if (File_root == NULL)
				end = File_root = newfile();
			else
			{
				end->nextfile = newfile();
				end = end->nextfile;
			}
			s = tack(s, ".obj");
			end->fname = s;		
		}
	}

	if (File_root == NULL)	
	{
		fprintf(stderr, "Error: no object files given\n");
		exit(1);
	}

	if (Do_odt)
	{
		end->nextfile = newfile();
		end->nextfile->fname = "/usr/libdata/odt.obj";
	}

	if (Scanerr)
		exit(1);
	outnames();
}


/***************************  strsub  ***************************************/


char	*strsub(s, t)	/* if t is the initial part of s then return a pointer
			** to the rest of s, else return NULL */
register char 	*s;
register char	*t;
{
	register char *q;
	
	q = t;
	while (*t)
		if (*s++ != *t++)
			return (NULL);

	if (*s == '\0')	/* check for no filename */
	{
		fprintf(stderr, "Argument error: a filename should be contiguous with -%s\n", q);
		Scanerr = 1;
		return(NULL);
	}
	else
		return (s);
}


/****************************  outnames  ************************************/


outnames()	/* determine names of output files */

{
	if (Outname == NULL)
	{
		Outbase = lalloc(strlen(File_root->fname) + 1);
		strcpy(Outbase, File_root->fname);
		strip(Outbase, ".obj");
	}
	else
	{
		Outbase = lalloc(strlen(Outname) + 5);
		strcpy(Outbase, Outname);
		strip(Outbase, ".out");
		strip(Outbase, ".o");
	}

	if (C_rel)
		Outname = tack(Outbase, ".o");
	else
		Outname = tack(Outbase, ".out");

	if (Do_map)
		Mapname = tack(Outbase, ".map");
}


/***************************  newpsect  ************************************/


struct psect	*newpsect()	/* allocate and initialize a new psect
				** structure */
{
	register struct psect	*p;

	p = (struct psect *) lalloc(sizeof (struct psect));
	p->next = p->pssame = p->obsame = NULL;
	p->slist = NULL;
	return (p);
}


/***********************  newsymbol  ***************************************/


struct symbol	*newsymbol()	/* allocate and initialize new symbol 
				** structure */
{
	register struct symbol	*p;

	p = (struct symbol *) lalloc(sizeof (struct symbol));
	p->prsect = (struct psect *)NULL;
	p->right = p->left = (struct symbol *)NULL;
	return (p);
}


/**********************  newfile  ******************************************/


struct objfile	*newfile()	/* allocate and initialize new psect 
				** structure */
{
	register struct objfile		*p;

	p = (struct objfile *) lalloc(sizeof (struct objfile));
	p->nextfile = NULL;
	p->psect_list = NULL;
	p->ver_id = NULL;
	return (p);
}


/****************************  new_gsect  ***********************************/


struct g_sect	*new_gsect()

{
	register struct g_sect	*p;

	p = (struct g_sect *) lalloc(sizeof(struct g_sect));
	p->leftt = p->rightt = NULL;
	return (p);
}


/*********************  pass1  *********************************************/


pass1()

{
	char			name[7];	/* string from M11 */
	char			attr;		/* attribute from M11 */
	register int		type;		/* type of string */
	int			value;		/* from M11 */
	register struct objfile *of;		/* current object file */
	register struct psect	*ps;		/* current psect */
	struct symbol		*sym;		/* current and last symbol
						** in symbol list */
	struct symbol		**nextsym;	/* pointer to pointer which
						** should point to next symbol
						** in the psect list */
	struct psect		*newpsect();
	struct symbol		*newsymbol();

	of = File_root;
	while (of != NULL)
	{
		ch_input(of->fname, HEADER);	/* change input */

		while(morebytes())
		{
			dc_symbol(name);
			attr = getbyte();
			type = getbyte();
			value = getword();
			switch(type)
			{
			  case 0:	/* program name */
				strcpy(of->pname, name);
				break;

			  case 1:	/* program section */
			  case 5:	/* program section */
			  if(Verbose)
				fprintf(stderr,"gsd%d <%6s> %o\n",
					type, name, attr);

				/* increment size if odd */
				value += (value % 2) ? 1 : 0;
				/* place psect at end of link-list for objfile */
				if (of->psect_list == NULL)
					ps = of->psect_list = newpsect();
				else
				{
					ps->obsame = newpsect();
					ps = ps->obsame;
				}
				strcpy(ps->name, name);
				if (Do_410)
					/* if the psect is sharable and data
					** make it instruction, this is an old
					** L11 kludge. */
					{
					if ((attr & SHR) && !(attr & (INS | BSS)))
						ps->type = attr | INS;
					else
						ps->type = attr;
					}
				else
					ps->type = attr;
				ps->nbytes = value;
				nextsym = &(ps->slist);

				if (!(attr & REL))	/* if psect is absolute */
				{
					ps->rc = 0;
					Maxabs = (value > Maxabs) ? value : Maxabs;
				}
					/* else put psect in proper ins-dat-bss
					** link-list */
				else
					place_global(ps);
				break;

			  case 2:	/* this case has not been observed
					** and is assumed to be un-used */
				lerror("internal symbol table");
				break;

			  case 3:	/* transfer address */
				if (value == 1)		/* if 1 bogus */
					break;
				if (Transadd != 1)	/* transfer address
							** already specified */
					fprintf(stderr, "Warning: more than \
						one transfer address specified\n");
				Transadd = value;
				Transfile = of;
				strcpy(Transsect, name);
				break;

			  case 4:	/* global symbol */
				/* make structure and place in obj file list */
				sym = newsymbol();
				sym->prsect = ps;
				strcpy(sym->name, name);
				sym->type = attr;
				sym->value = value;
				if (attr & DEF)	/* if the symbol is defined, 
						** place in link-list of
						** symbols */
				{
					*nextsym = sym;
					nextsym = &(sym->symlist);
				}
				/* place in symbol table */
				table(&Sym_root, sym);
				break;

			  case 6:	/* version identification */
				of->ver_id = lalloc(7);
				strcpy(of->ver_id, name);
				break;

			  default:
				lerror("header type out of range");
			}
		}
		of = of->nextfile;
	}
}


/*****************************  place_global  ******************************/


place_global(ps)		/* try to place the given program section
				** in proper ins - dat - bss link-list by
				** finding it in the global psect tree. if 
				** it is not there then add to tree and place
				** it through place_local */
register struct psect	*ps;
{
	register struct	g_sect 	**ptr;
	register int		cond;

	ptr = &Gsect_root;
	while (*ptr != NULL)
	{
		if ( !(cond = strcmp(ps->name, (*ptr)->name)))
		{
			if (ps->type != (*ptr)->type)
			{
				fprintf(stderr, "psect <%6s> attribute clash: ", ps->name);
				fprintf(stderr, " already has %3o, now declared %3o.  Using %3o\n", 0377 & (*ptr)->type, 0377 & ps->type, 0377 & (*ptr)->type);
				ps->type = (*ptr)->type;
			}
			/* place psect in list of sames */
			(*ptr)->last_sect->pssame = ps;
			(*ptr)->last_sect = ps;
			return;
		}
		else if (cond < 0)
			ptr = &((*ptr)->leftt);
		else
			ptr = &((*ptr)->rightt);
	}

	/* global section cannot be found in tree so make a new node for
	** it and place it as a local */

	*ptr = new_gsect();
	strcpy((*ptr)->name, ps->name);
	(*ptr)->type = ps->type;
	(*ptr)->last_sect = ps;
	place_local(ps);
}


/*************************  place_local  *******************************/


place_local(ps)			/* place psect at end of its UNIX section
				** type link-list */
register struct psect	*ps;
{
	register type = ps->type;
	
	if( !(type & REL))		/* asect */
	{
		fprintf(stderr, "Don't know what to do with .asect yet\n");
	}
	if (type & INS)	/* instruction psect */
	{
		if (Tex_root == NULL)
			Tex_root = Tex_end = ps;
		else
		{
			Tex_end->next = ps;
			Tex_end = ps;
		}
	}
	else if (type & BSS)	/* bss psect */
	{
		if (Bss_root == NULL)
			Bss_root = Bss_end = ps;
		else 
		{
			Bss_end->next = ps;
			Bss_end = ps;
		}
	}
	else				/* data psect */
	{
		if (Dat_root == NULL)
			Dat_root = Dat_end = ps;
		else
		{
			Dat_end->next = ps;
			Dat_end = ps;
		}
	}
}


/*************************  table  **************************************/


table(root, new)	/* place new symbol structure in symbol table tree */

register struct symbol	*root[];	/* pointer to root pointer of tree */
register struct symbol	*new;		/* pointer to symbol (structure) to
					** be added */
{
	register int	cond;

	for (;;)	/* repeat until break */
	{
		if (*root == NULL)	/* empty tree */
		{
			/* set root, return */
			*root = new;
			return;
		}
		/* check for same name */
		else if ((cond = strcmp(new->name, (*root)->name)) == 0)
		{
			/* new symbol and one in table have same name */
			if (new->type & DEF)	/* ignore new if undefined */
			{
				if (isdef(*root))    /* both defined */
				{
					if(isabs(*root) && (new->value ==
						(*root)->value))
						fprintf(stderr, "Warning: abs sym %s defined as %06o twice\n", new->name, new->value);
					else {

					sprintf(Erstring, "%s defined twice", new->name);
					uerror(Erstring);
					}
				}
				/* else	/* get rid of old undefined symbol */
				{
					new->left = (*root)->left;
					new->right = (*root)->right;
					*root = new;
				}
			}
			return;
		}
		/* else branch */
		else if (cond < 0)
			root = &((*root)->left);
		else
			root = &((*root)->right);
	}
}


/*****************************  relocate  ********************************/


# define	_8K	020000


relocate()	/* assign relocation constants for all relocatable psects */

{
	register unsigned temp;
	struct outword	trans_rc;

	R_counter = Maxabs;	/* set relocation counter to follow
				** absolute sections */
	asgn_rcs(Tex_root);
	/* text size = absolute and ins psects */
	Tex_size = R_counter;

	if (Do_410)
	{
		temp = Tex_size + _8K;
		temp &= ~(_8K - 1);
		R_counter = temp;

		Seekoff = (long) Tex_size - (long )R_counter;
	}
	else if (Do_411)
	{
		R_counter = temp = 0;
		Seekoff = Tex_size;
	}
	else
	{
		temp = Tex_size;
		Seekoff = 0;
	}

	asgn_rcs(Dat_root);
	Dat_size = (int)R_counter - (int)temp;
	temp = R_counter;

	asgn_rcs(Bss_root);
	Bss_size = (int)R_counter - (int)temp;

	/* relocate transfer address if defined */
	if (Transadd != 1)	/* if defined */
	{
		get_rc(&trans_rc, Transfile, Transsect);
		Transadd += trans_rc.val;
	}

	if (Do_bits && Maxabs)
	{
		uerror("absolute section present - relocation bits suppressed");
		Do_bits = 0;
	}
}


/***************************  asgn_rcs  ************************************/



asgn_rcs(p)	/* assign relocation constants to
		** the psects pointed to.  this routine uses the
		** variable R_counter which contains the address of the 
		** next available space in memory */
register struct psect 	*p;	/* called with ins-dat-bss root */

{
	register int		size;	/* addendum to R_counter */
	register struct psect 	*d;	/* temporary pointer used to access
					** same psects */
	for (;;)	/* repeat until break */
	{
		if (p == NULL)	/* end of list, break */
			break;

		/* set relocation constant to R_counter */
		p->rc = R_counter;

		/* set size initially to size of psect.  size is not added
		** to R_counter at this point because of the possibility
		** that the psect is part of a group of overlaid psects */

		size = p->nbytes;

		/* check for same psect(s) */
		if (p->pssame != NULL)
		{
			/* set d to same psect */
			d = p;

			if (p->type & OVR)	/* if the sames are overlaid */
				/* travel through sames.
				** set the rc's of all sames to R_counter.
				** size equals maximum psect size */
				do
				{
					d = d->pssame;
					d->rc = R_counter;
					size = (size > d->nbytes) ? size : d->nbytes;
				} while (d->pssame != NULL);

			else		/* sames are concatenated */
				/* travel through sames 
				** add size to R_counter before assigning
				** to rc for each psect.
				** redefine size as number of bytes in psect */
				do
				{
					d = d->pssame;
					d->rc = (R_counter += size);
					size = d->nbytes;
				} while (d->pssame != NULL);

			R_counter += size;

			/* insert a psect structure at bottom of sames list to
			** to facilitate '^p' M11 directives */

			d->pssame = newpsect();
			d = d->pssame;
			d->rc = p->rc;
			d->type = p->type;
			d->nbytes = R_counter - p->rc;
		}
		else
			R_counter += size;
		p = p->next;
	}
}


/*************************  relsyms  **************************************/


relsyms(sym)		/* relocate global symbols */

register struct symbol	*sym;	
{
	if (sym == NULL)
		return;
	sym->value += sym->prsect->rc;
	relsyms(sym->left);
	relsyms(sym->right);
}


/*************************  printmap  ***********************************/


printmap()
{
	register struct objfile	*op;
	register struct psect 	*pp;
	int			tvec[2];	/* time vector */
	static char		dashes[] = "----------------";

	Mapp = fopen(Mapname, "w");
	if (Mapp == NULL)
		fprintf(stderr, "%s not accessible\n", Mapname);
	else
	{
		long datstart = Tex_size;
		
		if(Do_411) datstart = 0L;
		
		/* print map header */
		time(tvec);
		fprintf(Mapp, "%s    Linker-11 version 22may79    %s\n", Outname, ctime(tvec));
		fprintf(Mapp, "Magic number:     %o\t",
				Do_410 ? 0410 : ( Do_411 ? 0411 : 0407) );
		if(Do_410) fprintf(Mapp, "(Shared text)\n");
		else if (Do_411) fprintf(Mapp,"(Separate I & D)\n");
		else fprintf(Mapp, "(Executable)\n");
		fprintf(Mapp, "\tStart\tLimit\tLength\n");
		brag("Text", 0L, (long)Tex_size, (Do_410 || Do_411)?"Shareable" : "");
		brag("Data", datstart, (long)Dat_size,  "");
		brag("Bss", (long)(datstart+Dat_size), (long)Bss_size,  "");
		fprintf(Mapp, "transfer address: %06o\n", Transadd);

		for (op = File_root; op != NULL; op = op->nextfile)
		{
			fprintf(Mapp, "%s\n", dashes);
			fprintf(Mapp, "module:  %s", op->pname);
			if (op->ver_id != NULL)
				fprintf(Mapp, "    %s", op->ver_id);
			fprintf(Mapp, "\nsection          orgin      size\n");
			pp = op->psect_list;
			while (pp != NULL)
			{
				fprintf(Mapp, "<%s>        ", pp->name);
				fprintf(Mapp, "%06o     %06o", pp->rc, pp->nbytes);
				prattr(pp->type);
				dump_symlist(pp->slist);
				pp = pp->obsame;
			}
		}
		fprintf(Mapp, "%s", dashes);	
		if (dump_undefs(Sym_root, 0))
			fprintf(Mapp, "\n%s\n", dashes);
		else
			fprintf(Mapp, "\n");
	}
}

brag(s, start, size, tag)
	char *s;
	long start, size;
	char *tag;
{
	long stop = 0177777 & (size + start);

	fprintf(Mapp, "%s:\t%06O\t%06O\t%06O\t%s\n", s, start, stop, size, tag);
}

prattr(x)
{
	fprintf(Mapp, "\t");
	attr(x&SHR, "shr, ", "prv, ");
	attr(x&INS, "ins, ", "");
	attr(x&BSS, "bss, ", "");
	attr( !((x&BSS) || (x&INS)), "dat,", "");
	attr(x&REL, "rel, ", "abs, ");
	attr(x&OVR,"ovr, ", "con, ");
	attr(x&GBL,"gbl", "loc");
}
attr(x, s, t)
	register x;
	register char *s, *t;
{
	if(x) fprintf(Mapp, s);
	else  fprintf(Mapp, t);
}
/*****************************  dump_symlist  *******************************/


dump_symlist(sym)	/* write to the map file the symbol list for a psect */
register struct symbol	*sym;
{
	register int	i;

	for (i = 0; sym != NULL; sym = sym->symlist)
	{
		fprintf(Mapp, "%s     ", (i++ % 3) ? "" : "\n     ");
		fprintf(Mapp, "%s %06o", sym->name, sym->value);
		fprintf(Mapp, " %c", (sym->type & REL) ? 'r' :'a');
	}
	fprintf(Mapp, "\n");
}


/*****************************  dump_undefs  *******************************/


dump_undefs(sym, n)	/* dump into map file all undefined global symbols */
struct symbol	*sym;
int		n;
{
	if (sym == NULL)
		return (n);
	n = dump_undefs(sym->left, n);
	if ( !(sym->type & DEF))
	{
		fprintf(Mapp, "%s     ", (n++ % 3) ? "" : "\n     ");
		fprintf(Mapp, "%s ****** u", sym->name);
	}
	n = dump_undefs(sym->right, n);
	return (n);
}


/**************************  post_bail  *************************************/


post_bail()	/* set interrupt routine to bail_out */

{
	extern	bail_out();

	sigx(1);
	sigx(2);
	sigx(3);
	sigx(15);
}
sigx(n)
{
	extern	bail_out();

	if(signal(n, SIG_IGN) != SIG_IGN)
		signal(n, bail_out);
}
