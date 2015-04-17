.th OPENBATCH AM 3/8/77
.sh NAME
openbatch - create and open the batch-update file
.sh SYNOPSIS
.nf
openbatch(rel_desc, index_desc, mode)
struct descriptor	*rel_desc;
struct descriptor	*index_desc;
int			mode;
.fi
.sh DESCRIPTION
Openbatch is used to initially create and open the
batch update file.
The relation to be updated should be opened and rel_desc
should point to the open descriptor struct.
If the relation has a secondary index, then the
"indexes" relation should be opened and index_desc
should point to the "indexes" descriptor struct.
Mode is the type of update.
It should be either mdDEL, mdREPL, or mdAPP
(see ../symbol.h).
.sh DIAGNOSTICS
.in +15
.ti -6
-1 -- the previous Batch file is still open,
i.e. a closebatch or rmbatch has not been done
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
addbatch, closebatch, rmbatch
