.th FIND AM 9/6/77
.sh NAME
find - set start and end points for a relation scan
.sh SYNOPSIS
.nf
find(d, mode, lotid, hitid, key)
struct descriptor	*d;
int			mode;
struct tup_id		*lotid, *hitid;
char			key[MAXTUP];
.fi
.sh DESCRIPTION
FIND is used to determine start and end points for a scan
of a relation.
MODE must be equal to one of the constants
NOKEY, EXACTKEY, LRANGEKEY, HRANGEKEY or FULLKEY
which are defined in the "ingres.h" header file.
LOTID and HITID will be set to the lower and upper
page limits of the scan, respectively.
.sp
If the entire relation is to be scanned,
FIND should be called with MODE = NOKEY.
In this case KEY may be omitted and LOTID and HITID
are set to the absolute lower and upper limits
of the relation.
.sp
There are two ways to restrict the number of pages
to be scanned.
One can look for specific values in certain domains,
or for a range of values in the domains.
In either case,
the search values should be placed in KEY using the
CLEARKEYS and SETKEY functions.
If one is looking for specific values,
then FIND should be called with MODE = EXACTKEY.
LOTID and HITID will both be set for the proper
scan limits.
If a range of values is desired, then FIND must be called
twice.
The first time with MODE = LRANGEKEY and KEY containing
the low values for the range.
This will set LOTID to the proper limit.
The second time with MODE = HRANGEKEY and KEY containing
the high values for the range.
This will set HITID to the proper limit.
It is not necessary to provide values for the same domains
on both calls to FIND,
but care must be taken to use the CLEARKEYS and SETKEY functions properly.
.sp
FIND is called with MODE = FULLKEY by FINDBEST and REPLACE.
This is the same as the EXACTKEY mode except that SETKEY does not
have to be called and values must be provided for every
domain of the tuple.
.sp
FIND can only reduce the number of tuples that one must check
against a given qualification.
FIND cannot gaurantee that the scan is limited to only
those tuples that meet the restrictions given in KEY.
.sh DIAGNOSTICS
.in +15
.ti -6
AMFIND_ERR -- unidentifiable relation storage structure
.ti -6
AMWRITE_ERR -- pageflush failure
.ti -6
AMREAD_ERR -- get_page failure
.ti -5
0 -- success
.in -15
.sh "TRACE FLAGS"
92.0, 92.1
.sh "SEE ALSO"
clearkeys, findbest, fullkey, get, ndxsearch, replace, rhash, setkey
