.th CENTER GEOQUEL 3/16/79
.sh NAME
center \- change window on map
.sh SYNOPSIS
CENTER relname AT x, y WITH MAGNITUDE mag
.sh DESCRIPTION
This command alters the logical center
and magnitude of the map `relname'.
The quantities x, y and mag define a window for viewing specific sections
of a map.
The window is originally set to display the entire map at the largest
possible magnitude.
The original values are set up by the MAP, SHADE, and OVERLAY commands.

Once the window has been re-defined, MAP, SHADE, and OVERLAY
will only display the portion of the map that falls within the boundaries
of the window.

Changes to the values x, y will shift the entire picture with the window
based on the new values of x, y.
(E.g. A new x that is larger than the existing x
will shift the plot to the left.)
Changes to mag affect the size of the map.
Due to some internal constraints, an increase in `mag' will decrease the size of
the map, and vice-versa.

If the optional clause is not specifed,
the current values of x, y and mag will
be printed and new values requested.
If you want to keep one
or two of the current values then
only type a newline when the new
value is requested.
.sh EXAMPLE
.nf
/* shift `mapofusa' to the right */
CENTER mapofusa

Current center definition:
x=51.3, y=42.1, mag=78.0
New x center: 41.3
New y center:
New magnitude:

/* draw it */
map mapofusa

/* reset the center */
CENTER mapofusa AT 51.3,42.1 WITH MAGNITUDE 78.0
.fi
.sh "SEE ALSO"
.nf
MAP(GEOQUEL)
SHADE(GEOQUEL)
OVERLAY(GEOQUEL)
