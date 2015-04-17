.th OVERLAY GEOQUEL 12/15/77
.sh NAME
overlay \- perform `ON domain' function of MAP command
.sh SYNOPSIS
OVERLAY relname WITH domain
.sh DESCRIPTION
Performs the same function as the `ON domain' option of the MAP command.
The map itself is not drawn.  The contents of `domain' are drawn at the
center of each polygon.  OVERLAY can be used to identify zones in a shaded
map as shown in the example.
This is primarily useful for labeling
maps that have been drawn at an earlier time.
.sh EXAMPLE
.nf
/* mark each shaded polygon with its zoneid */
OVERLAY mapofusa WITH zoneid
.fi
.sh "SEE ALSO"
.nf
SHADE(GEOQUEL)
MAP(GEOQUEL)
.fi
