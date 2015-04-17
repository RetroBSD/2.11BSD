# include	"../ingres.h"
# include	"../symbol.h"
# include	"../access.h"


/* tTf flag 87	TTF	put_tuple() */

put_tuple(tid, tuple, length)
struct tup_id	*tid;
char		*tuple;
int		length;

/*
**	Put the canonical tuple in the position
**	on the current page specified by tid
*/

{
	register char	*tp;
	char		*get_addr();

#	ifdef xATR2
	if (tTf(87, 0))
	{
		printf("put_tuple:len=%d,", length);
		dumptid(tid);
	}
#	endif

	/* get address in buffer */
	tp = get_addr(tid);

	/* move the tuple */
	bmove(tuple, tp, length);

	/* mark page as dirty */
	Acc_head->bufstatus |= BUF_DIRTY;
}


canonical(dx, tuplex)
struct descriptor	*dx;
char			*tuplex;

/*
**	Make the tuple canonical and return the length
**	of the tuple.
**
**	If the relation is compressed then the tuple in
**	compressed into the global area Accanon.
**
**	As a side effect, the address of the tuple to be
**	inserted is placed in Acctuple.
**
**	returns: length of canonical tuple
*/

{
	register struct descriptor	*d;
	register char			*tuple;
	register int			i;

	d = dx;
	tuple = tuplex;

	if (d->relspec < 0)
	{
		/* compress tuple */
		i = comp_tup(d, tuple);
		Acctuple = Accanon;
	}
	else
	{
		Acctuple = tuple;
		i = d->relwid;
	}
	return (i);
}


comp_tup(dx, tuple)
struct descriptor	*dx;
char			*tuple;

/*
**	Compress the tuple into Accanon. Compression is
**	done by copying INT and FLOAT as is.
**	For CHAR fields, the tuple is copied until a null
**	byte or until the end of the field. Then trailing
**	blanks are removed and a null byte is inserted at
**	the end if any trailing blanks were present.
*/

{
	register struct descriptor	*d;
	register char			*src, *dst;
	char				*save;
	char				*domlen, *domtype;
	int				i, j, len;

	src = tuple;
	d = dx;
	dst = Accanon;

	domlen = &d->relfrml[1];
	domtype = &d->relfrmt[1];

	for (i = 1; i <= d->relatts; i++)
	{
		len = *domlen++ & I1MASK;
		if (*domtype++ == CHAR)
		{
			save = src;
			for (j = 1; j <= len; j++)
			{
				if ((*dst++ = *src++) == NULL)
				{
					dst--;
					break;
				}
			}

			while (j--)
				if (*--dst != ' ')
					break;

			if (j != len)
				*++dst = NULL;

			dst++;
			src = save + len;
		}
		else
		{
			while (len--)
				*dst++ = *src++;

		}
	}
	return (dst - Accanon);
}


space_left(bp)
struct accbuf	*bp;

/*
**	Determine how much space remains on the page in
**	the current buffer. Included in computation
**	is the room for an additional line number
*/

{
	register int		nextoff;
	register struct accbuf	*buf;

	buf = bp;

	nextoff = buf->nxtlino;

	return (PGSIZE - buf->linetab[-nextoff] - (nextoff + 2) * 2);
}
