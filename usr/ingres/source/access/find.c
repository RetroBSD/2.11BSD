# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"../lock.h"

/* tTf flag 92		TTF */

/*
**	Find - determine limits for scan of a relation
**
**	Find determines the values of an initial TID
**	and an ending TID for scanning a relation.
**	The possible calls to find are:
**
**	find(desc, NOKEY, lotid, hightid)
**		sets tids to scan entire relation
**
**	find(desc, EXACTKEY, lotid, hightid, key)
**		sets tids according to structure
**		of the relation. Key should have
**		been build using setkey.
**
**	find(desc, LRANGEKEY, lotid, hightid, keylow)
**		Finds lotid less then or equal to keylow
**		for isam relations. Otherwise scans whole relation.
**		This call should be followed by a call with HRANGEKEY.
**
**	find(desc, HRANGEKEY, lotid, hightid, keyhigh)
**		Finds hightid greater than or equal to
**		keyhigh for isam relations. Otherwise sets
**		hightid to maximum scan.
**
**	find(desc, FULLKEY, lotid, hightid, key)
**		Same as find with EXACTKEY and all keys
**		provided. This mode is used only by findbest
**		and replace.
**
**	returns:
**		<0 fatal error
**		 0 success
*/



find(d1, mode, lotid1, hightid, key)
struct descriptor	*d1;
int			mode;
struct tup_id		*lotid1;
struct tup_id		*hightid;
char			*key;
{
	register struct descriptor	*d;
	register struct tup_id		*lotid;
	register int			ret;
	int				keyok;
	long				pageid;
	long				rhash();

	d = d1;
	lotid = lotid1;

#	ifdef xATR1
	if (tTf(92, 0))
	{
		printf("find: m%d,s%d,%.14s\n", mode, d->relspec, d->relid);
		if (mode != NOKEY)
			printup(d, key);
	}
#	endif

	ret = 0;	/* assume successful return */
	keyok = FALSE;

	switch (mode)
	{

	  case EXACTKEY:
		keyok = fullkey(d);
		break;

	  case FULLKEY:
		keyok = TRUE;

	  case NOKEY:
	  case LRANGEKEY:
	  case HRANGEKEY:
		break;

	  default:
		syserr("FIND: bad mode %d", mode);
	}

	/* set lotid for beginning of scan */
	if (mode != HRANGEKEY)
	{
		pageid = 0;
		stuff_page(lotid, &pageid);
		lotid->line_id = -1;
	}

	/* set hitid for end of scan */
	if (mode != LRANGEKEY)
	{
		pageid = -1;
		stuff_page(hightid, &pageid);
		hightid->line_id = -1;
	}

	if (mode != NOKEY)
	{
		switch (abs(d->relspec))
		{
	
		  case M_HEAP:
			break;
	
		  case M_ISAM:
			if (mode != HRANGEKEY)
			{
				/* compute lo limit */
				if (ret = ndxsearch(d, lotid, key, -1, keyok))
					break;	/* fatal error */
			}
	
			/* if the full key was provided and mode is exact, then done */
			if (keyok)
			{
				bmove(lotid, hightid, sizeof *lotid);
				break;
			}
	
			if (mode != LRANGEKEY)
				ret = ndxsearch(d, hightid, key, 1, keyok);
			break;
	
		  case M_HASH:
			if (!keyok)
				break;		/* can't do anything */
			pageid = rhash(d, key);
			stuff_page(lotid, &pageid);
			stuff_page(hightid, &pageid);
			break;

		  default:
			ret = acc_err(AMFIND_ERR);
		}
	}

#	ifdef xATR2
	if (tTf(92, 1))
	{
		printf("find: ret %d\tlow", ret);
		dumptid(lotid);
		printf("hi");
		dumptid(hightid);
	}
#	endif
	return (ret);
}

fullkey(des)
struct descriptor	*des;
/*
 * This routine will check that enough of the tuple has been specified
 * to enable a key access.
 *
 */
{
	register struct descriptor	*d;
	register int			i;

	d = des;
	for (i = 1; i <= d->relatts; i++)
		if (d->relxtra[i] && !d->relgiven[i])
			return (FALSE);
	return (TRUE);
}
