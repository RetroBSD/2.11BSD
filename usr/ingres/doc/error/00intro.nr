.th INTRODUCTION ERROR 3/30/79
.sh NAME
Error messages introduction
.sh DESCRIPTION
This document describes the error returns which are possible from the
\*(II data base system and gives an explanation of the probable reason
for their occurrence.
In all cases the errors are
numbered
.it nxxx
where
.it n
indicates the source of
the error,
according to the following table:
.s3
.nf
	1 =  \s-2EQUEL\s0 preprocessor
	2 =  parser
	3 =  query modification
	4 =  decomposition and one variable query processor
	5 =  data base utilities
	30 = \s-2GEO-QUEL\s0 errors
.s3
.fi
For a description of these routines the reader is referred to
.it "The Design and Implementation of \*(II."
The
.it xxx
in an error number is an
arbitrary identifier.
.s3
The error messages
are stored in the file
.bd \&.../files/error6.3_\c
.it n,
where
.it n
is defined as above.
The format of these files
is the error number,
a tab character,
the message to be printed,
and the tilde character (``~'')
to delimit the message.
.s3
In addition many error messages have ``%\c
.it i\c
\&''
in their body
where
.it i
is a digit
interpreted as an offset
into a list of parameters
returned by the source of the error.
This indicates
that a parameter will be inserted by the error handler into the error
return.
In most cases this parameter will be self explanatory in meaning.
.s3
Where the error message is thought to be completely self explanatory, no
additional description is provided.
