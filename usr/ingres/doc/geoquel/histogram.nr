.th HISTOGRAM GEOQUEL 3/16/79
.sh NAME
histogram \- draw a histogram
.sh SYNOPSIS
.nf
.in +5
.ti -5
HISTOGRAM relname ON domname
[ CELLWIDTH IS <any number> ] [,]
[ CELLCOUNT IS <integer> ] [,]
[ LOWEST IS <any number> ] [,]
[ HIGHEST IS <any number> ]
.in
.fi
.sh DESCRIPTION
The histogram function partitions the data,
in the domain (domname) of the relation (relname),
into 10 cells or intervals.
The maximum value in the domain is used as an upper bound
but is not included in the highest interval.
The data values included in any interval are the lower
bound up to but not including the upper value.

The data range boundaries may be respecified
with the `lowest' and `highest' clauses.
The specified highest value will not be included
in the highest interval.
These clauses can be used to subset the data range
from the relation or provide uniform scaling for multiple graphs.

There are two methods of specifying the number of cells in the
graph.
The `cellcount' clause is obvious.
Exactly the specified number of cells is included in the graph.
When the `cellwidth' is used, GEO-QUEL computes the number
of cells and then re-computes the upper boundary since there may
not be an integer number of cells that fit within the data range.
The one exception to this is that if the user specifies only the
upper bound an the width, then the lower bound is recomputed,
otherwise, the upper bound will be recomputed.

If both `cellcount' and `cellwidth' are specified only on of
the data range boundaries may be specified.

The four optional clauses may be entered in any order but only once
per command.

.sh EXAMPLE
HISTOGRAM class ON midterm

.nf
HISTOGRAM class ON final
LOWEST = 0,
HIGHEST = 100,
CELLCOUNT IS 20
.fi
