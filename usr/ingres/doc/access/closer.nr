.th CLOSER AM 9/6/75
.sh NAME
closer - close a relation
.sh SYNOPSIS
.nf
closer(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
CLOSER is used to close a relation which was opened by OPENR.
If the number of tuples in the relation were changed by the program,
CLOSER will update the RELTUPS field in the relation relation.
If all relations are opened for reading only,
(i.e. the number of tuples could not be changed)
it may be desirable to use INPCLOSER instead of CLOSER.
CLOSER should always be called once for each OPENR.
.sh DIAGNOSTICS
.in +15
.ti -6
AMCLOSE_ERR -- cannot close the relation
.ti -6
AMWRITE_ERR -- pageflush failure
.ti -6
AMREAD_ERR -- get_page failure
.ti -5
0 -- success
.ti -5
1 -- relation was never open
.in -15
.sh "SEE ALSO"
inpcloser, noclose, openr
