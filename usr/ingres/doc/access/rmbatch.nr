.th RMBATCH AM 3/9/77
.sh NAME
rmbatch - close and remove an existing batch file
.sh SYNOPSIS
rmbatch()
.sh DESCRIPTION
Rmbatch can be called at any time to destroy an
already existing batch file.
It is usefull for removing the batch file in
case of an error during processing.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- successful
.in -15
.sh "SEE ALSO"
addbatch, closebatch, openbatch
