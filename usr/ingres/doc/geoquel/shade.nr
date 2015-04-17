.th SHADE GEOQUEL 12/15/77
.sh NAME
shade \- shade a relation based on data in 1-4 domains
.sh SYNOPSIS
.nf
.in +5
.ti -5
SHADE relname WITH domain IS "shadeval"
{ [,] domain IS "shadeval" }
[ [,] DENSITY IS dens ]
.in
.fi
.sh DESCRIPTION
The shade command displays a map, shaded
with shadeval on the data in
the specified domain.
Shading may be dome on one to four
different domains at a time.
The maximum of four was chosen for 
visual clarity; more than four characters tend 
to become indiscernible.

Shadeval may be any single character or
one of the line types:
solid, dotted, dotdash, shortdash, longdash.

The
optional density constant allows the user to specify the resolution 
of the shading.
By increasing the density factor, more points or lines will be placed in each
zone, thereby causing the shading of zones which would otherwise be too
sparse to show up.
The default value for `dens' is 1.0.

.sh EXAMPLE
.nf
SHADE mapofusa WITH counties = "*"
SHADE mapofusa WITH persons IS "*", autos IS "X"
SHADE mapofusa WITH trees = "*", DENSITY = 2.0
SHADE mapofusa WITH persons IS "*", autos = "solid"
.fi
.sh "SEE ALSO"
.nf
MAP(GEOQUEL)
CENTER(GEOQUEL)
OVERLAY(GEOQUEL)
.fi
.sh BUGS
When trying to shade non-closed polygons
the results become undefined.
This is not checked due to the large overhead.

The GT40 and GT42 do not have hardware
generated dotted lines like the Tektronix 4014
so solid lines are substituted when necessary.

Since there is currently no facility to rotate
lines there cannot be cross-hatched shading.
Therefore lines for shading are always horizontal.
