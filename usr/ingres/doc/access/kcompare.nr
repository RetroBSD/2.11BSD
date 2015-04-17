.th KCOMPARE AM 9/6/77
.sh NAME
kcompare - compare a tuple and a key
.sh SYNOPSIS
.nf
kcompare(d, tuple, key)
struct descriptor	*d;
char			tuple[MAXTUP], key[MAXTUP];
.fi
.sh DESCRIPTION
KCOMPARE will compare each domain in key
which was set by SETKEY, with the corresponding domain in tuple.
Domains are compared in ascending domain number sequence
and NOT according to key number sequence.
.sh DIAGNOSTICS
.in +15
.ti -7
< 0 -- tuple is less than key
.ti -5
0 -- tuple equals key
.ti -7
> 0 -- tuple is greater than key
.in -15
.sh "SEE ALSO"
clearkeys, find, getequal, icompare, setkey
