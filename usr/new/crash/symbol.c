/*
 *	U N I X   2 . 9 B S D   C R A S H   A N A L Y Z E R
 *
 *	S Y M B O L   R O U T I N E S
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <a.out.h>
#include "crash.h"

#define	MAXDIFF		2048L
#define	SPACE		100

/* Global Variables */
int	kmem;				/* Global FD of dump file */
int	sym;				/* global fd for symbol file */

unsigned find(), findv();
char *malloc();

/*
 *			G E T S Y M
 *
 *
 *	getsym		John Stewart 2 Mar 83
 *	takes:		symref--string naming file with symbol table
 *	returns:	void
 *	it does:	loads symtab from symref file
 *				loads ovh
 *				sets globals symtab
 *	messy side effects:	loads ovh
 */
/*
 * Define symbol table globals here:
 */
struct symsml *symtab, *maxsym;
daddr_t symbas;
daddr_t txtbas;
unsigned nsyms;
struct ovlhdr ovh;	/* not really used */
extern	int overlay;

getsym(symref)
char *symref;
{
	long n;
	register int m;
	struct syment space[SPACE];
	register struct syment *symp;
	struct symsml *smlp;
	struct exec xbuf;

	sym = open(symref,0);
	if (sym < 0) {
		printf("Unable to open %s\n",symref);
		exit(1);
	}

	/*
	 *	Load the Symbol Table into Core
	 */
	symbas = 0L;
	read(sym, &xbuf, sizeof xbuf);
/*printf("read syms %04x\n", xbuf.a_syms);/**/
	if (N_BADMAG(xbuf)) {
		printf("Format error in %s\n",symref);
		exit(1);
	}
	symbas += xbuf.a_text + (long) xbuf.a_data;
	if (xbuf.a_magic == A_MAGIC5 || xbuf.a_magic == A_MAGIC6) {
		overlay++;
		read(sym, &ovh, sizeof (struct ovlhdr));
		for(m=0; m<NOVL; m++) {
			symbas += (long) ovh.ov_siz[m];
		}
		symbas += (long) sizeof ovh;
	}
	if (xbuf.a_flag != 1) {
		symbas *= 2L;
	}
	symbas += (long) sizeof xbuf;
	if (overlay)
		txtbas = ((((long)xbuf.a_text)+8192L-1L)/8192L)*8192L;
	lseek(sym, symbas, 0);
	n = 0;
	ioctl(sym, FIONREAD, (char *)&n);
	if (n == 0) {
		printf("No namelist in %s\n",symref);
		exit(1);
	}
	nsyms = n/sizeof(struct syment);	/* number of symbols in table */
	m = nsyms * sizeof(struct symsml);	/* small table size needed */
	symtab = (struct symsml *) malloc(m);
	if (symtab == (struct symsml *)NULL) {
		printf("Symbol table too large\n");
		exit(1);
	}
/*printf("calc syms %08X nsyms %04x m %04x\n", n, nsyms, m);/**/
	smlp = symtab;
	while (n) {
		m = sizeof space;
		if (n < m)
			m = n;
		read(sym, (caddr_t)space, m);
		n -= m;
		for (symp = space; (m -= sizeof(struct syment)) >= 0;symp++) {
/*printf("\t%8s %4x %d\n", symp->name, symp->value, symp->ovno);/**/
			smlp->svalue = symp->value;
			smlp->sovno = symp->ovno;
			if (symp->flags >= 041 || (symp->flags >= 01 &&
						   symp->flags <= 04))
			    if ((symp->flags & 07) == 02)
				smlp->sflags = ISYM;
			    else
				smlp->sflags = DSYM;
			else
			    smlp->sflags = NSYM;
			smlp++;
		}
	}
	maxsym = smlp;
}

/*
 *			S Y M B O L
 *
 *
 *	symbol		mark kampe	11/17/75
 *	takes:		int (an address)
 *			int (the type of symbol you want found TEXT or DATA)
 *	returns:	int (address of symbol table entry)
 *	it does:	find the global text symbol whose
 *			value is the nearest less than or equal to the
 *			passed address.  It then prints out a field
 *			of the form (<symbol>+<offset>).
 *			No value is now returned.  (Formerly,
 *			the value returned was the address of
 *			the symbol table entry for that symbol.)
 */
symbol(val, type, ovno)
unsigned val;
char type;
int ovno;
{
	register struct symsml *i, *minptr;
	struct syment symt;
	long mindif, offset, value, symval;
	int ov = 0;

	mindif = 0377777L;
	minptr = (struct symsml *) NULL;
	if (txtbas && type == ISYM && val > txtbas)
		ov = ovno;

	/* Find closest preceeding global symbol */
	i = symtab;
	value = val;
	while (mindif && i < maxsym) {
	    if (type == i->sflags && (!ov || ov == i->sovno)) {
		symval = (long)(unsigned) i->svalue;
		if (((value - symval) < mindif) && (value >= symval)) {
			mindif = value - symval;
			minptr = i;
		}
	    }
	    i++;
	}

	if (minptr && mindif < MAXDIFF) {
		offset = (long)((unsigned) (minptr - symtab));
		offset = symbas + offset*sizeof(struct syment);
		lseek(sym, offset, 0);
		read(sym, &symt, sizeof(struct syment));
		if (mindif)
			printf("%.8s+0%o", symt.name, (unsigned) mindif);
		else
			printf("%-8.8s", symt.name);
		if (overlay && minptr->sovno && type == ISYM)
			printf(",kov=%d",minptr->sovno);
		return(1);
	}
	return(0);
}

/*
 *			F E T C H
 *
 *	This routine takes a structure of 'fetch' format,
 * and reads from the core dump file each specified variable
 * into the given core address.  This provides a simple method
 * of loading our prototype data areas.
 *	Mike Muuss, 6/28/77.
 */

fetch( table )
register struct fetch *table;
{

	while( table -> f_size ) {
		if (vf(table->symbol, table->addr, table->f_size ) == 0) {
			printf("\nCould not find: ");
			barf(table->symbol);
		}
		table++;
	}
}

/*
 *			F E T C H A
 *
 *	This routine is like fetch, except that only addresses are
 *	stuffed
 */
fetcha( table )
register struct fetch *table;
{

	while( table -> f_size ) {
		if ((*((unsigned *)table->addr)
			= (unsigned) find(table->symbol, 1)) == NOTFOUND) {
			printf("\nCould not find: ");
			barf(table->symbol);
		}
		table++;
	}
}


/*
 *			V F
 *
 *
 * Variable Fetch -- Fetch the named symbol, and load it from the dump
 *	returns:	1 if successful, 0 if not
 */

int vf( name, addr, size )
register char *name;
char *addr;
{
	unsigned pos;			/* Kernel addr */

	/* position ourselves in the file */
	if ((pos=findv(name, 1))==NOTFOUND) {
		return(0);
	}
	lseek(kmem, (long) pos, 0);

	/* Perform the actual transfer */
	if (read(kmem, addr, size) == size)
		return(1);
	printf( "\n\t***WARNING: unsuccessful read of variable %s\n", name);
	return(0);
}


/*
 *			F I N D V
 *
 *
 * Look up the address of the given global variable in
 * the symbol table
 * takes:	string containing symbol
 * returns:	value of symbol, if found
 *			0177777, if not found.
 */

unsigned findv( string, global )
char *string;
register global;
{
	struct syment *i, space[SPACE];
	register unsigned n;
	register int m;
	
	n = nsyms * sizeof(struct syment);
	lseek(sym, symbas, 0);
	while(n) {
		m = sizeof space;
		if (n < m)
			m = n;
		read(sym, (caddr_t)space, m);
		n -= m;
		for (i = space; (m -= sizeof(struct syment)) >= 0; i++) {
		    if( (global && ((i->flags & 077)) < 041)) continue;
		    if( equal( string, i->name ) ) {
			return( i->value );
		    }
		}
	}

	return(NOTFOUND);
}

unsigned find(string, global)
char *string;
int global;
{
	register unsigned val;

	if ((val=findv(string, global))!=NOTFOUND)
		return(val);
	printf("\nCould not find: ");
	barf( string );
	/*NOTREACHED*/
}

/*
 * Obtain the ova and ovd arrays
 */

u_long		aova, aovd;		/* addresses of ova and ovd in core */
unsigned	ova[8], ovd[8];		/* overlay addresses and sizes */

getpars()
{
	lseek(kmem, (off_t)aova, 0);	/* position of the ova */
	read(kmem, ova, sizeof ova);	/* read it in */
	lseek(kmem, (off_t)aovd, 0);	/* position of the ovd */
	read(kmem, ovd, sizeof ovd);	/* read it in too */
}
