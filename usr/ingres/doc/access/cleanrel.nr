.th CLEANREL AM 9/3/77
.sh NAME
cleanrel - flush and reset any buffers belonging to a relation
.sh SYNOPSIS
.nf
cleanrel(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
CLEANREL will search the access method buffers for a buffer
being used by the given relation.
If a buffer is found, it is flushed and cleared so that
the next time the relation is accessed,
a disk read will be forced.
This is useful if an open relation is being updated
by some other program and it is necessary to force
a disk read so that the most up-to-date data can be
retrieved from the relation.
.sh DIAGNOSTICS
.in +15
.ti -6
AMWRITE_ERR -- pageflush error
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
acc_close
