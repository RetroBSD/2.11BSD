.th DELETE AM 9/6/77
.sh NAME
delete - delete a tuple
.sh SYNOPSIS
.nf
delete(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
DELETE will delete the tuple indicated by TID
from the relation specified by D.
.sh DIAGNOSTICS
.in +15
.ti -6
AMINVL_ERR -- invalid tid specified.
.ti -6
AMREAD_ERR -- get_page or invalid function error
.ti -5
0 -- success
.ti -5
2 -- tuple already deleted
.in -15
.sh "TRACE FLAGS"
91.0
.sh "SEE ALSO"
del_tuple, get, insert, openr
