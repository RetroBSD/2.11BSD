#include <stdio.h>
#include "defs.h"
#include <fcntl.h>
#include <sys/file.h>

	struct	SYMbol	*symbol;
	char	localok;
	int	lastframe;
	u_int	maxoff;
	long	maxstor;
	MAP	txtmap;
	char	curov;
	int	overlay;
	long	localval;
	char	*errflg;
	u_int	findsym();

	struct	SYMcache
		{
		char	*name;
		int	used;
		struct	SYMbol *syment;
		};

#ifndef	NUM_SYMS_CACHE
#define	NUM_SYMS_CACHE	50
#endif

static	struct	SYMcache symcache[NUM_SYMS_CACHE];
static	struct	SYMcache *endcache = &symcache[NUM_SYMS_CACHE];
static	struct	SYMbol	*symtab;
static	FILE	*strfp;
static	int	symcnt;
static	int	symnum;
static	char	*sgets();

extern	char	*myname, *symfil, *strdup();
extern	off_t	symoff, stroff;

/* symbol table and file handling service routines */

valpr(v,idsp)
{
	u_int	d;

	d = findsym(v,idsp);
	IF d < maxoff
	THEN	printf("%s", cache_sym(symbol));
		IF d
		THEN	printf(OFFMODE, d);
		FI
	FI
}

localsym(cframe)
	long	cframe;
{
	int symflg;

	WHILE symget() && localok && (symflg= (int)symbol->type) != N_FN
		&& *no_cache_sym(symbol) != '~'
	DO IF symflg>=2 ANDF symflg<=4
	   THEN localval=symbol->value;
		return(TRUE);
	   ELIF symflg==1
	   THEN localval=leng(shorten(cframe)+symbol->value);
		return(TRUE);
	   ELIF symflg==20 ANDF lastframe
	   THEN localval=leng(lastframe+2*symbol->value - (overlay?12:10));
		return(TRUE);
	   FI
	OD
	return(FALSE);
}

psymoff(v,type,s)
	long v;
	int type;
	char *s;
{
	u_int	w;

	w = findsym(shorten(v),type);
	IF w >= maxoff
	THEN printf(LPRMODE,v);
	ELSE printf("%s", cache_sym(symbol));
	     IF w THEN printf(OFFMODE,w); FI
	FI
	printf(s);
}

u_int
findsym(svalue,type)
	u_int	svalue;
	int	type;
	{
	long	diff, value, symval;
	register struct SYMbol	*sp;
	struct	SYMbol *symsav;
	int	i;
	char	ov = 0;

	if	(txtmap.bo && type==ISYM && svalue>=txtmap.bo)
		ov=curov;
	value = svalue;
	diff = 0377777L;

	if	(type != NSYM && symnum)
		{
		for	(i = 0, sp = symtab; diff && i < symnum; i++, sp++)
			{
			if	(SYMTYPE(sp->type) == type && 
					(!ov || ov == sp->ovno))
				{
				symval = leng(sp->value);
				if	((value - symval) < diff && 
						value >= symval)
					{
					diff = value - symval;
					symsav = sp;
					}
				}
			}
		}
	if	(symsav)
		symcnt =  symsav - symtab;
	symbol = symsav;
	return(shorten(diff));
	}

/* sequential search through table */
symset()
	{

	symcnt = -1;
	}

struct SYMbol *
symget()
	{

	if	(symcnt >= symnum || !symnum)
		return(NULL);
	symcnt++;
	symbol = &symtab[symcnt];
	return(symbol);
	}

/*
 * This only _looks_ expensive ;-)  The extra scan over the symbol
 * table allows us to cut down the amount of memory needed.  This is
 * where symbols with string table offsets over 64kb are excluded.  Also,
 * a late addition to the program excludes register symbols - the assembler
 * generates *lots* of them and they're useless to us.
*/
symINI(ex)
	struct	exec	*ex;
	{
	register struct	SYMbol	*sp;
	register FILE	*fp;
	struct	nlist	sym;
	int	i, nused, globals_only = 0;

	fp = fopen(symfil, "r");
	strfp = fp;
	if	(!fp)
		return;
	fcntl(fileno(fp), F_SETFD, 1);

	symnum = ex->a_syms / sizeof (sym);

	fseek(fp, symoff, L_SET);
	nused = 0;
	for	(i = 0; i < symnum; i++)
		{
		fread(&sym, sizeof (sym), 1, fp);
		if	(sym.n_type == N_REG)
			continue;
		if	(sym.n_un.n_strx >= 0200000L)
			printf("symbol %d string offset > 64k - ignored\n",i);
		else
			nused++;
		}
	fseek(fp, symoff, L_SET);

	symtab = (struct SYMbol *)malloc(nused * sizeof (struct SYMbol));
	if	(!symtab)
		{
		globals_only = 1;
		nused = 0;
		for	(symcnt = 0; symcnt < symnum; symcnt++)
			{
			fread(&sym, 1, sizeof (sym), fp);
			if	(sym.n_type == N_REG)
				continue;
			if	((sym.n_type & N_EXT) == 0)
				continue;
			if	(sym.n_un.n_strx >= 0200000L)
				continue;
			nused++;
			}
		symtab = (struct SYMbol *)malloc(nused * sizeof(struct SYMbol));
		if	(!symtab)
			{
			printf("%s: no memory for symbols\n", myname);
			symnum = 0;
			return;
			}
		}
	fseek(fp, symoff, L_SET);
	sp = symtab;
	for	(symcnt = 0; symcnt < symnum; symcnt++)
		{
		fread(&sym, 1, sizeof (sym), fp);
		if	(sym.n_type == N_REG)
			continue;
		if	(globals_only && !(sym.n_type & N_EXT))
			continue;
		if	(sym.n_un.n_strx >= 0200000L)
			continue;
		sp->value = sym.n_value;
		sp->ovno = sym.n_ovly;
		sp->type = sym.n_type;
		sp->soff = shorten(sym.n_un.n_strx);
		sp++;
		}
	symnum = nused;
#ifdef	debug
	printf("%d symbols loaded\n", nused);
#endif
	if	(globals_only)
		printf("%s: could only do global symbols\n", myname);
	symset();
	return(0);
	}

/*
 * Look in the cache for a symbol in memory.  If it is not found use
 * the least recently used entry in the cache and update it with the
 * symbol name.
*/
char	*
cache_sym(symp)
	register struct	SYMbol *symp;
	{
	register struct SYMcache *sc = symcache;
	struct	SYMcache *current;
	int	lru;

	if	(!symp)
		return("?");
	for	(current = NULL, lru = 30000 ; sc < endcache; sc++)
		{
		if	(sc->syment == symp)
			{
			sc->used++;
			if	(sc->used >= 30000)
				sc->used = 10000;
			return(sc->name);
			}
		if	(sc->used < lru)
			{
			lru = sc->used;
			current = sc;
			}
		}
	sc = current;
	if	(sc->name)
		free(sc->name);
	sc->used = 1;
	sc->syment = symp;
	sc->name = strdup(sgets(symp->soff));
	return(sc->name);
	}

/*
 * We take a look in the cache but do not update the cache on a miss.
 * This is done when scanning thru the symbol table (printing all externals
 * for example) for large numbers of symbols which probably won't be
 * used again any time soon.
*/
char	*
no_cache_sym(symp)
	register struct	SYMbol *symp;
	{
	register struct SYMcache *sc = symcache;

	if	(!symp)
		return("?");
	for	( ; sc < endcache; sc++)
		{
		if	(sc == symp)
			{
			sc->used++;
			if	(sc->used >= 30000)
				sc->used = 10000;
			return(sc->name);
			}
		}
	return(sgets(symp->soff));
	}

/*
 * Looks in the cache for a match by string value rather than string
 * file offset. 
*/

struct	SYMbol *
cache_by_string(str, ovsym)
	char	*str;
	int	ovsym;
	{
	register struct SYMcache *sc;

	for	(sc = symcache; sc < endcache; sc++)
		{
		if	(!sc->name)
			continue;
		if	(eqsym(sc->name, str, ovsym ? '~' : '_'))
			break;
		}
	if	(sc < endcache)
		{
		sc->used++;
		if	(sc->used > 30000)
			sc->used = 10000;
		return(sc->syment);
		}
	return(0);
	}

static char *
sgets(soff)
	u_short	soff;
	{
	static	char	symname[MAXSYMLEN + 2];
	register char	*buf = symname;
	int	c;
	register int i;

	fseek(strfp, stroff + soff, L_SET);
	for	(i = 0; i < MAXSYMLEN; i++)
		{
		c = getc(strfp);
		*buf++ = c;
		if	(c == '\0')
			break;
		}
	*buf = '\0';
	return(symname);
	}
