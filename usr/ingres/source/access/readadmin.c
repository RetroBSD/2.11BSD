# include	"../ingres.h"
# include	"../access.h"
# include	"../aux.h"
# include	"../lock.h"

/*
**  READADMIN -- read admin file into 'Admin' cache
**
**	The admin file in the current directory is opened and read
**	into the 'Admin' cache.  The admin file contains the following
**	information:
**
**	A header block, containing the owner of the database (that is,
**	the DBA), and a set of status bits for the database as a whole.
**	These bits are defined in aux.h.
**
**	Descriptors for the relation and attribute relations.  These
**	descriptors should be completely correct except for the
**	relfp and relopn fields.  These are required so that the
**	process of opening a relation is not recursive.
**
**	After the admin file is read in, the relation and attribute
**	files are opened, and the relfp and relopn fields in both
**	descriptors are correctly initialized.  Both catalogs are
**	opened read/write.
**
**	Finally, the relation catalog is scanned for the entries
**	of both catalogs, and the reltid field in both descriptors
**	are filled in from this.  This is because of a bug in an
**	earlier version of creatdb, which failed to initialize
**	these fields correctly.  In fact, it is critical that these
**	fields are correct in this implementation, since the reltid
**	field is what uniquely identifies the cache buffer in the
**	low-level routines.
**
**	WARNING:  This routine is redefined by creatdb.  If this
**		routine is changed, check that program also!!
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		The 'Admin' struct is filled in from the 'admin' file
**			in the current directory.
**		The 'relation....xx' and 'attribute...xx' files are
**			opened.
**
**	Defined Constants:
**		none
**
**	Defines:
**		readadmin()
**
**	Requires:
**		ingresname() -- to make the physical file name.
**		get_reltuple() -- to read the tuples from the
**			relation relation.
**		'Admin' structure (see ../aux.h).
**
**	Called By:
**		acc_init (in accbuf.c)
**
**	Files:
**		./admin
**			The bootstrap description of the database,
**			described above.
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		open admin
**			It was not possible to open the 'admin' file.
**			This is virtually a total disaster.
**		open rel
**			It was not possible to open the 'relation...xx'
**			file.
**		open att
**			It was not possible to open the 'attribute..xx'
**			file.
**		get_reltuple rel
**			It was not possible to get the relation
**			relation tuple for the relation relation.
**			That is, it was not possible to get the
**			"relation" tuple from the relation catalog.
**		get_reltuple att
**			Ditto, for the "attribute" tuple.
**
**	History:
**		12/13/78 (rse) -- get_reltuple isn't called if relation
**			is hashed. This always worked even in 6.0
**		8/2/78 (eric) -- changed 'admin' read to be a single
**			read, by combining all the admin info into
**			one large struct (instead of three small
**			ones).  See also access.h, accbuf.c,
**			noclose.c, openr.c, dbu/modify.c,
**			dbu/modupdate.c, ovqp/interp.c, and printr.
**
**		8/21/78 (michael) -- changed so that "attribute" opened
**			r/w. relopen is left ro.
*/

readadmin()
{
	register int		i;
	register int		retval;
	struct descriptor	desc;
	char			relname[MAXNAME + 4];

	/* read the stuff from the admin file */
	i = open("admin", 0);
	if (i < 0)
		syserr("readadmin: open admin %d", i);
	if (read(i, &Admin, sizeof Admin) != sizeof Admin)
		syserr("readadmin: read err admin");
	close(i);

	/* open the physical files for 'relation' and 'attribute' */
	ingresname(Admin.adreld.relid, Admin.adreld.relowner, relname);
	if ((Admin.adreld.relfp = open(relname, 2)) < 0)
		syserr("readadmin: open rel %d", Admin.adreld.relfp);
	ingresname(Admin.adattd.relid, Admin.adattd.relowner, relname);
	if ((Admin.adattd.relfp = open(relname, 2)) < 0)
		syserr("readadmin: open att %d", Admin.adattd.relfp);
	Admin.adreld.relopn = (Admin.adreld.relfp + 1) * -5;
	/* we just want to read here create, modify and destroy fix it up */
	Admin.adattd.relopn = (Admin.adattd.relfp + 1) * 5;

	/* get the correct tid's (actually, should already be correct) */
	/* (for compatability with 6.0 databases) */
	if (Admin.adreld.relspec == M_HEAP)
	{
		if (retval = get_reltuple(&desc, "relation"))
			syserr("readadmin: get_reltuple rel %d", retval);
		bmove(&desc.reltid, &Admin.adreld.reltid, sizeof Admin.adreld.reltid);
	}
	if (Admin.adattd.relspec == M_HEAP)
	{
		if (retval = get_reltuple(&desc, "attribute"))
			syserr("readadmin: get_reltuple att %d", retval);
		bmove(&desc.reltid, &Admin.adattd.reltid, sizeof Admin.adattd.reltid);
	}

	return (0);
}
