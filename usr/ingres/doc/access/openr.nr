.th OPENR AM 9/13/77
.sh NAME
openr - open a relation
.sh SYNOPSIS
.nf
openr(d, mode, uniqid)
struct descriptor	*d;
int			mode;
char			uniqid[MAXNAME];
.fi
.sh DESCRIPTION
OPENR is used to open a relation and set up a descriptor
for the relation.
The descriptor is used by all of the
access method functions and contains a template of the relation.
The parameter UNIQID is the unique relation id which is to
be opened.
OPENR will search the relation relation and find the tuple which describes
the desired relation.  
OPENR will first search for a relation named by UNIQID and
owned by the current user.
If tha fails,
OPENR will search for a relation with the same name
but owned by the data base administrator.
The calling program may determine who the relation owner is by
examining D->RELOWNER in the returned descriptor.
OPENR will also search the attribute
relation and gather information necessary to create a template
of the relation.
Finally, OPENR will do the UNIX open on the
file which contains the relation.
.sp
If MODE equals 0, the relation will be opened as a read only file,
if MODE equals 1 or 2, the relation will be opened as a read/write file.
If MODE is specified as -1 then the relation will be looked
up in the relation relation and that part
of the descriptor will be filled in.
However, the relation will not
be opened and the attribute template will not be set in the
descriptor.
OPENR may be called with MODE set to -2,
which will fill in the attribute template and open the relation as read only.
Calling OPENR twice, first with MODE set to -1, the second time
with MODE set to -2,
is equivalent to calling OPENR once with MODE set to 0.
Using mode -3 is the same as mode
-2 except that the relation is open for writing.
Modes -4 and -5 both assume the relation has been
opened previously and the descriptor is complete.
Mode -4 will reopen the unix file for reading,
and mode -5 will reopen the unix file for writing.
.sh DIAGNOSTICS
.in +15
.ti -6
AMOPNVIEW_ERR -- attempt to open a view for reading or writing.
.ti -6
AMFIND_ERR -- unidentifiable storage structure for relation or attribute
relations
.ti -6
AMNOATTS_ERR -- error in filling attribute template
.ti -6
AMNOFILE_ERR -- no UNIX file corresponding to relation
.ti -6
AMWRITE_ERR -- pageflush failure
.ti -6
AMREAD_ERR -- get_page failure
.ti -5
0 -- success
.ti -5
1 -- no such relation
.in -15
.sh "TRACE FLAGS"
90, 90.4
.sh "SEE ALSO"
closer, get_attuples, get_reltuple, inpcloser, noclose
