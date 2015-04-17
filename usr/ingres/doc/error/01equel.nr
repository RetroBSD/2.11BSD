.th EQUEL ERROR 3/30/79
.sh NAME
\s-2EQUEL\s0 error message summary
.sh SYNOPSIS
Error numbers 1000 \- 1999.
.sh DESCRIPTION
The following errors can be generated at run time
by \s-2EQUEL\s0 programs.
.sh ERRORS
.lp +8 8
1000	In domain %0 numeric retrieved into char field.
.s3
Equel does not support conversion
at run-time
of numeric data from the data base
to character string representation.
Hence, if you attempt to assign
a domain of numeric type to a C-variable of type character string, you
will get this error message.
To convert numerics to characters
use the "ascii" function
in \s-2QUEL\s0.
.s3
.lp +8 8
1001	Numeric overflow during retrieve on domain %0.
.s3
You will get this error if you attempt to assign a numeric data base domain
to a C-variable of a numeric type but with a shorter length.
In this
case the conversion routines may generate an overflow.
For example, this
error will result from an attempt to retrieve
a large floating point number
into a C-variable of type integer.
.s3
.lp +8 8
1002	In domain %0, character retrieved into numeric variable.
.s3
This error is the converse of error 1000.
.s3
.lp +8 8
1003	Bad type in target list of parameterized retrieve "%0".
.s3
Valid types are %f8, %f4, %i4, %i2, %c.
.s3
.lp +8 8
1004	Bad type in target list of parameterized statement "%0".
.s3
Valid types are %f8, %f4, %i4, %i2, %i1, %c.
