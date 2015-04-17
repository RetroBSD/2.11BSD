# include	"../ingres.h"
# include	"../access.h"
# include	"../symbol.h"

/*
**	Routine associated with getting a tuple out of
**	the current buffer.
*/

get_tuple(dx, tid, tuple)
struct descriptor	*dx;
struct tup_id		*tid;
char			*tuple;

/*
**	Get_tuple - take the tuple specified
**	by tid and move it to "tuple"
*/

{
	register struct descriptor	*d;
	register char			*cp;
	char				*get_addr();

	d = dx;

	cp = get_addr(tid);

	if (d->relspec < 0)
		uncomp_tup(d, cp, tuple);	/* compressed tuple */
	else
		bmove(cp, tuple, d->relwid);	/* uncompressed tuple */
}


char *getint_tuple(dx, tid, tuple)
struct descriptor	*dx;
struct tup_id		*tid;
char			*tuple;

/*
**	Getint_tuple - get the tuple specified by
**	tid. If possible avoid moving the tuple.
**	Return value is address of tuple.
*/

{
	register struct descriptor	*d;
	register char			*cp, *ret;
	char				*get_addr();

	d = dx;

	cp = get_addr(tid);

	if (d->relspec < 0)
	{
		ret = tuple;
		uncomp_tup(d, cp, ret);	/* compressed tuple */
	}
	else
		ret = cp;			/* uncompressed tuple */
	return (ret);
}


char *get_addr(tidx)
struct tup_id	*tidx;

/*
**	Routine to compute the address of a tuple
**	within the current buffer.
**	Syserr if specified tuple deleted.
*/

{
	register struct tup_id	*tid;
	register int		offset;

	tid = tidx;

	offset = Acc_head->linetab[-(tid->line_id & I1MASK)];
	if (offset == 0)
	{
		syserr("get_addr %s", locv(Acc_head->rel_tupid));
	}
	return (&((struct raw_accbuf *)Acc_head)->acc_buf[offset]);
}



uncomp_tup(dx, cp, tuple)
struct descriptor	*dx;
char			*cp;
char			*tuple;

/*
**	Uncompress - decompress the tuple at address cp
**	according to descriptor.
*/

{
	register struct descriptor	*d;
	register char			*src, *dst;
	int				i, j;

	d = dx;
	dst = tuple;
	src = cp;

	/* for each domain, copy and blank pad if char */
	for (i = 1; i <= d->relatts; i++)
	{
		j = d->relfrml[i] & I1MASK;
		if (d->relfrmt[i] == CHAR)
		{
			while (j--)
			{
				if ((*dst++ = *src++) == NULL)
				{
					/* back up one */
					dst--;
					j++;
					break;
				}
			}

			/* blank pad if necessary */
			while (j-- > 0)
				*dst++ = ' ';
		}
		else
		{
			while (j--)
				*dst++ = *src++;
		}
	}
}


invalid(tidx)
struct tup_id	*tidx;

/*
**	Check if a line number is valid.
**	If linenumber is illegal return AMINVL_ERR
**	if Line has been deleted return 2
**	else return 0
*/

{
	register struct tup_id	*tid;
	register int		i;

	tid = tidx;

	i = tid->line_id & I1MASK;

	if (i >= Acc_head->nxtlino)
		return (acc_err(AMINVL_ERR));

	if (Acc_head->linetab[-i] == 0)
		return (2);

	return (0);
}
