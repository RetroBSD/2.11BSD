.th INSERT AM 9/13/77
.sh NAME
insert - add a new tuple to a relation
.sh SYNOPSIS
.nf
insert(d, tid, tuple, checkdups)
struct descriptor	*d;
struct tup_id		*tid;
char			tuple[MAXTUP];
int			checkdups;
.fi
.sh DESCRIPTION
INSERT will insert the offered tuple into the "correct"
position in the relation.  TID will be set to the TID which is
assigned to the tuple.
Normally INSERT should be called with CHECKDUPS = 1 which will
cause INSERT to check for a duplicate tuple prior to making the
insertion.  In certain special cases when dealing with INGRES
system relations (i.e. RELATION, ATTRIBUTE, etc.),
the calling program may know that a check for duplicates is not 
necessary and may call INSERT with CHECKDUPS = 0.
If the relation is stored as a heap (D->RELSPEC = 5),
checkdups will be ignored
and assumed to be false.
.sh DIAGNOSTICS
.in +15
.ti -6
AMFIND_ERR -- unidentifiable storage structure
.ti -6
AMWRITE_ERR -- write error
.ti -6
AMREAD_ERR -- read error
.ti -6
0 -- success
.ti -5
1 -- a duplicate of TUPLE was found
Tid will be set to the tuple id of the duplicate.
.in -15
.sh "TRACE FLAGS"
94.0
.sh "SEE ALSO"
closer, findbest, openr, put_tuple
