# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"../lock.h"

/* tTf flag 92		TTF */

long rhash(des, key)
struct descriptor	*des;
char			key[MAXTUP];
/*
 * RHASH will perform a randomizing hash on the full key.
 *
 */
{
	register struct descriptor	*d;
	register int			i;
	register char			*cp;
	long				bucket;
	char				tmp;
	int				j, *k, knt, numeric;

	d = des;
	bucket = 0;
	knt = 0;
	for (i = 1; i <= d->relatts; i++)
		if (d->relxtra[i])
		{
			/* form pointer to field */
			cp = &key[d->reloff[i]];
			numeric = d->relfrmt[i] != CHAR;
			for (j = 0; j < (d->relfrml[i] & I1MASK); j++)
				if (((tmp = *cp++) != ' ') || numeric)
					addabyte(tmp, &bucket, knt++);
		}
	/* remove sign bit from bucket the hard way */
	k = (int *) &bucket;
	*k &= 077777;
#	ifdef xATR3
	if (tTf(92, 8))
		printf("rhash:hval=%s", locv(bucket));
#	endif
	bucket %= d->relprim;
#	ifdef xATR3
	if (tTf(92, 8))
		printf(",returning %s\n", locv(bucket));
#	endif
	return (bucket);
}

addabyte(ch, word, knt1)
char	ch;
long	*word;
int	knt1;
/*
 * ADDABYTE is used to map a long key into a four byte integer.
 * As bytes are added, they are first rotated, then exclusive ored
 * into the existing key.
 *
 */
{
	register int	knt;
	long		i;

	knt = knt1;
	i = ch & 0377;	/*get rid of any sign extension*/
	knt += 8 * (knt & 3);	/*alternately add 0, 8, 16 or 24 to knt */
	knt &= 037;
	*word ^= (i << (knt) | i >> (32 - knt));
}
