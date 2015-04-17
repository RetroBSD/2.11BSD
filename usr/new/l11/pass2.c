# include	"link.h"
# define	MAXREG		8
WORD	getword();
char *sprintf();


/******************** variables with global scope ************************/


extern struct objfile	*File_root;	/* root of object file link-list */
extern struct symbol	*Sym_root;	/* root of symbol table */
extern struct g_sect	*Gsect_root;	/* root of global psect tree */
WORD	Maxabs;			/* maximum size of absolute files,
					** also low limit of relocatable code */
extern WORD	R_counter;		/* relocation counter, used in 
					** assigning relocation constants,
					** also high limit of relocatable
					** code after relocation */
extern long	Seekoff;		/* offset for seeking in out file */
extern char	Do_410;			/* out file format boolean */
extern char	Do_411;			/* out file format boolean */
extern char	Do_map;			/* boolean for printing load map */
extern char	Do_lpr_map;		/* boolean for line printing map */
extern char	Do_bits;		/* boolean for including relocation 
					** bits in .out file */ 
extern char	Do_kludge;		/* boolean to write global symbols in
					** out file table with underscore */
extern char	Do_silent;		/* boolean for not printing zero
					** errors message */
extern char	Do_table;		/* boolean for including symbol table
					** in out file */
extern char	No_locals;		/* boolean for not including local
					** symbols in out table */
extern WORD	Transadd;		/* transfer address */
int		T_counter;		/* used for assigning numbers to the
					** global symbols so that they may be
					** referenced in relocation bits */
int		Glc;			/* global location counter */
struct outword	Curr;			/* Curr.val is the current relocation
					** constant, and Curr.rbits is the 
					** current relocation bits */
WORD	Tex_size;		/* for out file */
WORD	Dat_size;		/* for out file */
WORD	Bss_size;		/* for out file */
char	*Outname;		/* name of out file */
FILE		*Outp = NULL;		/* pointer for out file */
extern char	Erstring[80];		/* buffer for error messages */
extern int	Nerrors;		/* the number of user errors */
FILE		*Bitp = NULL;		/* temporary file for relocation bits */
char		Bitname[20];		/* name of temporary bit file */
FILE		*Symp = NULL;		/* temporary file for symbol table */
char		Symname[20];		/* name of temporary file for symbol table */
char	*Mapname;		/* name of the map file */
FILE	*Mapp;			/* pointer for map file */
char	No_out;			/* boolean for no out file */
char		Undefineds = 0;		/* boolean, set if there are any 
					** references to undefined global 
					** symbols in the out file */
struct outword	Vreg[MAXREG];		/* virtual registers */

/**************************  warmup  ****************************************/


warmup()	/* get ready for pass 2: open out file and write header,
		** open temporary file for out file
		** symbol table and write global variables to it */

{
	WORD h[8];

	Outp = fopen(Outname, "w");
	if (Outp == NULL)
	{
		sprintf(Erstring, "%s not accessible\n", Outname);
		lerror(Erstring);
	}
	fseek(Outp, 15L+(long)Tex_size+(long)Dat_size, 0);
	putc(0, Outp);

	/* write header for out file */
	if (Do_410)
		h[0] = 0410;
	else if (Do_411)
		h[0] = 0411;
	else
		h[0] = 0407;
	h[1] = Tex_size;
	h[2] = Dat_size;
	h[3] = Bss_size;
	h[4] = 0;
	h[5] = Transadd;
	h[6] = 0;
	h[7] = Do_bits ? 0 : 1;
	
	fseek(Outp, 0L, 0);
	fwrite(h, 8, 2, Outp);

	if (Do_table)
	{
		sprintf(Symname, "/tmp/l11.sym.%d", getpid());
		Symp = fopen(Symname, "w");
		if (Symp == NULL)
			lerror("can't open symbol table file");
		else
		{
			T_counter = 0;	/* initialize counter for numbering global
					** symbols */
			dump_tree(Sym_root);
		}
	}

	if (Do_bits)
	{
		sprintf(Bitname, "/tmp/l11.bit.%d", getpid());
		Bitp = fopen(Bitname, "w");
		if (Bitp == NULL)
			lerror("can't open relocation bits file");
	}
}


/*************************  dump_tree  **************************************/


dump_tree(sym)		/* dump the sub-tree of symbols pointed to by *sym and
			** number its contents for future reference to
			** undefined symbols */

register struct symbol	*sym;
{
	if (sym == NULL)
		return;
	
	dump_tree(sym->left);

	write_sym(sym->name, Do_kludge);
	if (sym->type & DEF)	/* if the symbol is defined */
		Putw(040 | get_type(sym->prsect->type), Symp);
	else			/* undefined */
		Putw(040, Symp);
	Putw(sym->value, Symp);
	sym->t_numb = T_counter++;

	dump_tree(sym->right);
}


/**************************  write_sym  ***********************************/


write_sym(sname, flag)	/* write the given symbol as 8 bytes (null padded)
			** in the symbol file , if flag then write the symbol
			** with an underscore */
register char	*sname;
register int	flag;
{
	if (flag)
		putc('_', Symp);
	while (*sname)
	{
		if (*sname == ' ')
			putc(0, Symp);
		else
			putc(*sname, Symp);
		sname++;
	}
	putc(0, Symp);
	if (!flag)
		putc(0, Symp);
}


/****************************  pass2  ****************************************/


pass2()		/* translate code and write local symbols */

{
	struct objfile	*obj;	/* which object file */

	obj = File_root;
	while (obj != NULL)
	{
		transcode(obj);
		if (Do_table && !No_locals)
			dump_locals(obj);
		obj = obj->nextfile;
	}
}


/************************  transcode  ****************************************/


transcode(obj)		/* translate code */

struct objfile	*obj;		/* object file to translate code from */
{
	register int	drctv;		/* directive from obj file */
	register int	value;		/* temporary variable */
	int		vrd;		/* possible virtual register directive */
	static struct outword	wbuff;


	ch_input(obj->fname, CODE);
	while (morebytes())	/* continue reading code section until
				** empty */
	{
	    switch (drctv = getbyte())
	    {
		case 000:
			abswrite((WORD)0, (WORD)0);
			break;

		case 002:
			relwrite((WORD)0, (WORD)0);
			break;

		case 004:
			abswrite(getword(), (WORD)0);
			break;

		case 006:
			relwrite(getword(), (WORD)0);
			break;

		case 010:
			abswrite(Curr.val, Curr.rbits);
			break;

		case 014:
			abswrite(Curr.val + getword(), Curr.rbits);
			break;

		case 020:
			get_sym(&wbuff);
			abswrite(wbuff.val, wbuff.rbits);
			break;

		case 022:
			get_sym(&wbuff);
			relwrite(wbuff.val, wbuff.rbits);
			break;

		case 030:
			get_rc(&wbuff, obj, (char *)NULL);
			abswrite(wbuff.val, wbuff.rbits);
			break;

		case 032:
			get_rc(&wbuff, obj, (char *)NULL);
			relwrite(wbuff.val, wbuff.rbits);
			break;

		case 034:
			get_rc(&wbuff, obj, (char *)NULL);
			abswrite(wbuff.val + getword(), wbuff.rbits);
			break;

		case 036:
			get_rc(&wbuff, obj, (char *)NULL);
			relwrite(wbuff.val + getword(), wbuff.rbits);
			break;

		case 040:
			do040(obj);
			break;

		case 044:
			wbuff.val = getword();
			wbuff.rbits = 0;
			vreg_oper(getbyte(), &wbuff);
			break;

		case 050:
			if ((vrd = getbyte()) == 0)
				linkseek(Curr.val, Curr.rbits);
			else
				vreg_oper(vrd, &Curr);
			break;

		case 054:
			value = getword();
			if ((vrd = getbyte()) == 0)
				linkseek(Curr.val + value, Curr.rbits);
			else
			{
				wbuff.val = Curr.val + value;
				wbuff.rbits = Curr.rbits;
				vreg_oper(vrd, &wbuff);
			}
			break;

		case 060:
			get_sym(&wbuff);
			vreg_oper(getbyte(), &wbuff);
			break;

		case 070:
			get_rc(&wbuff, obj, (char *)NULL);
			if ((vrd = getbyte()) == 0)
			{
				linkseek(wbuff.val, wbuff.rbits);
				Curr.val = wbuff.val;
				Curr.rbits = wbuff.rbits;
			}
			else
				vreg_oper(vrd, &wbuff);
			break;

		case 074:
			get_rc(&wbuff, obj, (char *)NULL);
			value = getword();
			if ((vrd = getbyte()) == 0)
			{
				linkseek(wbuff.val + value, wbuff.rbits);
				Curr.val = wbuff.val;
				Curr.rbits = wbuff.rbits;
			}
			else
			{
				wbuff.val += value;
				vreg_oper(vrd, &wbuff);
			}
			break;

		case 0200:
			bytewrite((char)0);
			break;

		case 0204:
			bytewrite((char)getbyte());
			break;

		case 0220:
			get_sym(&wbuff);
			if (Do_bits && wbuff.rbits != 0)
				uerror("unrelocatable byte expression");
			bytewrite((char)(0377 & wbuff.val));
			break;

		default:
			sprintf(Erstring, "bad code directive %03o", drctv);
			lerror(Erstring);
	    }
	}
}


/*************************  abswrite  *****************************************/


abswrite(value, rbits)	/* write value in the out file */
register WORD	value;
register WORD	rbits;	/* relocation bits */
{
	Putw(value, Outp);
	Glc += 2;
	if (Do_bits)
		Putw(rbits, Bitp);
}


/************************  relwrite  ****************************************/


relwrite(value, rbits)	/* write value in out file relative to 
			** global location counter */
register WORD	value;
register WORD	rbits;
{
	Putw(value - Glc - 2, Outp);
	Glc += 2;
	if (Do_bits)
		Putw((rbits == Curr.rbits) ? 0 : (rbits | 01), Bitp);
			/* if the relocation bits for the word being written are 
			** the same as the current psect's then the word is an 
			** absolute offset, otherwise add 1 to the relocation
			** bits to indicate relativity to the PC */
}


/*************************  bytewrite  *************************************/


bytewrite(value)	/* write the byte in the out file */
	char value;
{
	putc(0377&value, Outp);
	Glc++ ;
}


/*************************  get_rc  *****************************************/


get_rc(wbuff, obj, psname)	/* place in wbuff the relocation constant and
				** relocation bits of psname
				** or if psname is NULL the psect whose
				** name is in the input stream, and whose object
				** file is pointed to by 'obj'. */

register struct outword		*wbuff;
struct objfile			*obj;
register char			*psname;

{
	char			name[7];	/* the name of the psect */
	register struct psect	*ps;

	/* if psname is NULL get name from input stream */
	if (psname == NULL)
	{
		psname = name;
		dc_symbol(psname);
	}

	ps = obj->psect_list;

	while (strcmp(ps->name, psname))	/* while the strings are
						** not equal */
	{
		ps = ps->obsame;
		if (ps == NULL)
		{
			sprintf(Erstring, "rc not found for %s", psname);
			lerror(Erstring);
		}
	}
	wbuff->val = ps->rc;
	wbuff->rbits = get_bits(ps->type);
}


/*****************************  get_bits  **********************************/


get_bits(attributes)	/* get the out file symbol table bits and convert
			** to relocation table bits */

register int 	attributes;	/* the M11 attributes of a psect */
{
	return (2 * (get_type(attributes) - 1));
}


/*****************************  get_type  ***********************************/


get_type(attr)		/* decode the psect type into out file symbol table
			** attribute word format */
register int	attr;
{
	if (!(attr & REL))	/* absolute */
		return (01);
	else if (attr & INS)	/* text */
		return (02);
	else if (attr & BSS)	/* bss */
		return (04);
	else			/* data */
		return (03);
}


/***************************  get_sym  ***************************************/


get_sym(wbuff)		/* get the value of the symbol in the input stream */
register struct outword	*wbuff;

{
	char			sname[7];	/* the name of the symbol */
	register struct symbol  *sym;
	register int		cond;		/* used for branching left
						** or right */
	int			index;		/* virtual register index */

	dc_symbol(sname);
	if (*sname != ' ')
	{
		sym = Sym_root;
		while (sym != NULL)
		{
			if ((cond = strcmp(sname, sym->name)) == 0)
			{
				if (sym->type & DEF)	/* if defined */
				{
					wbuff->val = sym->value;
					wbuff->rbits = get_bits(sym->prsect->type);
					return;
				}
				else if (Do_bits)	/* set relocation bits
							** for undefined symbol
							** and return zero */
				{
					Undefineds = 1;
					wbuff->val = 0;
					wbuff->rbits = 020 * sym->t_numb + 010;
					return;
				}
				else
				{
					sprintf(Erstring, "undefined symbol: %s", sname);
					uerror(Erstring);
					wbuff->val = 0;
					wbuff->rbits = 0;
					return;
				}
			}
			else if (cond < 0)
				sym = sym->left;
			else
				sym = sym->right;
		}
		/* symbol has not been found */
		sprintf(Erstring, "%s not found", sname);
		lerror(Erstring);
		wbuff->val = 0;
		wbuff->rbits = 0;
	}
	else	/* virtual register */
	{
		index = sname[5] - 'a';
		wbuff->val = Vreg[index].val;
		wbuff->rbits = Vreg[index].rbits;
	}
}


/***************************  vreg_oper  *************************************/

vreg_oper(drctv, wbuff)		/* preform an operation on a virtual register */

register int		drctv;		/* directive (operation) */
register struct outword *wbuff;		/* source value and relocation bits */
{
	register int	index;		/* index of destination register */
	static char	mess[] = "unrelocatable arithmetic expression";

	index = getbyte() - 1;
	if (index >= MAXREG)
	{
		uerror("expression involving global symbols too large");
		index %= MAXREG;
	}

	switch(drctv)
	{
		case 0200:
			Vreg[index].val = wbuff->val;
			Vreg[index].rbits = wbuff->rbits;
			break;

		case 0201:
			Vreg[index].val += wbuff->val;
			if (Do_bits && Vreg[index].rbits && wbuff->rbits)
				uerror(mess);
			else
				Vreg[index].rbits |= wbuff->rbits;
			break;

		case 0202:
			Vreg[index].val -= wbuff->val;
			if (Do_bits && wbuff->rbits)
				if (Vreg[index].rbits == wbuff->rbits)
					Vreg[index].rbits = 0;
				else
					uerror(mess);
			break;

		case 0203:
			Vreg[index].val *= wbuff->val;
			if (Do_bits)
				uerror(mess);
			break;

		case 0204:
			Vreg[index].val /= wbuff->val;
			if (Do_bits)
				uerror(mess);
			break;

		case 0205:
			Vreg[index].val &= wbuff->val;
			if (Do_bits)
				uerror(mess);
			break;

		case 0206:
			Vreg[index].val |= wbuff->val;
			if (Do_bits)
				uerror(mess);
			break;

		default:
			sprintf(Erstring, "bad v.r. directive: %03o", drctv);
			lerror(Erstring);
	}
}


/**************************  do040  ***************************************/


do040(obj)	/* do 040 code directive */

register struct objfile	*obj;
{
	register int	drctv;
	register int	index;

	switch (drctv = getbyte())
	{
		case 001:	/* low limit */
			index = getbyte() - 1;
			Vreg[index].val = Maxabs;
			Vreg[index].rbits = 0;
			if (Do_bits)
				uerror("'.limit' directive not relocatable");
			break;

		case 002:	/* high limit */
			index = getbyte() - 1;
			Vreg[index].val = R_counter;
			Vreg[index].rbits = 0;
			break;

		case 003:	/* ^pl */
		case 004:	/* ^ph */
			p_limit(obj, drctv);
			break;

		case 0200:	/* clear */
			index = getbyte() - 1;
			Vreg[index].val = Vreg[index].rbits = 0;
			break;

		case 0201:	/* add zero */
			index = getbyte();
			break;

		default:
			sprintf(Erstring, "bad 040 directive %03o", drctv);
			lerror(Erstring);
	}
}


/*************************  p_limit  **************************************/


p_limit(obj, drctv)	/* find the low or high limit of a psect */
	struct objfile		*obj;
	int			drctv;
{
	register struct psect	*ps;
	register struct g_sect	*gptr;
	int			cond;
	register int		index;
	char			pname[7];

	dc_symbol(pname);
	index = getbyte() - 1;

	ps = obj->psect_list;

	while (ps != NULL)	/* try to find the psect in local link-list */
	{
		if ( !strcmp(pname, ps->name))
			break;
		ps = ps->obsame;
	}

	if (ps != NULL)		/* if it was found in the local list */
	{
		while (ps->pssame != NULL)	/* find bottom of sames, if any.
					 	** see relocate() */
			ps = ps->pssame;
	}
	else	/* psect not a local so check global tree */
	{
		gptr = Gsect_root;
		while (gptr != NULL)
		{
			if ( !(cond = strcmp(pname, gptr->name)))
			{
				if (gptr->last_sect->pssame == NULL)
					/* the psect is solitary */
					ps = gptr->last_sect;
				else
					/* last_psect points to the last psect
					** in a plural link-list of sames.
					** there is a psect after last_sect
					** which contains the conglomeration
					** of all the sames.
					** see relocate() */
					ps = gptr->last_sect->pssame;
				break;
			}
			else if (cond < 0)
				gptr = gptr->leftt;
			else
				gptr = gptr->rightt;
		}

		if (ps == NULL)		/* psect not found */
		{
			sprintf(Erstring, "%s not found in ^p reference", pname);
			uerror(Erstring);
			Vreg[index].val = Vreg[index].rbits = 0;
			return;
		}
	}

	if (drctv == 003)	/* low */
		Vreg[index].val = ps->rc;
	else			/* high */
		Vreg[index].val = ps->rc + ps->nbytes;

	Vreg[index].rbits = get_bits(ps->type);
}


/***************************  linkseek  *************************************/


linkseek(nlc, nrbits)

register WORD nlc;	/* new location counter */
register WORD nrbits;	/* new relocation bits */
{
	long	where;

	Glc = nlc;

	if (nrbits == 04)	/* if data section add offset */
		where = nlc + Seekoff + 020;
	else
		where = nlc + 020;

	if (fseek(Outp, where, 0) == -1)
	{
skerr:		fprintf(stderr, "Fseek error\n");
		bail_out();
	}
	if (Do_bits)
		if (fseek(Bitp, where - 020L, 0) == -1)
			goto skerr;

	if (nrbits == 06)	/* bss section about to be writen */
		uerror("error: initialized data in bss psect");
}


/****************************  dump_locals  ********************************/


# define	MAXCONSTS	10


dump_locals(obj)	/* dump local symbols */

struct objfile	*obj;
{
	int		rconst[MAXCONSTS];	/* relocation constants */
	int		segment[MAXCONSTS];	/* segment type for out file */
	struct psect	*last;			
	struct psect	*temp;
	char		sname[7];		/* symbol name */
	char		index;
	char		type;
	int		value;
	int		i;

	/* fill rconst and segment arrays from the first MAXCONSTS psects */
	temp = obj->psect_list;
	i = 0;
	while (temp != NULL && i < MAXCONSTS)
	{
		rconst[i] = temp->rc;
		segment[i++] = get_type(temp->type);
		temp = temp->obsame;
	}
	last = temp; 	/* (in case there are more psects than MAXCONSTS) */

	/* write 'em */

	ch_input(obj->fname, SYMBOLS);
	while (morebytes())
	{
		dc_symbol(sname);
		type = getbyte();
		index = getbyte();
		value = getword();
		
		if (type & 001)		/* if internal m11 symbol, forget it */
			continue;

		write_sym(sname, 0);
		if (index >= MAXCONSTS)
		{
			temp = last;
			for (i = MAXCONSTS; i < index; i++)
				temp = temp->obsame;
			Putw(get_type(temp->type), Symp);
			Putw(value + temp->rc, Symp);
		}
		else 
		{
			Putw(segment[index], Symp);
			Putw(value + rconst[index], Symp);
		}
	}
}


/************************  uerror  ******************************************/


uerror(mess)	/* print user error message and increment Nerrors */

register char	*mess;
{
	Nerrors++;
	fprintf(stderr, "%s\n", mess);
	if (Do_map)
		fprintf(Mapp, "%s\n", mess);
}


/**************************  loose_ends  ************************************/


loose_ends()
{
	register c;
	register int	nbytes;	/* number of bytes in out file symbol table */


	if (Do_bits)
	{
		if (ferror(Bitp))
			werror(Bitname);
		
		Bitp = freopen(Bitname, "r", Bitp);
		if (Bitp == NULL)
			lerror("can't reopen relocation bits file");

		fseek(Outp, (long) (Tex_size + Dat_size + 020L), 0);

		while ((c = getc(Bitp)) != EOF)
			putc(0377&c, Outp);

		unlink(Bitname);
	}

	if (Do_table)
	{
		if (ferror(Symp))
			werror(Symname);

		Symp = freopen(Symname, "r", Symp);
		if (Symp == NULL)
			lerror("can't reopen symbol table file");

		/* send r/w head to end of rbits or code section in out file */
			
		fseek(Outp, (long) ((Tex_size + Dat_size) * (Do_bits + 1) + 020L), 0);

		nbytes = 0;
		while ((c = getc(Symp)) != EOF)
		{
			nbytes++;
			putc(0377&c, Outp);
		}

		/* write size of symbol table */
		fseek(Outp, 010L, 0);
		Putw(nbytes, Outp);

		unlink(Symname);
	}

	if (Do_map)
	{
		fprintf(Mapp, "errors detected:  %d\n", Nerrors);
		if (ferror(Mapp))
			werror(Mapname);
		fclose(Mapp);
		if (Do_lpr_map)
			execl("/usr/bin/lpr", "lpr", Mapname, 0);
	}

	if (ferror(Outp))
		werror(Outname);

	/* change out file to be executable if no errors */
	chmod(Outname, Nerrors ? 0644 : 0755);

	if (Nerrors || !Do_silent)
		fprintf(stderr, "errors detected:  %d\n", Nerrors);

	exit(Nerrors ? 1 : 0);
}


/****************************  bail_out  ***********************************/


bail_out()	/* unlink any opened output file then exit */

{
	if (Outp != NULL)
		unlink(Outname);
	if (Mapp != NULL)
		unlink(Mapname);
	if (Symp != NULL)
		unlink(Symname);
	if (Bitp != NULL)
		unlink(Bitname);
	exit(1);
}


/********************************  werror  **********************************/


werror(fname)		/* write error handler */
char	*fname;

{
	perror(fname);
	if (Symp != NULL)
		unlink(Symname);
	if (Bitp != NULL)
		unlink(Bitname);
	exit(1);
}
Putw(x, p)
	FILE *p;
{
	putc(x & 0377, p);
	x >>= 8;
	putc(x & 0377, p);
}
