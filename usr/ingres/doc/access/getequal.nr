.th GETEQUAL AM 9/12/77
.sh NAME
getequal - get the first tuple equal to the provided key
.sh SYNOPSIS
.nf
getequal(d, key, tuple, tid)
struct descriptor	*d;
char			key[MAXTUP];
char			tuple[MAXTUP];
struct tup_id		*tid;
.fi
.sh DESCRIPTION
GETEQUAL is used to do a keyed retrieval of a single
tuple in cases where the calling program knows the key to
be unique.  SETKEY must be called first to set all desired
domain values.  GETEQUAL will return the first tuple with
equality on all of the specified domains.
The tuple is returned in TUPLE.
.sh DIAGNOSTICS
.in +15
.ti -6
AMFIND_ERR -- unidentifiable storage structure in find
.ti -6
AMWRITE_ERR -- pageflush failure
.ti -6
AMREAD_ERR -- get_page failure or invalid error
.ti -5
0 -- success
.ti -5
1 -- no match
.in -15
.sh "TRACE FLAGS"
97.0, 97.1, 97.2
.sh "SEE ALSO"
clearkeys, closer, find, get, openr, setkey
