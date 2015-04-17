.th SETKEY AM 9/14/77
.sh NAME
setkey - provide key values to be used by find
.sh SYNOPSIS
.nf
setkey(d, key, value, domnum)
struct descriptor	*d;
char			key[MAXTUP], *value;
int			domnum;
.fi
.sh DESCRIPTION
SETKEY is used to provide values for keyed domains.
Keys may be
as large as the largest tuple so the key character array must be
declared as the same size as a tuple buffer (i.e. length is MAXTUP).
SETKEY will move the supplied value into the proper position
in the key and will mark the descriptor to indicate that the value
has been supplied.
DOMNUM specifies which domain in the key is being assigned a value.
VALUE is a pointer to the value which is to be set into the key.
If a value is being given for a character domain,
the domain area in the key will be blank padded
after the value is moved in.
This is done to eliminate any nulls in character domains.
If VALUE is 0, (i.e. a null pointer)
then the tag in the descriptor indicating that a value was
provided for a domain is reset.
.sp
CLEARKEYS should be called prior to any SETKEY calls.
SETKEY must be called at least once before a call to FIND when 
find mode is LRANGEKEY, HRANGEKEY or EXACTKEY.
If find mode is NOKEY then neither
CLEARKEYS nor SETKEY need be called.
.sh DIAGNOSTICS
No value is returned.
If an error is detected, a SYSERR is generated.
.sh "TRACE FLAGS"
96.0
.sh "SEE ALSO"
clearkeys, find, getequal
