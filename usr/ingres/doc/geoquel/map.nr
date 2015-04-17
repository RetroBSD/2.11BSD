.th MAP GEOQUEL 12/15/77
.sh NAME
map \- display a map of relation
.sh SYNOPSIS
MAP relname [ON domain]
.sh DESCRIPTION
The map command displays the map `relname' with respect to the window
defined by the values in the MAPRELATION relation.  If no tuple for
the map exists in the MAPRELATION relation, then the map is scaled to
fit entirely on the screen, and an appropriate tuple is inserted into the
MAPRELATION relation.

If the optional `ON' clause is specified the value of `domain'
will be displayed over the center of a polygon.
The `ON' option is useful, for example, to label zones with their
zoneid's.
Domain may be any domain, character or numeric type.
.sh EXAMPLE
.nf
MAP mapofusa
MAP mapofusa ON zoneid
MAP mapofusa ON population
.fi
.sh "SEE ALSO"
.nf
GEO-QUEL(UNIX)
CENTER(GEOQUEL)
OVERLAY(GEOQUEL)
SHADE(GEOQUEL)
.fi
