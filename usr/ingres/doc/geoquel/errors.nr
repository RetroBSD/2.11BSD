.th ERRORS GEOQUEL 12/20/77
.sh NAME
error messages for \*(GQ display system
.sh DESCRIPTION
\*(GQ reports errors in a manner
identical to \*(II.
The error messages are stored in the file
\&.../files/error6.2_30 which is of the same
format as the \*(II error files
(please see the \*(II reference manual).
.sp 2
.ce
GEO-QUEL ERROR MESSAGES
.sp 1
.in +8
.sp 1
.ti -8
30210	GEO-QUEL is creating a MAPRELATION relation~
.sp 1
This is an informational message to notify
the user of what \*(GQ is doing.
This should only happen the first time
you log into a database with \*(GQ.
.sp 1
.ti -8
30300	line %0, In attempting to do a MAP, SHADE, or OVERLAY the relation
\&`%1' does not exist or is not owned by you~
.sp 1
.ti -8
30301	line %0, Failed to obtain information to put a tuple in MAPRELATION
relation~
.sp 1
This is probably due to an attempt to draw
a map without the nine requisite domains.
.sp 1
.ti -8
30310	line %0, Failed to complete MAP~
.sp 1
This is always preceded by an error message
from \*(II and is only intended to locate
the effect of the error.
.sp 1
.ti -8
30330	line %0, Failed while building temporary relation in SHADE command~
.sp 1
This is to explain a message from \*(II.
The message should tell what went wrong and
this is to help pinpoint the problem.
.sp 1
.ti -8
30331	line %0, Maximum polygon value is 0.0 which causes floating point
exception~
.sp 1
The hardware floating point unit on a PDP-11 causes an
interrupt on divide by zero your data values
for shading must not range `-n' - 0.
.sp 1
.ti -8
30332	line %0, Too many sides (maxside) in polygon (zoneid: %1, groupid: %2)~
.sp 1
\&`Maxside' is the manifest constant restricting the number
of sides in a polygon.
.sp 1
.ti -8
30340	line %0, Failed while trying to obtain range limits for
axes in POINTGRAPH or LINEGRAPH~
.sp 1
Again, this message is to explain one from \*(II.
.sp 1
.ti -8
30341	line %0, The linetype specified (%1) in the linegraph command
does not make sense for the device~
.sp 1
Please consult the \*(GQ reference manual for allowed line types.
.sp 1
.ti -8
30342	line %0, %1 does not exist or cannot be accessed by you~
.sp 1
The relation named in the command does not exist in the database,
or you do not have permission to see the contents.
.sp 1
.ti -8
30350	line %0, There is no current center definition for this map: %1~
.sp 1
CENTER cannot be used to specify the center
definitions for a map that
has no entry in the MAPRELATION relation.
.sp 1
.ti -8
30400	line %0, Too many domains for SHADE(shadedoms)~
.sp 1
You have specified too many domain-shadeval
pairs.
The name in parentheses is the manifest
constant which must
be changed to extend this limit.
.sp 1
.ti -8
30401	line %0, Too many domains for POINTGRAPH or LINEGRAPH (graphdoms)~
.sp 1
There were too many domain-character pairs specified.
The name in parentheses is the manifest constant
which sets the limit.
.sp 1
.ti -8
30410	line %0, Error in creating destination file `%1' in SAVEMAP~
.sp 1
.ti -8
30402	line %0, Invalid shade value: %1~
.sp 1
Only single character shade values or
the five line types are allowed.
.sp 1
.ti -8
30420	line %0, You may specify only one boundary when using
\&`cellcount' and `cellwidth'~
.sp 1
If more than one boundary is specified, there may be a conflict
in the number of cells and the width of cells.
.sp 1
.ti -8
40421	line %0, Only one cellwidth per command has meaning~
.ti -8
40422	line %0, Only one cellcount per command has meaning~
.ti -8
40423	line %0, Only one upper bound per command has meaning~
.ti -8
40424	line %0, Only one lower bound per command has meaning~
.sp 1
You have specified one of the optional clauses more than once.
.sp 1
.ti -8
30425	line %0, %1 does not exist or cannot be accessed by you~
.sp 1
The relation named in the command does not exist in the database or
you do not have permission to see the data.
.in -8
