# include	"../ingres.h"
# include	"../symbol.h"

/*
**	Clearkeys - reset all key indicators in descriptor
**
**	Clearkeys is used to clear the key supplied
**	flags before calls to setkey
*/


clearkeys(des)
struct descriptor	*des;
{
	register struct descriptor	*d;
	register int			i;

	d = des;
	for (i = 0; i <= d->relatts; i++)
		d->relgiven[i] = 0;
	return (0);
}

/*
**	Setkey - indicate a partial key for find
**
**	Setkey is used to set a key value into place
**	in a key. The key can be as large as the entire
**	tuple. Setkey moves the key value into the
**	proper place in the key and marks the value as
**	supplied
**
**	If the value is a null pointer, then the key is
**	cleared.
**
**	Clearkeys should be called once before the
**	first call to setkey.
**
*/

setkey(d1, key, value, domnum)
struct descriptor	*d1;
char			*key;
char			*value;
int			domnum;

{
	register struct descriptor	*d;
	register int			dom, len;
	char				*cp;

	d = d1;
	dom = domnum;

#	ifdef xATR1
	if (tTf(96, 0))
		printf("setkey: %.14s, %d\n", d->relid, dom);
#	endif

	/* check validity of domain number */
	if (dom < 1 || dom > d->relatts)
		syserr("setkey:rel=%.12s,dom=%d", d->relid, dom);

	/* if value is null, clear key */
	if (value == 0)
	{
		d->relgiven[dom] = 0;
		return;
	}

	/* mark as given */
	d->relgiven[dom] = 1;

	len = d->relfrml[dom] & I1MASK;
	cp = &key[d->reloff[dom]];

	if (d->relfrmt[dom] == CHAR)
		pmove(value, cp, len, ' ');
	else
		bmove(value, cp, len);
}
