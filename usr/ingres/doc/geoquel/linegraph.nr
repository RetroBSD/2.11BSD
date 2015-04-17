.th LINEGRAPH GEOQUEL 3/16/79
.sh NAME
linegraph - draw a connected scatter plot
.sh SYNOPSIS
.nf
.in +5
.ti -5
LINEGRAPH relname ON xdomain WITH ydomain IS "plotval"
{ [,] ydomain IS "plotval" }
[ [,] BOUNDS AT (lowx, lowy, highx, highy) ]
.in
.fi
.sh DESCRIPTION
Up to 10 y-domains may be plotted against
the one x-domain.
For visual correctness the x-domain must be
sorted in ascending or descending order.
For each ydomain the user must specify a character
or the name of a line type acceptable for the output device,
to be used for plotting.
If a character is specified rather than a line type,
solid lines are used and the character is placed
at the end of each line
The `plotval' need not be different for each
domain but visual clarity usually requires uniqueness.
The scaling for the axes is dependent upon the range of
data values in the relation.
When more than one y-domain is specified, the maximum
of all y-domains is used (see below).

While MAP, OVERLAY, and SHADE are constrained to work only
with maps (specifically defined relations), LINEGRAPH is free
to operate on any relation with 2 or more numeric domains.

LINEGRAPH assumes that the user wants the axes labeled from 0,0 to the
maximum value of the xdomain and the maximum value of all the
ydomains.
The optional `BOUNDS AT' clause allows the user to specify a certain
section of the graph to be drawn.  For example, in plotting sales
versus years, the years scale would be useless unless it was redefined
to range over a few years rather than all the years since
the year 1.
.sh EXAMPLE
.nf
.in +8
.ti -8
/* make sure that the xdomain is sorted properly */
.ti -8
MODIFY xyzcorp TO heapsort ON years

.ti -8
LINEGRAPH xyzcorp ON years
WITH toysales = "*"

.ti -8
/* limit years scale to 1965-1980 */
.ti -8
LINEGRAPH xyzcorp ON years
WITH toysales IS "*",
BOUNDS AT (1965, 0, 1980, 100)

.ti -8
/* use two y domains */
.ti -8
LINEGRAPH xyzcorp ON years
WITH toysales = "*",
candysales IS "X",
BOUNDS AT (1965, 0, 1980, 100)
.in -8
.fi
.sh "SEE ALSO"
POINTGRAPH(GEOQUEL)
.sh BUGS
It would be nice if the method for re-specifying the axes range
values were more friendly.
