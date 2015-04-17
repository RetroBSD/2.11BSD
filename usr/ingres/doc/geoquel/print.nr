.th PRINT GEOQUEL 3/28/79
.sh NAME
print \- print relation
.sh SYNOPSIS
.bd print 
relname
.sh DESCRIPTION
.it Print 
displays the contents of the relation specified
on the
terminal (standard output).
The formats for various types of domains can be
defined by the use of switches when 
.it ingres
is invoked.
Domain names are truncated to fit into the specified
width.
.s3
To print a relation one must either be the owner of the relation,
or the relation must have "retrieve to all" or "all to all" permissions.
.s3
See ingres(quel) for details.
.sh EXAMPLE
.nf
/* Print the emp relation */
   print emp
.fi
.sh "SEE ALSO"
permit(quel), retrieve(quel), 
ingres(unix), printr(unix)
.sh BUGS
.it Print
should
handle long lines of output correctly \- no wrap around.
.s3
.it Print
should have more formating features to make printouts more readable.
.s3
.it Print
should have an option to print on the line printer.
