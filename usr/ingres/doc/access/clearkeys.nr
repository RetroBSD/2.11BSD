.th CLEARKEYS AM 4/27/75
.sh NAME
clearkeys - reset key indicators in descriptor
.sh SYNOPSIS
.nf
clearkeys(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
CLEARKEYS is used to clear the indicators of a descriptor
before setting up key values with one or more calls to SETKEY.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
find, getequal, setkey
