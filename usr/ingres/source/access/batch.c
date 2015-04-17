# include	"../ingres.h"
# include	"../aux.h"
# include	"../catalog.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"../batch.h"
openbatch(rel_desc, index_desc, mode)
struct descriptor	*rel_desc, *index_desc;
int			mode;

/*
**	Open batch prepares for batch processing.
**	1. If the batch is already open, return an error
**	2. Create the batch file.
**	3. clear domain flags.
**	4. If the relation is indexed, Identify all the domains
**		which must be saved to speed the index update.
**	5. Set up specifics for each type of update.
**	6. Write out the batch structure.
**
**	The following itemizes what is saved (in bytes):
**	f(si) means it's a function of the secondary index keys
**	space for newtid is saved only if there is a sec. index.
**
**			mdDEL	mdREPL	mdAPP
**
**	oldtid		4	4	0
**	oldtuple	f(si)	f(si)	0
**	newtuple	0	tupwid	tupwid
**	newtid		0	4	4
*/

{
	register struct si_doms		*sp;
	register struct descriptor	*rel, *indx;
	int				i, saveoff, dom;
	char				*p, *batchname();
	long				lotid, hitid;
	struct index			itup;
	extern char			*Database;

	if (Batchhd.mode_up)
		return (-1);	/* batch already open */
	rel = rel_desc;
	indx = index_desc;
	p = batchname();	/* form batch name */
#	ifdef xATR1
	if (tTf(89, -1))
		printf("Openbatch %s\n", p);
#	endif
	if ((Batch_fp = creat(p, FILEMODE)) < 0)
		syserr("openbatch:can't creat %s,%d", p, Batch_fp);
	Batch_cnt = 0;

	/* copy the important info */
	smove(Fileset, Batchbuf.file_id);
	smove(Database, Batchhd.db_name);
	bmove(rel->relid, Batchhd.rel_name, MAXNAME);
	bmove(rel->relowner, Batchhd.userid, 2);
	Batchhd.mode_up = mode;
	Batchhd.num_updts = 0;

	/* clear out the secondary index domain flags */
	sp = Batchhd.si;	/* sp points to the structure */
	for (i = 1; i <= MAXDOM; i++)
	{
		sp->dom_size = 0;
		sp++;
	}
	Batchhd.si_dcount = 0;

	/* set up the tid and tuple sizes by type of update */
	/* assume size of tido, tidn, and tupn */
	Batchhd.tido_size = 4;	/* assume old tid is needed */
	Batchhd.tupo_size = 0;	/* assume old tuple isn't needed */
	Batchhd.tupn_size = rel->relwid;	/* assume new tuple is needed */
	Batchhd.tidn_size = 4;	/* assume space is needed for new tid */
	switch(Batchhd.mode_up)
	{

	  case mdDEL:
		Batchhd.tupn_size = 0;	/* new tuple isn't needed */
		Batchhd.tidn_size = 0;	/* new tid isn't needed */

	  case mdREPL:
		break;

	  case mdAPP:
		Batchhd.tido_size = 0;	/* old tid isn't needed */
		break;

	  default:
		syserr("openbatch:mode %d", Batchhd.mode_up);
	}
	/* if there are no secondary indexes then tipn isn't needed */
	if (rel->relindxd <= 0)
		Batchhd.tidn_size = 0;

	/* if this relation has a secondary index, figure out what to save */
	if (rel->relindxd > 0 && mode != mdAPP)
	{
		setkey(indx, &itup, rel->relid, IRELIDP);
		setkey(indx, &itup, rel->relowner, IOWNERP);

		if (find(indx, EXACTKEY, &lotid, &hitid, &itup))
			syserr("openbatch:bad find %.12s", rel);

		/* check each entry in "index" relation for useful index */
		while(!(i = get(indx, &lotid, &hitid, &itup, TRUE)))
		{
			if (!bequal(itup.irelidp, rel->relid, MAXNAME) ||
				!bequal(itup.iownerp, rel->relowner, 2))
				continue;
			/* found one. copy the used domains */
			p = itup.idom;		/* get address of first */
			i = 6;
			while (i--)
			{
				if ((dom = *p++) == 0)
					break;	/* no more domains */
				sp = &Batchhd.si[dom];
				if (sp->dom_size != 0)
					continue;	/* domain has already been done once */
				Batchhd.si_dcount++;
				Batchhd.tupo_size += rel->relfrml[dom] & 0377;
				sp->rel_off = rel->reloff[dom];
				sp->dom_size = rel->relfrml[dom] & 0377;
				sp->tupo_off = saveoff;
				saveoff += sp->dom_size;
			}
		}
		if (i < 0)
			syserr("openbatch:bad get index %d", i);
		/* compute offsets of domains in saved "oldtuple" */
		saveoff = 0;
		sp = Batchhd.si;
		i = Batchhd.si_dcount;
		while (i--)
		{
			/* skip to next domain */
			while (sp->dom_size == 0)
				sp++;
			sp->tupo_off = saveoff;
			saveoff += sp->dom_size;
			sp++;
		}
	}
	wrbatch(&Batchhd, sizeof Batchhd);
	return (0);
}

addbatch(oldtid, newtuple, oldtuple)
long	*oldtid;
char	*newtuple, *oldtuple;
{
	long			newtid;
	register int		i;
	register struct si_doms	*sp;
	register char		*old;

#	ifdef xATR1
	if (tTf(89,3))
		printf("addbatch\n");
#	endif
	if (Batchhd.mode_up == 0)
		return (-1);
	Batchhd.num_updts++;	/* increment the number of add batches */
	old = oldtuple;
	/* write out the old tid */
	wrbatch(oldtid, Batchhd.tido_size);

	/* write out each of the old tuple domains */
	i = Batchhd.si_dcount;	/* i get the number of domains */
	sp = Batchhd.si;	/* sp points to the domain structures */

	while (i--)
	{
		/* skip to the next domain */
		while (sp->dom_size == 0)
			sp++;

		wrbatch(&old[sp->rel_off], sp->dom_size);
		sp++;
	}

	/* write out the new tuple */
	wrbatch(newtuple, Batchhd.tupn_size);

	/* reserve space for the new tid. Init to -1 */
	newtid = -1;
	wrbatch(&newtid, Batchhd.tidn_size);
	return (0);
}

closebatch()
{
	register int	i;
	long		zero;

	if (Batchhd.mode_up == 0)
		return (-1);
	flushbatch();	/* write out any remainder */
	zero = 0;
	if ((i = lseek(Batch_fp, zero, 0)) < 0)
		syserr("closebatch:seek %d", i);
	wrbatch(&Batchhd, sizeof Batchhd);	/* update num_updts */
	flushbatch();
	if (i = close(Batch_fp))
		syserr("closebatch:close %d", i);
	Batchhd.mode_up = 0;
	return (0);
}
