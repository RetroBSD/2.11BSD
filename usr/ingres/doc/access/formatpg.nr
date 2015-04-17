.th FORMATPG AM 9/12/77
.sh NAME
formatpg - build a skeleton relation of n pages
.sh SYNOPSIS
.nf
formatpg(d, n)
struct	descriptor	*d;
long			n;
.fi
.sh DESCRIPTION
FORMATPG will build a skeleton relation
described by D with N primary pages.
The calling program is responsible for creating
the relation.
The calling program need only supply
D->RELFP.
.sh DIAGNOSTICS
.in +15
.ti -10
-3, -4 -- write error on relation being built
.ti -6
-2 -- seek error on the relation being built
.ti -5
0 -- success
.in -15
