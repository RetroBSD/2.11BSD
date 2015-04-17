.th DELETEMAP GEOQUEL 3/16/79
.sh NAME
deletemap \- remove a map from the data base
.sh SYNOPSIS
.nf
DELETEMAP relname
.fi
.sh DESCRIPTION
This command deletes the specified map
from the database.
The relation `relname' is
destroyed and then the tuple is removed from the MAPRELATION relation.
.sh "SEE ALSO"
DESTROY(QUEL)
.sh BUGS
If the map is removed by the user using the DESTROY command
within \*(II, then the tuple is not removed
by \*(GQ.
