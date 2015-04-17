.th SAVEMAP GEOQUEL 12/15/77
.sh NAME
savemap \- copy display list into a file
.sh SYNOPSIS
.nf
SAVEMAP "filename"
.fi
.sh DESCRIPTION
This command copies the display list of the last command executed into
a file for re-displaying at a later time.
To redisplay the saved display list,
use the following command:
.ti +10
cat filename

It is useful for displaying material produced on a non-display terminal
or to save interactions for examination at a later date without
redrawing them.
