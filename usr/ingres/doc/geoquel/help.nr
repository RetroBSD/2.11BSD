.th HELP GEOQUEL 3/28/79
.sh NAME
help \- restricted interface to \*(II
.it Help
command
.sh SYNOPSIS
.bd help 
[\c
relname\c
] 
.br
.sh DESCRIPTION
.it Help 
may be used to obtain
information on the content of the current data base or
information about specific relations in the data base.
The legal forms are as follow:
.s3
.if n .in +5
.if t .in +.5i
.de xx
.if n .ti -5
.if t .ti -.5i
..
.xx
.bd help
\-  Gives information about all relations that exist in the
current database.
.xx
.bd help
.it relname
\- Gives information about the
specified relation.
.if n .in -5
.if t .in -.5i
.sh EXAMPLE
help
.br
help emp
.i0
