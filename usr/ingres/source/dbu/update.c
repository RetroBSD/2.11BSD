# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"../batch.h"

/*
**	Update reads a batch file written by the
**	access method routines (openbatch, addbatch, closebatch)
**	and performs the updates stored in the file.
**
**	It assumes that it is running in the database. It
**	is driven by the data in the Batchhd struct (see ../batch.h).
**	If the relation has a secondary index then update calls
**	secupdate. As a last step the batch file is removed.
**
**	The global flag Batch_recovery is tested in case
**	of an error. It should be FALSE if update is being
**	run as the dbu deferred update processor. It should
**	be TRUE if it is being used as part of the recovery
**	procedure.
*/

update()
{
	register int		i, mode;
	struct descriptor	rel;
	long			oldtid, tupcnt;
	char			oldtup[MAXTUP], newtup[MAXTUP];
	char			*batchname();
	extern int		Dburetflag;
	extern struct retcode	Dburetcode;

#	ifdef xZTR1
	if (tTf(15, -1))
		printf("Update on %s\n", batchname());
#	endif
	/* set up to read batchhd */
	Batch_cnt = BATCHSIZE;	/* force a read on next getbatch */
	Batch_dirty = FALSE;
	if ((Batch_fp = open(batchname(), 2)) < 0)
		syserr("prim:can't open %s", batchname());
	getbatch(&Batchhd, sizeof Batchhd);

	tupcnt = Batchhd.num_updts;
#	ifdef xZTR1
	if (tTf(15, 0))
		printf("rel=%s tups=%s\n", Batchhd.rel_name, locv(tupcnt));
#	endif
	Dburetflag = TRUE;
	Dburetcode.rc_tupcount = 0;
	if (!tupcnt)
	{
		rmbatch();
		return (1);
	}

	/* update the primary relation */
	if (i = openr(&rel, 2, Batchhd.rel_name))
		syserr("prim:can't openr %s %d", Batchhd.rel_name, i);
	mode = Batchhd.mode_up;

	while (tupcnt--)
	{
		getbatch(&oldtid, Batchhd.tido_size);	/* read old tid */
		getbatch(oldtup, Batchhd.tupo_size);	/* and portions of old tuple */
		getbatch(newtup, Batchhd.tupn_size);	/* and the newtup */

		switch (mode)
		{

		  case mdDEL:
			if ((i = delete(&rel, &oldtid)) < 0)
				syserr("prim:bad del %d %s", i, Batchhd.rel_name);
			break;

		  case mdREPL:
			if (i = replace(&rel, &oldtid, newtup, TRUE))
			{
				/* if newtuple is a duplicate, then ok */
				if (i == 1)
					break;
				/* if this is recovery and oldtup not there, try to insert newtup */
				if (Batch_recovery && i == 2)
					goto upinsert;
				syserr("prim:Non-functional replace on %s (%d)", i, Batchhd.rel_name);
			}
			Dburetcode.rc_tupcount++;
			break;

		  case mdAPP:
		  upinsert:
			if ((i = insert(&rel, &oldtid, newtup, TRUE)) < 0)
				syserr("prim:bad insert %d %s", i, Batchhd.rel_name);
			break;

		  default:
			syserr("prim:impossible mode %d", mode);
		}
		putbatch(&oldtid, Batchhd.tidn_size);	/* write new tid if necessary */
	}
	/* fix the tupchanged count if delete or append */
	if (mode != mdREPL)
		Dburetcode.rc_tupcount = rel.reladds >= 0 ? rel.reladds : -rel.reladds;
	/* close the relation but secupdate will still use the decriptor */
	if (i = closer(&rel))
		syserr("prim:close err %d %s", i, Batchhd.rel_name);
	batchflush();

	/* if this relation is indexed, update the indexes */
	if (rel.relindxd > 0)
		secupdate(&rel);
	rmbatch();

#	ifdef xZTR1
	if (tTf(15, 2))
		printf("%s tups changed\n", locv(Dburetcode.rc_tupcount));
#	endif
	return (0);
}
