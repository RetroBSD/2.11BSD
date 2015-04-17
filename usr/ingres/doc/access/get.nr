.th GET AM 9/12/77
.sh NAME
get - get a single tuple
.sh SYNOPSIS
.nf
get(d, tid, limtid, tuple, getnxt)
struct descriptor	*d;
struct tup_id		*tid, *limtid;
char			tuple[MAXTUP];
int			getnxt;
.fi
.sh DESCRIPTION
GET provides a mechanism to scan through tuples in a relation.
Each call to GET returns one tuple into the caller's buffer (TUPLE).
If GETNXT is false (=0), then the tuple identified by TID is returned.
This mode is useful for retrieval by secondary indices and does not
require a call to FIND before the call to GET.

In most cases GETNXT will be true (non-zero) and a series of GET calls
will be made.
FIND must be called to initialize the
starting (TID) and ending (LIMTID) points of the scan.
GET will update the tuple id (TID) as tuples are returned and
other programs should not attempt to perform any arithmetic on tuple id's.
Tuple id's are unique identifiers of a tuple but the value of the
tuple id has no relation to either the tuple's physical order in
the relation or the position in a file.

Calls done via GETNXT will not accumulate pages
in the buffer pool.
When Get has to go to the next page,
the current page buffer will be reused.
.sh DIAGNOSTICS
.in +15
.ti -6
AMWRITE_ERR -- pageflush error
.ti -6
AMREAD_ERR -- get_page or invalid error
.ti -5
0 -- success
.ti -5
1 -- end of scan (no tuple returned)
.ti -5
2 -- tuple deleted (getnxt = false only)
.in -15
.sh "SEE ALSO"
closer, find, get_tuple, openr
