.th REPLACE AM 9/14/77
.sh NAME
replace - modify an existing tuple
.sh SYNOPSIS
.nf
replace(d, tid, tuple, checkdups)
struct descriptor	*d;
struct tup_id		*tid;
char			tuple[MAXTUP];
int			checkdups;
.fi
.sh DESCRIPTION
REPLACE will replace the current value of the tuple identified
by TID with the new value in TUPLE.
CHECKDUPS has the same meaning for REPLACE as it does for INSERT.
See INSERT for description.
.sp
REPLACE will normally put the new tuple in the same
location occupied by the old tuple except if the
relation is compressed or if the value of a keyed
domain is changed.
This is necessary in the former case because compression
may give a tuple size different from the previous tuple,
while in the latter case a new key value for the tuple
may force the tuple to be located in a different section
of the relation so that it can be found by FIND.
The two exceptions are handled by calling DELETE
to remove the old tuple and calling INSERT to
add the new tuple.
.sh DIAGNOSTICS
.in +15
.ti -6
AMFIND_ERR -- unidentifiable storage structure in find
.ti -6
AMWRITE_ERR -- write error
.ti -6
AMREAD_ERR -- read error
.ti -6
0 -- success
.ti -6
1 -- new tuple was a duplicate
Tid is set to the tuple id of the duplicate
.ti -6
2 -- tuple identified by tid has been deleted
.in -15
.sh "TRACE FLAGS"
95.0, 95.1, 95.2
.sh "SEE ALSO"
closer, delete, get, insert, openr
