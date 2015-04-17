.de am
.ne 8
.sp 4
\\$1 (\\$2)
..
.th INTERNAL AM 1/19/79
.sh NAME
internal routine used by the access methods.
.sh SYNOPSIS
The functions listed in this section should not be
called directly by any higher level program.
These functions are all used to implement the
access method calls described elsewhere in this manual.
.am ACC_CLOSE 9/3/77
.sh NAME
acc_close - close relations opened by readadmin
.sh SYNOPSIS
acc_close()
.sh DESCRIPTION
ACC_CLOSE closes the relation and attribute relations
which were opened by READADMIN using the
Admin_rel and Admin_att descriptor structs.
ACC_CLOSE calls CLEANREL to flush and reset any
buffers associated with these two relations.
.sh DIAGNOSTICS
If an error occurs in CLEANREL
a SYSERR is generated.
.sh "SEE ALSO"
readadmin
.am ACC_ERR 9/13/77
.sh NAME
acc_err - set global Accerror indicator
.sh SYNOPSIS
.nf
acc_err(errnum)
int	errnum;
.fi
.sh DESCRIPTION
ACC_ERR will set the global Accerror indicator to the value
of ERRNUM and then return that value.
The Accerror indicator is printed whenever a SYSERR occurs
and its value is non-zero.
.sh "TRACE FLAGS"
85.0
.sh "SEE ALSO"
closer, get_attuples, get_page, openr, pageflush
.am ACC_INIT 8/31/77
.sh NAME
acc_init - initialize access methods
.sh SYNOPSIS
.nf
acc_init()
.fi
.sh DESCRIPTION
ACC_INIT initializes the access methods by setting up
all of the access method buffers and all the pointers
that link them together.
The buffers are organized so that only one buffer can be used
by any one relation.
This helps so that
"there is a little bit of each relation" available since
many programs go back and forth between two or more relations.
The buffers are allocated in a least recenlty used fashion.
ACC_INIT calls READADMIN to initialize the Admin,
Admin_rel and Admin_att structures and to
open the relation and attribute relations.
ACC_INIT also opens the lock device
used for concurrency control.
.sh DIAGNOSTICS
No value is returned.
If an error occurs a SYSERR is generated.
.sh "TRACE FLAGS"
81.0, 81.1
.sh "SEE ALSO"
resetacc, readadmin
.am ADD_A_PAGE 9/3/77
.sh NAME
add_a_page - add a page to a relation
.sh SYNOPSIS
.nf
add_a_page(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
ADD_A_PAGE adds a page to the given relation.
It is assumed that the current access method buffer
contains a page from the relation.
No linking is done to this page,
i.e. no other page is affected by adding a page
to a relation using this function.
The new page is not written but is marked for writing.
TID will contain the page id of the new page.
This function is used by MODIFY.
.sh DIAGNOSTICS
.in +15
.ti -6
-1 -- pageflush failure on previous page in buffer
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
add_ovflo, add_prim
.am ADD_OVFLO 9/6/77
.sh NAME
add_ovflo - add overflow page to a relation
.sh SYNOPSIS
.nf
add_ovflo(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
ADD_OVFLO will add an overflow page to the page
in the current access method buffer.
The page must be part of the relation described in D.
TID will be updated to the id of the new overflow page.
.sh DIAGNOSTICS
.in +15
.ti -10
-5, -3 -- get_page failure
.ti -14
-4, -2, -1 -- pageflush failure
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
add_a_page, add_prim, findbest
.am ADD_PRIM 9/3/77
.sh NAME
add_prim - add a primary page to a relation
.sh SYNOPSIS
.nf
add_prim(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
ADD_PRIM adds a primary page to the given relation.
It is assumed that the current access method buffer
contains the last primary page of the relation.
The new page is marked for writing.
TID will contain the page id of the new page.
ADD_PRIM is called by MODIFY.
.sh DIAGNOSTICS
.in +15
.ti -10
-1, -2 -- pageflush failure
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
add_a_page, add_ovflo
.am ADDABYTE 9/7/77
.sh NAME
addabyte - add byte to hash key
.sh SYNOPSIS
.nf
addabyte(ch, word, knt)
char	ch;
long	*word;
int	knt;
.fi
.sh DESCRIPTION
ADDABYTE takes a byte at a time from CH which is rotated and exclusive-ored
into WORD, a long integer.
This function is used by RHASH to form the hash key.
KNT determines how CH is rotated.
.sh DIAGNOSTICS
No value is returned.
.sh "SEE ALSO"
rhash
.am CANONICAL 9/13/77
.sh NAME
canonical - make tuple canonical
.sh SYNOPSIS
.nf
canonical (d, tuple)
struct descriptor	*d;
char			tuple[MAXTUP];
.fi
.sh DESCRIPTION
CANONICAL will make TUPLE canonical and return the new length
of the tuple.
If the relation specified by D is compressed,
TUPLE will be compressed and stored in the global Accanon.
If the relation is not compressed,
all character domains of TUPLE will be blank padded.
In either case the global pointer Acctuple
will point to the canonical tuple.
.sh DIAGNOSTICS
No error value is returned.
.sh "SEE ALSO"
comp_tup, insert, put_tuple, replace
.am CHOOSE_BUF 9/3/77
.sh NAME
choose_buf - select an access method buffer
.sh SYNOPSIS
.nf
struct accbuf	*choose_buf()
.fi
.sh DESCRIPTION
CHOOSE_BUF will scan the access method buffers for a
free buffer and return a pointer to the buffer.
If there is no free buffer
the last buffer in the LRU list is flushed and reset
and a pointer to it is returned.
.sh DIAGNOSTICS
.in +15
.ti -7
-1 -- pageflush or reset failure
.in -15
.sh "TRACE FLAGS"
82.8
.sh "SEE ALSO"
formatpg, get_page, resetacc
.am COMP_TUP 9/13/77
.sh NAME
comp_tup - compress a tuple
.sh SYNOPSIS
.nf
comp_tup(d, tuple)
struct descriptor	*d;
char			tuple[MAXTUP];
.fi
.sh DESCRIPTION
COMP_TUP will compress TUPLE into the global area Accanon.
The compression consists of replacing trailing blanks in character fields
with null string terminators.
No compression is done on numeric fields.
This compression method will never result in a tuple
becoming longer than the normal tuple.
COMP_TUP will return the length of the compressed tuple.
.sh DIAGNOSTICS
No error value is returned.
.sh "SEE ALSO"
canonical, put_tuple, uncomp_tup
.am DEL_TUPLE 9/6/77
.sh NAME
del_tuple - delete specified tuple from current page
.sh SYNOPSIS
.nf
del_tuple(tid, width)
struct tup_id	*tid;
int		width;
.fi
.sh DESCRIPTION
DEL_TUPLE deletes the tuple specified by TID from the page
in the current access method buffer.
WIDTH must equal the true width of the tuple in bytes
after any compression is performed.
No checking is done to guarantee that the page id in TID
matches the page in the access method buffer.
.sh DIAGNOSTICS
No value is returned.
.sh "SEE ALSO"
delete, replace
.am DUMPTID 9/14/77
.sh NAME
dumptid - print tuple id
.sh SYNOPSIS
.nf
dumptid(tid)
struct tup_id	*tid;
.fi
.sh DESCRIPTION
DUMPTID will print the page and line ids of TID
on the UNIX standard output.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
.am DUP_CHECK 9/14/77
.sh NAME
dup_check - check page for duplicate tuple
.sh SYNOPSIS
.nf
dup_check(d, tuple)
struct descriptor	*d;
char			tuple[MAXTUP];
.fi
.sh DESCRIPTION
DUP_CHECK will scan the page in the current access method buffer
for a duplicate of TUPLE.
No checking is done to verify that the page in the current access
method buffer is part of the relation described by D.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- no duplicate found
.ti -5
1 -- duplicate found
.in -15
.sh "SEE ALSO"
findbest, scan_dups
.am FIND_BUF 9/3/77
.sh NAME
find_buf - find a buffer with a page from the given relation
.sh SYNOPSIS
.nf
struct accbuf		*find_buf(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
FIND_BUF will scan all the access method buffers until it finds
a buffer with a relation id that matches the relation id
in the descriptor struct,
and return a pointer to the buffer.
If no buffer is found a zero is returned.
If the access method buffers have not been initialized
by the time FIND_BUF is called,
ACC_INIT is called to initialize them.
.sh DIAGNOSTICS
If an error occurs in ACC_INIT a SYSERR is generated.
.sh "TRACE FLAGS"
82.0
.sh "SEE ALSO"
acc_init, cleanrel, closer, find, findbest, inpcloser, noclose, get_page
.am FINDBEST 9/8/77
.sh NAME
findbest - determine best page to insert a tuple
.sh SYNOPSIS
.nf
findbest(d, tid, tuple, need, checkdups)
struct descriptor	*d;
struct tup_id		*tid;
char			tuple[MAXTUP];
int			need, checkdups;
.fi
.sh DESCRIPTION
FINDBEST will find the best page in the relation described
by D for TUPLE.
NEED indicates how much space is needed to store the tuple.
If the relation is stored as a heap,
FINDBEST will try to fit the tuple on the current page
in the access method buffers.
If there is no space,
or if the current page is not available,
the last page of the relation is selected.
For other storage structures,
FINDBEST will try to fit the tuple on the correct primary page
or on one of its overflow pages that is closest to the primary page.
If there is no free space a new overflow page is created.
If CHECKDUPS is true,
the primary page and all its overflow pages will be scanned
for a duplicate of the tuple.
If a duplicate is found
then the new tuple will not be inserted,
otherwise FINDBEST allocates an entry in the line number table
of the target page for the tuple being inserted.
TID will be set to the location of the new tuple.
.sh DIAGNOSTICS
.in +15
.ti -6
-8 -- unidentifiable storage structure in find
.ti -14
-5, -3, -1 -- get_page failure
.ti -14
-4, -2, -1 -- pageflush failure
.ti -5
1 -- duplicate found
.in -15
.sh "TRACE FLAGS"
88.2
.sh "SEE ALSO"
dup_check, find, insert, replace, scan_dups
.am FULLKEY 9/6/77
.sh NAME
fullkey - determine if SETKEY has been called
for every keyed domain of a relation.
.sh SYNOPSIS
.nf
fullkey(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
FULLKEY will determine if SETKEY was called
for every keyed domain of a given relation.
This is important for the hashing function
since it can only accept keys where a value has
been provided for each keyed domain.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- only a partial key is provided
.ti -5
1 -- a full key is provided
.in -15
.sh "SEE ALSO"
find, rhash
.am GET_ADDR 9/12/77
.sh NAME
get_addr - get address of tuple within page
.sh SYNOPSIS
.nf
char *get_addr(tid)
struct tup_id	*tid;
.fi
.sh DESCRIPTION
GET_ADDR computes and returns the character address of the tuple specified
by TID in the current access method buffer.
.sh DIAGNOSTICS
No error codes are returned.
If an error is detected
a SYSERR is generated.
.sh "SEE ALSO"
get_tuple, put_tuple
.am GET_ATTUPLES 9/13/77
.sh NAME
get_attuples - get tuples from attribute relation
.sh SYNOPSIS
.nf
get_attuples(d)
struct descriptor	*d;
.fi
.sh DESCRIPTION
GET_ATTUPLES will fill the attribute template of the descriptor
struct D with data from tuples of the attribute relation.
The relation must be identified in D,
which can be done by calling GET_RELTUPLE.
.sh DIAGNOSTICS
.in +15
.ti -6
-8 -- unidentifiable storage structure in find
.ti -6
-6 -- missing attributes from attribute relation
.ti -6
-2 -- pagelfush failure
.ti -6
-1 -- get_page failure or invalid error
.ti -5
0 -- success
.in -15
.sh "TRACE FLAGS"
90.3
.sh "SEE ALSO"
get_reltuple, openr
.am GET_PAGE 4/5/77
.sh NAME
get_page - get a page from a relation
.sh SYNOPSIS
.nf
get_page(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
GET_PAGE will retrieve a page from the UNIX file
which corresponds to a given relation,
unless the page is already available in one of
the access method buffers.
If the page has to be read in,
it will be placed in a buffer corresponding to the
relation specified by D.
If there is no such buffer,
the last buffer in the LRU buffer queue is flushed
and the page is placed in that buffer.
The buffer containing the requested page is then placed at the
top of the LRU buffer queue.
If the relation was opened in read/write mode,
a lock is set on the page to maintain proper concurrency control.
The current page size is 512 bytes of which
498 bytes are available for tuple storage.
.sh DIAGNOSTICS
.in +15
.ti -6
-2 -- pageflush error
.ti -6
-1 -- UNIX read error
.ti -5
0 -- success
.in -15
.sh "TRACE FLAGS"
83.0, 83.1, 83.2
.sh "SEE ALSO"
choose_buf, find_buf, get, resetacc, pageflush, top_acc
.am GET_RELTUPLE 9/13/77
.sh NAME
get_reltuple - get tuple from relation relation
.sh SYNOPSIS
.nf
get_reltuple(d, name)
struct descriptor	*d;
char			name[MAXNAME + 1];
.fi
.sh DESCRIPTION
GET_RELTUPLE will get a tuple from the relation relation
for the relation specified by NAME
and place the tuple in the descriptor struct pointed to by D.
Two search attempts are made,
the first for a relation owned by the current user,
the second for a relation owned by the
data base administrator.
.sh DIAGNOSTICS
.in +15
.ti -6
-8 -- unidentifiable storage structure in find
.ti -6
-2 -- pageflush failure
.ti -6
-1 -- get_page failure or invalid error
.ti -5
0 -- success
.ti -5
1 -- no tuples could be found
.in -15
.sh "TRACE FLAGS"
90.1
.sh "SEE ALSO"
openr, readadmin
.am GET_TUPLE 9/12/77
.sh NAME
get_tuple - get tuple from page
.sh SYNOPSIS
.nf
get_tuple(d, tid, tuple)
struct descriptor	*d;
struct tup_id		*tid;
char			tuple[MAXTUP];
.fi
.sh DESCRIPTION
GET_TUPLE will move a tuple from the page in the current
access method buffer to TUPLE,
performing any necessary decompression.
The tuple is identified by TID,
no checking is done to verify that the current page in the
access method buffer corresponds to TID or the relation
specified by D.
.sh DIAGNOSTICS
No value is returned.
.sh "SEE ALSO"
get, uncomp_tup
.am INVALID 9/12/77
.sh NAME
invalid - check the validity of a tuple id
.sh SYNOPSIS
.nf
invalid(tid)
struct tup_id	*tid;
.fi
.sh DESCRIPTION
INVALID checks if the tuple id represents a valid tuple
versus one that has been deleted.
INVALID assumes that the tuple id is for the
page in the current buffer.
.sh DIAGNOSTICS
.in +15
.ti -6
-1 -- line id exceeded line number table
.ti -5
0 -- tuple id is valid
.ti -5
2 -- tuple was deleted
.in -15
.sh "SEE ALSO"
delete, get, replace
.am LAST_PAGE 9/13/77
.sh NAME
last_page - compute tid for last page in relation
.sh SYNOPSIS
.nf
last_page(d, tid)
struct descriptor	*d;
struct tup_id		*tid;
.fi
.sh DESCRIPTION
LAST_PAGE computes a tuple id for the last page in the relation specified
by D and stores the id in TID.
The line id in TID is set to zero.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
If an error occurs a SYSERR is generated.
.sh "SEE ALSO"
add_ovflo, findbest, fstat (UNIX function)
.am NDXSEARCH 9/7/77
.sh NAME
ndxsearch - search ISAM directory
.sh SYNOPSIS
.nf
ndxsearch(d, tid, key, mode, keyok)
struct descriptor	*d;
struct tup_id		*tid;
char			key[MAXTUP];
int			mode, keyok;
.fi
.sh DESCRIPTION
NDXSEARCH will search the ISAM directory for the given relation.
It is assumed that the relation is stored as an ISAM file.
It is not necessary to provide values for all keyed domains,
but the only values used are those starting with the most
significant keyed domain
to the first keyed domain that does not have a value
provided by SETKEY.
If MODE is less than zero,
NDXSEARCH will determine the lower limit of a scan
according to the key.
The limit is returned in TID.
If MODE is greater than zero,
the upper scan limit is determined.
If KEYOK is true (not equal to zero),
it is assumed that values for all keyed domains
have been provided regardless of whether SETKEY was called.
.sh DIAGNOSTICS
.in +15
.ti -6
-2 -- pageflush failure
.ti -6
-1 -- get_page failure
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
find, setkey
.am NEWLINO 9/13/77
.sh NAME
newlino - find free line number on page
.sh SYNOPSIS
.nf
newlino(len)
int	len;
.fi
.sh DESCRIPTION
NEWLINO will scan the line number table in the page in the
current access method buffer,
looking for a free table entry.
If an entry is found it is allocated for a new tuple,
otherwise a new entry is created at the end of the
line number table and allocated for a new tuple.
Space in the page is also reserved for a tuple
of length LEN.
NEWLINO returns the line number allocated for a new tuple.
.sh DIAGNOSTICS
No error value is returned.
.sh "SEE ALSO"
findbest
.am PAGEFLUSH 9/13/77
.sh NAME
pageflush - flush access method buffers
.sh SYNOPSIS
.nf
pageflush(buf)
struct accbuf	*buf;
.fi
.sh DESCRIPTION
PAGEFLUSH will write the access method buffer pointed to
by BUF onto disk if the buffer was tagged for writing.
PAGEFLUSH can be called with the parameter "Acc_head"
which is a global buffer pointer (declared in access.h)
which will flush the current access method buffer.
PAGEFLUSH can also be called with a 0 (i.e. a null pointer)
which will flush all buffers
tagged for writing.
If a page was locked,
it is unlocked after being written to disk.
.sh DIAGNOSTICS
.in +15
.ti -6
-2 -- UNIX write error
.ti -5
0 -- success
.in -15
.sh "TRACE FLAGS"
84.0, 84.1, 84.2
.sh "SEE ALSO"
get_page, resetacc
.am PLUCK_PAGE 9/14/77
.sh NAME
pluck_page - extract page id from tuple id
.sh SYNOPSIS
.nf
pluck_page(tid, pnum)
struct tup_id	*tid;
long		*pnum;
.fi
.sh DESCRIPTION
PLUCK_PAGE will extract the page id from TID and
store it in the long variable pointed to by PNUM.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
stuff_page, tup_id
.am PUT_TUPLE 9/13/77
.sh NAME
put_tuple - put tuple in page
.sh SYNOPSIS
.nf
put_tuple(tid, tuple, length)
struct tup_id	*tid;
char		tuple[MAXTUP];
int		length;
.fi
.sh DESCRIPTION
PUT_TUPLE will move TUPLE into the page in the current access method buffer
at the location specified by TID.
It is assumed that any required compression has already been done
and that the correct amount of space has been reserved on the page
for the tuple.
No checking is done to verify that the page identified by TID
corresponds to the page in the current access method buffer.
.sh DIAGNOSTICS
No value is returned.
.sh "TRACE FLAGS"
87.0
.sh "SEE ALSO"
canonical, get_addr, insert, replace
.am READADMIN 9/3/77
.sh NAME
readadmin - read data from admin file
.sh SYNOPSIS
.nf
readadmin()
.fi
.sh DESCRIPTION
READADMIN will read data from the admin file.
The admin file contains two descriptor structs,
one for the relation relation and one for the
attribute relation.
These two descriptors are used by the access methods
whenever either of the relations has to be used
to retrieve or update information.
The two descriptors are globally declared as
Admin_rel and Admin_att.
The admin file also contains the user id for the data base administrator,
which is stored in the Admin_admin struct.
.sh DIAGNOSTICS
Nothing is returned.
If an error occurs a SYSERR is generated.
.sh "SEE ALSO"
acc_init
.am RESETACC 8/31/77
.sh NAME
resetacc - reset access method buffer
.sh SYNOPSIS
.nf
resetacc(buf)
struct accbuf	*buf;
.fi
.sh DESCRIPTION
RESETACC will first flush the given buffer
if it contains a dirty page from some relation,
then clear the relation identifier and other fields in the buffer.
This has
the effect of erasing the buffer so that any attempt to access
the page that was contained in the buffer will fail unless
the page is read back from disk.
If the buf parameter equals zero,
the current access method buffer is reset.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.ti -7
< 0 -- pageflush fail (see pageflush for error codes)
.in -15
.sh "TRACE FLAGS"
80.0
.sh "SEE ALSO"
acc_init, choose_buf, cleanrel, pageflush
.am RHASH 9/7/77
.sh NAME
rhash - compute hash key
.sh SYNOPSIS
.nf
long			rhash(d, key)
struct descriptor	*d;
char			key[MAXTUP];
.fi
.sh DESCRIPTION
RHASH will perform a randomizing hash on the full key.
Values for the hash function are taken from every keyed domain of KEY.
It is important that every keyed domain have a correct value,
otherwise the hash result will be incorrect.
RHASH returns a page number in the form of a long integer.
.sh DIAGNOSTICS
None.
.sh "TRACE FLAGS"
92.8
.sh "SEE ALSO"
addabyte, find
.am SCAN_DUPS 9/14/77
.sh NAME
scan_dups - scan for duplicate tuples
.sh SYNOPSIS
.nf
scan_dups(d, tid, tuple)
struct descriptor	*d;
struct tup_id		*tid;
char			tuple[MAXTUP];
.fi
.sh DESCRIPTION
SCAN_DUPS will look for a duplicate of TUPLE
beginning with the page specified by TID.
SCAN_DUPS will also check all overflow pages linked
to the first page until a duplicate is found.
SCAN_DUPS will make sure that the page
that was in the current access method buffer upon entry
will be there upon exit,
as long as that page was part of the relation
described by D.
.sh DIAGNOSTICS
.in +15
.ti -6
-2 -- pageflush failure
.ti -6
-1 -- get_page failure
.ti -5
0 -- no duplicate found
.ti -5
1 -- duplicate found
.in -15
.sh "SEE ALSO"
dup_check, findbest, replace
.am SPACE_LEFT 9/13/77
.sh NAME
space_left - compute amount of free space on page
.sh DESCRIPTION
SPACE_LEFT will determine how much space is left on
the current page for tuple storage.
The return value is the amount of free space in bytes.
.sh DIAGNOSTICS
none
.sh "SEE ALSO"
findbest
.am STUFF_PAGE 9/14/77
.sh NAME
stuff_page - set page id of tuple id
.sh SYNOPSIS
.nf
stuff_page(tid, pnum)
struct tup_id	*tid;
long		*pnum;
.fi
.sh DESCRIPTION
STUFF_PAGE will take the page number pointed to by PNUM
and store it in TID without affecting the
line id in TID.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
pluck_page, tup_id
.am TOP_ACC 9/3/77
.sh NAME
top_acc - move buffer to top of LRU list
.sh SYNOPSIS
.nf
top_acc(buf)
struct accbuf	*buf
.fi
.sh DESCRIPTION
TOP_ACC unlinks the buffer from its current position
in the LRU linked list and relinks it at the top of the list.
TOP_ACC is called for a buffer every time it is accessed
to maintain the LRU sequence of the linked list.
.sh DIAGNOSTICS
.in +15
.ti -5
0 -- success
.in -15
.sh "SEE ALSO"
acc_init
.am TUP_ID 9/14/77
.sh NAME
tup_id - tuple id structure
.sh SYNOPSIS
.nf
struct tup_id
{
	char	pg1, line_id;
	int	pg2;
}	tid;
.fi
.sh DESCRIPTION
The TUP_ID structure is used to declare tuple id's
which are passed as parameters to some of the
access method routines.
The tuple id uniquely addresses a tuple of a relation.
Since tuples can move around within a relation,
tuple id's should NOT be saved.
It may be desirable to access the tuple id in some
special cases to gather statistics on disk usage.
At no time should a program modify a tuple id,
the access methods take care of initializing
and updating tuple id's as necessary.
.sh "SEE ALSO"
delete, find, get, insert, pluck_page, replace, stuff_page
.am TUP_LEN 9/14/77
.sh NAME
tup_len - determine length of tuple
.sh SYNOPSIS
.nf
tup_len(tid)
struct tup_id	*tid;
.fi
.sh DESCRIPTION
TUP_LEN will determine how many bytes in the page in the current access method
buffer are used by the tuple
pointed to by TID,
and return the length.
No checking is done to verify that the page specified by TID
corresponds to the page in the current access method buffer.
.sh DIAGNOSTICS
No error value is returned.
.sh "TRACE FLAGS"
89.1
.sh "SEE ALSO"
delete, replace
.am UNCOMP_TUP 9/12/77
.sh NAME
uncomp_tup - decompress tuple
.sh SYNOPSIS
.nf
uncomp_tup(d, cp, tuple)
struct descriptor	*d;
char			*cp, tuple[MAXTUP];
.fi
.sh DESCRIPTION
UNCOMP_TUP will decompress the tuple pointed to by CP
and store the result in TUPLE.
.sh DIAGNOSTICS
No value is returned.
.sh "SEE ALSO"
comp_tup, get_tuple
