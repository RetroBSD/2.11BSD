.th INPCLOSER AM 3/29/77
.sh NAME
inpcloser - close a relation that was opened as read only
.sh SYNOPSIS
.nf
inpcloser(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
INPCLOSER will close a relation that was opened by OPENR
with mode 0 (read only).
If a program never calls OPENR to open a relation with
mode 2 (read/write) then INPCLOSER should be used
to close the relation instead of CLOSER.
INPCLOSER uses less time
and memory than CLOSER.
.sh DIAGNOSTICS
.in +15
.ti -6
AMCLOSE_ERR -- cannot close the unix file
.ti -5
0 -- success
.ti -5
1 -- relation was not open
.ti -5
2 -- relation was opened mode 2 (read/write), CLOSER
must be used to close the relation
.in -15
.sh "SEE ALSO"
closer, noclose, openr
