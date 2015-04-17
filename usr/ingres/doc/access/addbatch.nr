.th ADDBATCH AM 3/9/77
.sh NAME
addbatch - add a record to the update file
.sh SYNOPSIS
.nf
addbatch(oldtid, newtuple, oldtuple)
struct tupid	*oldtid;
char		newtuple[MAXTUP];
char		oldtuple[MAXTUP];
.fi
.sh DESCRIPTION
Addbatch is used to add tuples and their tids to the
batch update file.
Oldtid is the tid of oldtuple (used for mdDEL and mdREPL).
Newtuple is a pointer to the newtuple (used for
mdREPL and mdAPP).
Oldtuple is a pointer to the oldtuple (used for mdDEL and
mdREPL).
.sh DIAGNOSTICS
.in +15
.ti -6
-1 -- batchfile wasn't open
.ti -5
0 -- successful
.in -15
.sh "SEE ALSO"
closebatch, openbatch, rmbatch
