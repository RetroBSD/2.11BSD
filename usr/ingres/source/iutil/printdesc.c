# include	"../ingres.h"

/*
**  PRINT RELATION DESCRIPTOR (for debugging)
**
**	A pointer of a file descriptor is passed.  All pertinent
**	info in that descriptor is printed on the standard output.
**
**	For debugging purposes only
*/

printdesc(d1)
struct descriptor	*d1;
{
	register struct descriptor	*d;
	register int			i;
	register int			end;

	d = d1;

	printf("Descriptor 0%o %.12s %.2s\n", d, d->relid, d->relowner);
	printf("spec %d, indxd %d, stat %d\n",
		d->relspec, d->relindxd, d->relstat);
	printf("save %s", locv(d->relsave));
	printf(", tups %s, atts %d, wid %d, prim ",
		locv(d->reltups), d->relatts, d->relwid);
	printf("%s\n", locv(d->relprim));
	printf("spare %s\n", locv(d->relspare));
	printf("fp %d, opn %d, tid %s", d->relfp, d->relopn,
		locv(d->reltid));
	printf(", adds %s\n", locv(d->reladds));

	end = d->relatts;
	for (i = 0; i <= end; i++)
	{
		printf("[%2d] off %3d frmt %2d frml %3d, xtra %3d, given %3d\n",
			i, d->reloff[i], d->relfrmt[i],
			d->relfrml[i] & 0377, d->relxtra[i], d->relgiven[i]);
	}
}
