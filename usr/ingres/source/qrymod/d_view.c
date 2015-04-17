# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../catalog.h"
# include	"qrymod.h"



/*
**  D_VIEW -- define view
**
**	This procedure connects the tree in with the relation catalog
**	and inserts the view tree into the tree catalog.
**
**	The information in the pipe is expected to come as follows:
**		create for view, with S_VIEW bit set so that a
**			physical relation is not created.
**		define tree, which will put the translation tree
**			into the 'tree' catalog.
**		define view, which will connect the two together.
**			The first two absolutely must be done before
**			this step can be called.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		I/O in 'tree' catalog.
**
**	Requires:
**		rdpipe -- to get parameters.
**		Treeroot -- a ptr to the defintion tree.
**		Pipe -- R_up pipe block.
**		puttree -- to output the tree to the tree catalog.
**		Rangev, Qmode, Resultvar -- for parameter validation.
**
**	Called By:
**		define
**
**	Trace Flags:
**		39
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		On inconsistancies in system catalogs.
**
**	History:
**		2/19/79 (eric) -- split from define.c.
*/

extern QTREE			*Treeroot;
extern struct descriptor	Reldes;
extern struct pipfrmt		Pipe;




d_view()
{
	char		viewid[MAXNAME + 1];
	struct relation	relkey, reltup;
	register QTREE	*t;
	register int	i;
	struct tup_id	tid;
	int		treeid;

	/*
	**  Read parameters.
	**	Only parameter is view name (which must match the
	**	name of the Resultvar).
	*/

	if (rdpipe(P_NORM, &Pipe, R_up, viewid, 0) > MAXNAME + 1)
		syserr("d_view: rdpipe");
	pad(viewid, MAXNAME);

	/* done with pipe... */
	rdpipe(P_SYNC, &Pipe, R_up);
	
#	ifdef xQTR3
	/* do some extra validation */
	if (Treeroot == NULL)
		syserr("d_view: NULL Treeroot");
	if (Qmode != mdDEFINE)
		syserr("d_view: Qmode %d", Qmode);
	if (Resultvar < 0)
		syserr("d_view: Rv %d", Resultvar);
	if (!Rangev[Resultvar].rused || !bequal(Rangev[Resultvar].relid, viewid, MAXNAME))
		syserr("d_view: rangev %d %.14s", Rangev[Resultvar].rused,
		    Rangev[Resultvar].relid);
#	endif

	Rangev[Resultvar].rused = FALSE;
	Resultvar = -1;
	Qmode = -1;

	/* output tree to tree catalog */
	treeid = puttree(Treeroot, viewid, Usercode, mdVIEW);

	/* clear Treeroot to detect some errors */
	Treeroot = NULL;
}
