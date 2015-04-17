.th ICOMPARE AM 9/6/77
.sh NAME
icompare - compare two domains
.sh SYNOPSIS
.nf
icompare(a, b, frmt, frml)
char	*a, *b, frmt, frml
.fi
.sh DESCRIPTION
ICOMPARE will compare two domains or fields which are
pointed to by A and B.
FRMT specifies the type of data
being compared.
Its value must correspond to the constants
CHAR, INT or FLOAT as defined in the "symbol.h" header file.
FRML specifies the length of the fields being compared.
The fields may begin at any address,
i.e. they do not have to begin at a "0 mod 2" or "0 mod 4" address.
When fields of type CHAR are compared, all leading, trailing
and embedded blanks are ignored.
.sh DIAGNOSTICS
.in +15
.ti -7
< 0 -- A is less than B
.ti -5
0 -- A equals B
.ti -7
> 0 -- A is greater than B
.in -15
.sh "SEE ALSO"
kcompare, scompare
