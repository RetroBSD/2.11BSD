# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
**  CLOSECATALOG -- close system catalog
**
**	This routine closes the sysetm relations opened by calls
**	to opencatalog.
**
**	The 'Desxx' struct defines which relations will be closed
**	in this manner and is defined in .../source/aux.h.
**
**	The actual desxx structure definition is in the file
**	
**		catalog_desc.c
**
**	which defines which relations can be cached and if any
**	alias descriptors exist for the relations. That file
**	can be redefined to include various caching.
**
**
**	Parameters:
**		really - whether to actually close the relations
**			or just update and flush them.
**
**	Returns:
**		none
**
**	Side Effects:
**		A relation is (may be) closed and its pages flushed
**
**	Requires:
**		Desxx -- a structure which defines the relations which
**			should be closed. A default structure is defined in
**			catalog_desc.c
**
**	Called By:
**		most dbu routines.
**		qrymod.
**		parser.
**		decomp.
**		creatdb.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**
**	History:
**		1/30/79 (rse) -- closecat(really) needed to clear relopn
**		12/12/78 (rse) -- Split from opencatalog.c
*/


closecatalog(really)
int	really;
{
	register struct desxx		*p;
	register int			r;
	extern struct desxx		Desxx[];

	r = really;

	for (p = Desxx; p->cach_relname; p++)
		if (r && !p->cach_alias)
		{
			if (closer(p->cach_desc) < 0)
				syserr("closecat %s", p->cach_relname);
		}
		else
		{
			if (noclose(p->cach_desc) < 0)
				syserr("closecat %s", p->cach_relname);
			if (r)
				p->cach_desc->relopn = 0;
		}
}
