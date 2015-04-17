# include	"../ingres.h"

/*
**	Remove tuples from the specified system relation.
**
**	'Desa' is a descriptor for a system relation and
**	key[12] and dom[12] are the keys and domain numbers
**	to match on for the delete.
**	All the tuples in 'desa' with key1 and key2
**	are deleted from the relation.
*/

purgetup(desa, key1, dom1, key2, dom2)
struct descriptor	*desa;
char			*key1;
int			dom1;
char			*key2;
int			dom2;
{
	struct tup_id			tid, limtid;
	register int			i;
	char				tupkey[MAXTUP], tuple[MAXTUP];
	register struct descriptor	*d;

	d = desa;

	setkey(d, tupkey, key1, dom1);
	setkey(d, tupkey, key2, dom2);
	if (i = find(d, EXACTKEY, &tid, &limtid, tupkey))
		syserr("purgetup:find:%d", i);
	while ((i = get(d, &tid, &limtid, tuple, TRUE)) == 0)
	{
		if (kcompare(d, tuple, tupkey) == 0)
			if (i = delete(d, &tid))
				syserr("attflush: delete %d", i);
	}

	if (i < 0)
		syserr("purgetup:get %.14s:%d", d->relid, i);
}
