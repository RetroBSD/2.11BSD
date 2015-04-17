.th INTRODUCTION GEOQUEL 3/16/79
This manual is a reference manual for the \*(GQ
system.  The introduction
describes some of the basic
pieces of information needed for operating
\*(GQ.
.sh "INPUT MONITOR"
Interacting with \*(GQ is similar to interacting
with the \*(II monitor.
A specific description is in the
MONITOR(GEOQUEL) manual section.

It is possible to run \*(GQ as a batch processing interface
using the `<', `>' and `|' operators of the \*(UU shell.
Of course, any interactions completed in the batch mode
must be saved for inspection.
This is accomplished by using the SAVEMAP command.
.sh "GENERAL SYNTAX"
The general syntax of \*(GQ for comments,
names, and constants is the same as for QUEL.
The keywords recognized by \*(GQ are as follows:
.nf
.in +8
center		overlay		is
deletemap	pointgraph	at
histogram	savemap		lowest
shade		magnitude	highest
linegraph	density		cellcount
map		bounds		cellwidth
on		with
.in -8
.fi
and these punctuation or operator symbols:
.br
.ti +8
,  (  )  .  "  /  /*  */  -  =
.br
It should also be noted that any time
the keyword `IS' is used, the
operator `=' can be substituted.
.sh "MAPRELATION RELATION"
The MAPRELATION relation contains one tuple describing
the viewing window for each map
in the database.
When the map is first drawn,
\*(GQ scales it to fit entirely
into the window area.
After this, the user is free to
redefine the window using the CENTER command.
It can be thought of as a user defined system relation,
which \*(GQ will create if it doesn't
exist in the database.
It is of the form:

.nf
.in +10
.ti -5
MAPRELATION (MRELID=c12, MRELOWNER=c2, MXCENTER=f4,
MYCENTER=f4, MXMAGNTD=f4, MYMAGNTD=f4)
.in
.fi

In this relation,
the domain MRELID contains the name of the map.

MRELOWNER is the usercode of the owner of this map.  More than one map
may exist with the same name in a data base as long as
the map owners are different.  A user may only access
maps which he owns or which the data base
administrator owns.

MXCENTER, MYCENTER are the x,y coordinates which
correspond to the point on the map which will be displayed
at the center of the screen.  (This does not
necessarily correspond to the actual physical
center of the map.  See the description of the CENTER
command.)

MXMAGNTD, MYMAGNTD are the x and y limits of the map
which are used in scaling to the screen size.  Together
with MXCENTER and MYCENTER, these domains define a
window for viewing a part of the map.
.sh MAPS
A map is a relation and as such
may be manipulated with a great deal of
flexibility by a user in \*(II.  The only restriction
is that the relation must have the nine special
domains described below to be handled by
the graphics commands in \*(GQ.  Thus, the user may use
\*(II freely to perform such operations as adding
data domains to a map, deleting data domains, overlaying
two or more maps, separating previously overlayed maps,
or obtaining subsets of maps.
A map is defined to be a
relation containing at least the following domains:  X1, Y1,
X2, Y2, PLZTYPE, ZONEID, GROUPID, GRAPHCHAR, and INTENSITY.  In
addition, other domains may be present representing data which
is related in some way to the map.  Thus, a map is a relation of the
form:

.nf
.in +10
.ti -5
RELNAME (X1, Y1, X2, Y2, PLZTYPE, ZONEID, GROUPID,
GRAPHCHAR, INTENSITY, ...other domains...)
.in
.fi

An explanation of each of these domains follows.  First,
a few definitions:

Points {(x,y)} and line segments {(x1,y1,x2,y2)} are the
obvious spatial constructs.
A polygon is a group of n line segments
such that a) each of the 2n points
belongs to exactly 2 line segments, b) no two line
segments cross,
and c) the polygon is fully closed.
Maps are then collections of points, lines, and
zones.
Within \*(GQ, the terminology of `zones' and
\&`polygons' are the same and are interchangeable.

In the above description, X1, Y1, X2, and Y2 are
coordinates of a point or line segment
and are numeric domains.
In the case of a point
X2 and Y2 are superfluous and unused.

PLZTYPE indicates whether the tuple represents
a point (PLZTYPE = p), a line segment which is not part of a
polygon
(PLZTYPE = l), or a line segment which is part of a
zone (PLZTYPE = z).  Points may be used to describe the locations
of separate entities such as schools or county seats.  Lines
may be used to define continuous entities such as
rivers or railways.  Zones may be used to define areas.  In
addition there is a fourth type, PLZTYPE = c, which indicates
that the tuple identifies a coordinate which is associated with a
zone and usually located in the vicinity of the zone's center.
This is used as the `center' of a zone for the "ON domain"
option of the MAP command,
and the OVERLAY command.
The PLZTYPE domain is of type character and
need be no longer than 1 byte.

ZONEID is a zone identifier.  It is used to logically connect
the separate lines segments which may comprise a single zone.
ZONEID is a character domain and may be 1-12
characters long.
Together with GROUPID, ZONEID forms a unique identifier for each zone.

GROUPID is a secondary grouping mechanism for zones.
It allows subdividing a large
area such as a state into smaller areas
like counties. GROUPID is a numeric domain.

GRAPHCHAR applies to tuples of type point only.  If
GRAPHCHAR is not zero the point will be displayed
represented by the character specified.  Otherwise it will
be displayed as a hardware point.
GRAPHCHAR is an i2 domain.

INTENSITY indicates the display screen intensity of a
tuple.  The value of INTENSITY may range in integer increments
from 0 (the tuple will not be displayed at all) to 7
(maximum brightness).
INTENSITY is also an i2 domain.

Other domains are user specified data domains which are
tied to the points, lines, or zones of the map.
For example, in a map in which points represent schools, an
\&`other domain' could contain the number of classrooms;  where
lines represent rivers, the user
may add domains showing average rate of flow by
season;  where zones represent states, other domains could
be specified for population, state flower, largest city,
or governor's name.

.sh "DISPLAY DEVICES"
\*(GQ currently supports
DEC's GT40 and GT42, and Tektronix's 4014.
When a user logs into \*(GQ, the terminal
type is looked up and the output of \*(GQ
is appropriate for the type of terminal.
If the terminal is unable to display graphical
output or \*(GQ is being run in background,
the user is informed at login time and is
reminded that interactions must be saved using the
SAVEMAP command.
Further discussion of display terminal support
may be found in the GEO-QUEL(UNIX) section
of the manual and in the \*(GQ setup
instructions.
