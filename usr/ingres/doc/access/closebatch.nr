.th CLOSEBATCH AM 3/9/77
.sh NAME
closebatch - close the batch-update file
.sh SYNOPSIS
.nf
closebatch()
.fi
.sh DESCRIPTION
Closebatch is used to close the batch file for further
updates.
After closebatch is called, the DBU routine update should
be used to process the batch file.
The next call to
openbatch will destroy the batch file if it still exists.
.sh DIAGNOSTICS
.in +15
.ti -6
-1 -- batchfile was not open
.ti -5
0 -- successful
.in -15
.sh "SEE ALSO"
addbatch, openbatch, rmbatch
