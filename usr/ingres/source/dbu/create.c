# include	"../ingres.h"
# include	"../access.h"
# include	"../aux.h"
# include	"../catalog.h"
# include	"../symbol.h"
# include	"../lock.h"


/*
**  CREATE -- create new relation
**
**	This module creates a brand new relation in the current
**	directory (database).  The relation is always created as
**	a paged heap.  It may not redefine an existing relation,
**	or rename a system catalog.
**
**	Defines:
**		create -- driver
**		chk_att -- check attribute for validity
**		ins_att -- insert attribute into 'attribute' catalog
**		dup_att -- check for duplicate attribute
**		initstructs -- initialize relation and attribute
**			tuples for insertion into catalog
**		formck -- check for valid attribute type.
**
**	Requires:
**		ingresname
**		opencatalog
**		openr
**		insert
**		setcsl, unlcs
**		creat -- to create physical file
**		Usercode -- to find out the current user.
**		Reldes, Attdes -- from opencatalog
**
**	Required By:
**		overlaya
**		overlaym.c
**
**	Files:
**		relname....cc -- where cc is the Usercode.
**
**	Trace Flags:
**		2
**
**	Diagnostics:
**		5102 -- duplicate relation name %0
**			The relation named already exists (and is
**			owned by you.
**		5103 -- %0 is a system catalog
**			It is not OK to declare a relation name which
**			is the same as that of a system catalog, even
**			if you do not own the system catalog.  This is
**			so that when other parts of the system do an
**			openr on system-catalog type stuff, it will not
**			accidently get something it didn't expect.
**		5104 -- %0: invalid attribute name %1
**			An attribute name may not be called "tid",
**			since that is magic.
**		5105 -- %0: duplicate attribute name %1
**			Naturally, all attribute names must be unique.
**		5106 -- %0: invalid attribute format %2 on %1
**			Attribute formats can only be the usual 'i1',
**			'i2', 'i4', 'f4', 'f8', or 'cN', with N from
**			one to 255.  N can be zero if the relation is
**			generated internally ("_SYS..."); this is used
**			by decomp to avoid some nasty bugs.
**		5107 -- %0: excessive domain count on %1
**			There is a magic number (MAXDOM - 1) of domains
**			which is the maximum that may occur in a single
**			relation.  As the error documentation says:
**			"The origin of this magic number is obscure.
**			This is very difficult to change."  Oh well.
**		5108 -- %0: excessive relation width on attr %1
**			I wonder who came up with these AWFUL error messages.
**			Tuple widths may not exceed a magic number,
**			based heavily on the size of a page in UNIX.
**			The current value of THIS magic number (MAXTUP)
**			is 498 bytes.
**
**	History:
**		10/23/79 (6.2/8) (eric) -- null terminated relation filename
**			before creat() call so that systems which check
**			for names too long won't balk.
**		12/7/78 (rse) -- removed call to sys_catalog and put code
**				in line.
**		11/3/78 (rse) -- formatpg now resets the page itself so
**				call to resetacc removed.
**		11/1/78 (rse) -- views have no limit on tuple width.
**		10/30/78 (rse) -- added pageflush after inserting attributes.
**		8/17/78 (rse) -- fixed call to formatpg().
**			Changed calls to error() to have full error number
**		8/1/78 (eric) -- call to 'noclose' (for concurrency
**			reasons) moved to index.c, which is the only
**			place it was needed.  This made creatdb
**			simpler.
**		2/27/78 (eric) -- modified to take 'relstat' as param
**			zero, instead of the (ignored) storage struc-
**			ture.
*/





struct domain
{
	char	*name;
	char	frmt;
	char	frml;
};

/*
**  CREATE -- create new relation
**
**	This routine is the driver for the create module.
**
**	Parameters:
**		pc -- parameter count
**		pv -- parameter vector:
**			0 -- relation status (relstat) -- stored into
**				the 'relstat' field in the relation
**				relation, and used to determine the
**				caller.  Interesting bits are:
**
**				S_INDEX -- means called by the index
**					processor.  If set, the 'relindxd'
**					field will also be set to -1
**					(SECINDEX) to indicate that this
**					relation is a secondary index.
**				S_CATALOG -- this is a system catalog.
**					If set, this create was called
**					from creatdb, and the physical
**					file is not created.  Also, the
**					expiration date is set infinite.
**				S_VIEW -- this is a view.  Create has
**					been called by the 'define'
**					statement, rather than the
**					'create' statement.  The physical
**					file is not created.
**
**			1 -- relation name.
**			2 -- attname1
**			3 -- format1
**			4, etc -- attname, format pairs.
**
**	Returns:
**		zero -- successful create.
**		else -- failure somewhere.
**
**	Side Effects:
**		A relation is created (this is a side effect?).  This
**		means entries in the 'relation' and 'attribute' cata-
**		logs, and (probably) a physical file somewhere, with
**		one page already in it.
**
**	Requires:
**		opencatalog -- to open the 'relation' and 'attribute'
**			catalogs into 'Reldes' and 'Attdes' resp.
**		initstructs -- to initialize relation and attribute
**			tuples.
**		chk_att -- to check each attribute for validity:
**			good name, correct format, no dups, etc.
**		setcsl -- to set a critical section lock around the
**			section of code to physically create a file.
**			For concurrency reasons, this is the "true"
**			test for existance of a relation.
**		unlcs -- to remove locks, of course.
**		formatpg -- a mystic routine that outputs the initial
**			page of the relation (empty) so that the
**			access methods will not choke later.
**		insert -- to insert tuples into the 'relation' and
**			'attribute' catalogs.
**
**	Called By:
**		overlaya, overlaym
**		(maybe other overlay?)
**		creatdb
**		index
**
**	Trace Flags:
**		2.* -- entry message
**
**	Diagnostics:
**		5102 -- duplicate relation name
**		5103 -- renaming system catalog
**		5104 -- invalid attribute name
**		5105 -- duplicate attribute name
**		5106 -- invalid attribute format spec
**		5107 -- too many domains
**		5108 -- tuple too wide
**
**	Syserrs:
**		create: creat %s
**			The 'creat' call failed, probably meaning
**			that the directory is not writable, the file
**			exists mode zero, or this process is not
**			running as 'ingres'.
**		create: formatpg %d
**			The 'formatpg' routine failed.  Could be
**			because of a lack of disk space.  The number
**			is the return from formatpg; check it for
**			more details.
**		create: insert(rel, %s) %d
**			The insert for relid %s into the 'relation'
**			catalog failed; %d is the return from insert.
**			Check insert for details.
**
**	History:
**		8/1/78 (eric) -- 'noclose' call moved to index.c.
**		2/27/78 (eric) -- changed to take 'relstat' as pv[0]
**			instead of the (ignored) relspec.
*/

create(pc, pv)
int	pc;
char	**pv;
{
	register char			**pp;
	register int			i;
	int				bad;
	struct domain			domain[MAXDOM];
	struct domain			*dom;
	char				*relname, tempname[MAXNAME+3];
	struct tup_id			tid;
	struct relation			rel, key;
	struct attribute		att;
	struct descriptor		desr;
	extern char			*Usercode;
	extern struct descriptor	Reldes, Attdes;
	extern int			errno;
	register int			relstat;
	long				temptid;
	long				npages;
	int				fdes;

#	ifdef xZTR1
	if (tTf(2, -1))
		printf("creating %s\n", pv[1]);
#	endif
	pp = pv;
	relstat = oatoi(pp[0]);
	/*
	**	If this database has query modification, then default
	**	to denial on all user relations.
	**	(Since views cannot be protected, this doesn't apply to them)
	*/
	if ((Admin.adhdr.adflags & A_QRYMOD) && ((relstat & (S_VIEW || S_CATALOG)) == 0))
		relstat |= (S_PROTALL | S_PROTRET);
	relname = *(++pp);
	ingresname(relname, Usercode, rel.relid);
	bmove(rel.relid, att.attrelid, MAXNAME + 2);
	opencatalog("relation", 2);

	/* check for duplicate relation name */
	if ((relstat & S_CATALOG) == 0)
	{
		if (openr(&desr, -1, relname) == 0)
		{
			if (bequal(desr.relowner, rel.relowner, 2))
			{
				return (error(5102, relname, 0));	/* bad relname */
			}
			if (desr.relstat & S_CATALOG)
			{
				return (error(5103, relname, 0));	/* attempt to rename system catalog */
			}
		}
	}
	opencatalog("attribute", 2);

	/* initialize structures for system catalogs */
	initstructs(&att, &rel);
	rel.relstat = relstat;
	if ((relstat & S_CATALOG) != 0)
		rel.relsave = 0;
	else if ((relstat & S_INDEX) != 0)
		rel.relindxd = SECINDEX;
	
#	ifdef xZTR3
	if (tTf(2, 2))
		printup(&Reldes, &rel);
#	endif

	/* check attributes */
	pp++;
	for (i = pc - 2; i > 0; i -= 2)
	{
		bad = chk_att(&rel, pp[0], pp[1], domain);
		if (bad != 0)
		{
			return (error(bad, relname, pp[0], pp[1], 0));
		}
		pp += 2;
	}

	/*
	** Create files if appropriate. Concurrency control for
	** the create depends on the actual file. To prevent
	** to users with the same usercode from creating the
	** same relation at the same time, their is check
	** on the existence of the file. The important events are
	** (1) if a tuple exists in the relation relation then
	** the relation really exists. (2) if the file exists then
	** the relation is being created but will not exist for
	** use until the relation relation tuple is present.
	** For VIEWS, the file is used for concurrency control
	** during the create but is removed afterwards.
	*/
	if ((relstat & S_CATALOG) == 0)
	{
		/* for non system named temporary relations
		** set a critical section lock while checking the
		** existence of a file.  If it exists, error return(5102)
		** else create file.
		*/
		temptid = 0;
		if (Lockrel && (!bequal(rel.relid,"_SYS",4)))
		{
			temptid = -1;
			setcsl(temptid);	/* set critical section lock */
			if ((fdes = open(rel.relid,0)) >= 0)
			{
						/* file already exists */
				close(fdes);
				unlcs(temptid);	/* release critical section lock */
				return (error(5102, relname, 0));
			}
			errno = 0;	/* file doesn't exist */
		}
		ingresname(rel.relid, rel.relowner, tempname);
		desr.relfp = creat(tempname, FILEMODE);
		if (temptid != 0)
			unlcs(temptid);	/* release critical section lock */
		if (desr.relfp < 0)
			syserr("create: creat %s", rel.relid);
		desr.reltid = -1;	/* init reltid to unused */
		if ((relstat & S_VIEW) == 0)
		{
			npages = 1;
			if (i = formatpg(&desr, npages))
				syserr("syserr: formatpg %d", i);
		}

		close(desr.relfp);
	}

	/* insert attributes into attribute relation */
	pp = pv + 2;
	dom = domain;
	for (i = pc - 2; i > 0; i -= 2)
	{
		ins_att(&Attdes, &att, dom++);
		pp += 2;
	}

	/*
	** Flush the attributes. This is necessary for recovery reasons.
	** If for some reason the relation relation is flushed and the
	** machine crashes before the attributes are flushed, then recovery
	** will not detect the error.
	** The call below cannot be a "noclose" without major changes to
	** creatdb.
	*/
	if (i = pageflush(0))
		syserr("create:flush att %d", i);

	if (i = insert(&Reldes, &tid, &rel, FALSE))
		syserr("create: insert(rel, %.14s) %d", rel.relid, i);

	if (relstat & S_VIEW)
		unlink(tempname);
	return (0);
}



/*
**  CHK_ATT -- check attribute for validity
**
**	The attribute is checked to see if
**	* it's name is ok (within MAXNAME bytes)
**	* it is not a duplicate name
**	* the format specified is legal
**	* there are not a ridiculous number of attributes
**	  (ridiculous being defined as anything over MAXDOM - 1)
**	* the tuple is not too wide to fit on one page
**
**	Parameters:
**		rel -- relation relation tuple for this relation.
**		attname -- tentative name of attribute.
**		format -- tentative format for attribute.
**		domain -- a 'struct domain' used to determine dupli-
**			cation, and to store the resulting name and
**			format in.
**
**	Returns:
**		zero -- OK
**		5104 -- bad attribute name.
**		5105 -- duplicate attribute name.
**		5106 -- bad attribute format.
**		5107 -- too many attributes.
**		5108 -- tuple too wide.
**
**	Side Effects:
**		'rel' has the relatts and relwid fields updated to
**		reflect the new attribute.
**
**	Requires:
**		length -- to check length of 'attname' against MAXNAME.
**		dup_att -- to check for duplicate attribute and
**			initialize 'name' field of 'domain' struct.
**		formck -- to check and convert the attribute format.
**
**	Called By:
**		create
**
**	Trace Flags:
**		2.1 -- entry print
**
**	Diagnostics:
**		as noted in the return.
**
**	Syserrs:
**		none
**
**	History:
**		11/1/78 (rse) -- views have no limit on tuple width.
**		2/27/78 (eric) -- documented.
*/

chk_att(rel, attname, format, domain)
struct relation		*rel;
char			*attname, *format;
struct domain		domain[];
{
	register int			i;
	register struct relation	*r;

	r = rel;

#	ifdef xZTR3
	if (tTf(2, 1))
		printf("chk_att %s %s\n", attname, format);
#	endif

	if (sequal(attname, "tid"))
		return (5104);		/* bad attribute name */
	if ((i = dup_att(attname, r->relatts, domain)) < 0)
		return (5105);		/* duplicate attribute */
	if (formck(format, &domain[i]))
		return (5106);		/* bad attribute format */
	r->relatts++;
	r->relwid += domain[i].frml & 0377;
	if (r->relatts >= MAXDOM)
		return (5107);		/* too many attributes */
	if (r->relwid > MAXTUP && (r->relstat & S_VIEW) == 0)
		return (5108);		/* tuple too wide */
	return (0);
}




/*
**  INS_ATT -- insert attribute into attribute relation
**
**	Parameters:
**		des -- relation descriptor for the attribute catalog.
**		att -- attribute tuple, preinitialized with all sorts
**			of good stuff (everything except 'attname',
**			'attfrmt', and 'attfrml'; 'attid' and 'attoff'
**			must be initialized to zero before this routine
**			is called the first time.
**		dom -- 'struct domain' -- the information needed about
**			each domain.
**
**	Returns:
**		none
**
**	Side Effects:
**		The 'att' tuple is updated in the obvious ways.
**		A tuple is added to the 'attribute' catalog.
**
**	Requires:
**		insert
**		pmove -- to make the attribute name nice and clean.
**
**	Called By:
**		create
**
**	Trace Flags:
**		none currently
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		ins_att: insert(att, %s)
**			The insert into the attribute catalog failed.
**
**	History:
**		2/27/78 (eric) -- result of insert checked.
*/

ins_att(des, att, dom)
struct descriptor	*des;
struct attribute	*att;
struct domain		*dom;
{
	register int		i;
	struct tup_id		tid;
	register struct domain	*d;

	d = dom;

	pmove(d->name, att->attname, MAXNAME, ' ');
	att->attfrmt = d->frmt;
	att->attfrml = d->frml;
	att->attid++;
	if (insert(des, &tid, att, FALSE))
		syserr("ins_att: insert(att, %s)", d->name);
	att->attoff += att->attfrml & 0377;
}




/*
**  DUP_ATT -- check for duplicate attribute
**
**	The attribute named 'name' is inserted into the 'attalias'
**	vector at position 'count'.  'Count' should be the count
**	of existing entries in 'attalias'.  'Attalias' is checked
**	to see that 'name' is not already present.
**
**	Parameters:
**		name -- the name of the attribute.
**		count -- the count of attributes so far.
**		domain -- 'struct domain' -- the list of domains
**			so far, names and types.
**
**	Returns:
**		-1 -- attribute name is a duplicate.
**		else -- index in 'domain' for this attribute (also
**			the attid).
**
**	Side Effects:
**		The 'domain' vector is extended.
**
**	Requires:
**		none
**
**	Called By:
**		chk_att
**
**	Trace Flags:
**		none
**
**	History:
**		2/27/78 (eric) -- documented.
*/

dup_att(name, count, domain)
char		*name;
int		count;
struct domain	domain[];
{
	register struct domain	*d;
	register int		lim;
	register int		i;

	lim = count;
	d = domain;

	for (i = 0; i < lim; i++)
		if (sequal(name, d++->name))
			return (-1);
	if (count < MAXDOM)
		d->name = name;
	return (i);
}




/*
**  INITSTRUCTS -- initialize relation and attribute tuples
**
**	Structures containing images of 'relation' relation and
**	'attribute' relation tuples are initialized with all the
**	information initially needed to do the create.  Frankly,
**	the only interesting part is the the expiration date
**	computation; longconst(9, 14976) is exactly the number
**	of seconds in one week.
**
**	Parameters:
**		att -- attribute relation tuple.
**		rel -- relation relation tuple.
**
**	Returns:
**		none
**
**	Side Effects:
**		'att' and 'rel' are initialized.
**
**	Requires:
**		time -- to get the current date.
**
**	Called By:
**		create
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		2/27/78 (eric) -- documented.
*/

initstructs(att1, rel1)
struct attribute	*att1;
struct relation		*rel1;
{
	register struct relation	*rel;
	register struct attribute	*att;
	extern long			longconst();

	rel = rel1;
	att = att1;

	/* setup expiration date (today + one week) */
	time(&rel->relsave);
	rel->relsave += longconst(9, 14976);

	rel->reltups = 0;
	rel->relatts = 0;
	rel->relwid = 0;
	rel->relprim = 1;
	rel->relspec = M_HEAP;
	rel->relindxd = 0;
	rel->relspare = 0;

	att->attxtra = 0;
	att->attid = 0;
	att->attoff = 0;
}



/*
**  CHECK ATTRIBUTE FORMAT AND CONVERT
**
**	The string 'a' is checked for a valid attribute format
**	and is converted to internal form.
**
**	zero is returned if the format is good; one is returned
**	if it is bad.  If it is bad, the conversion into a is not
**	made.
**
**	A format of CHAR can be length zero but a format
**	of 'c' cannot.
*/

formck(a, dom)
char		*a;
struct domain	*dom;
{
	int			len;
	register int		i;
	char			c;
	register char		*p;
	register struct domain	*d;

	p = a;
	c = *p++;
	d = dom;

	if (atoi(p, &len) != 0)
		return (1);
	i = len;

	switch (c)
	{

	  case INT:
	  case 'i':
		if (i == 1 || i == 2 || i == 4)
		{
			d->frmt = INT;
			d->frml = i;
			return (0);
		}
		return (1);

	  case FLOAT:
	  case 'f':
		if (i == 4 || i == 8)
		{
			d->frmt = FLOAT;
			d->frml = i;
			return (0);
		}
		return (1);

	  case 'c':
		if (i == 0)
			return (1);
	  case CHAR:
		if (i > 255 || i < 0)
			return (1);
		d->frmt = CHAR;
		d->frml = i;
		return (0);
	}
	return (1);
}
