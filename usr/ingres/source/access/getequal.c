# include	"../ingres.h"
# include	"../access.h"

/* tTf flag 97		TTF */

/*
getequal - get the first tuple equal to the provided key
DESCRIPTION
GETEQUAL is used to do a keyed retrieval of a single
tuple in cases where the calling program knows the key to
be unique.  SETKEY must be called first to set all desired
domain values.  GETEQUAL will return the first tuple with
equality on all of the specified domains.
The tuple is returned in TUPLE.

	function values:

		<0 fatal error
		 0  success
		 1  no match
 */


getequal(d1, key, tuple, tid)
struct descriptor	*d1;
char			key[MAXTUP];
char			tuple[MAXTUP];
struct tup_id		*tid;
{
	struct tup_id			limtid;
	register struct descriptor	*d;
	register int			i;

	d = d1;

#	ifdef xATR1
	if (tTf(97, 0))
	{
		printf("getequal: %.14s,", d->relid);
		printup(d, key);
	}
#	endif
	if (i = find(d, EXACTKEY, tid, &limtid, key))
		return (i);
	while ((i = get(d, tid, &limtid, tuple, TRUE)) == 0)
	{
		if (!kcompare(d, key, tuple))
		{
#			ifdef xATR2
			if (tTf(97, 1))
			{
				printf("getequal: ");
				dumptid(tid);
				printf("getequal: ");
				printup(d, tuple);
			}
#			endif
			return (0);
		}
	}
#	ifdef xATR2
	if (tTf(97, 2))
		printf("getequal: %d\n", i);
#	endif
	return (i);
}
