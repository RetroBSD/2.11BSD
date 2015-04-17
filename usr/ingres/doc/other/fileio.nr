.th FILEIO UTIL 1/1/78
.sh NAME
fileio -- file I/O library
.sh SYNOPSIS
.nf
# include	".../source/fileio.h"

FILE *fopen(file, mode, buffer)
char	*file;
char	*mode;
char	buffer[IOBUFSIZ];

fclose(iop)
FILE	*iop;

FILE *freopen(file, mode, iop)
char	*file;
char	*mode;
FILE	*iop;

setbuf(iop, buffer, bufsize)
FILE	*iop;
char	buffer[];
int	bufsize;

int getc(iop)
FILE	*iop;

putc(c, iop)
char	c;
FILE	*iop;

rewind(iop)
FILE	*iop;

fflush(iop)
FILE	*iop;

fread(iop, area, count)
FILE	*iop;
char	area[];
int	count;

fwrite(iop, area, count)
FILE	*iop;
char	*area;
int	count;

getchar()

putchar(c)
char	c;

getpw(uid, buf)
int	uid;
char	buf[];
.fi
.sh DESCRIPTION
These routines provide a consistant
interface
to UNIX files
using buffered I/O.
They are in many ways similiar
to the UNIX buffering routines;
however,
there are many important differences.

The role of a "file descriptor" in UNIX
is replaced by a "file pointer"
returned by fopen and freopen.
Every open file
has a file pointer associated with it.
In addition,
each open file
may optionally
have a buffer associated with it.
If it does,
the file is buffered;
otherwise,
it is unbuffered.

There are several defined constants
which are worth noting:

.in +10
.ta 11
.ti -10
IOBUFSIZ	The size of a buffer.
.ti -10
FILE	The base type for a file pointer.
.ti -10
EOF	The value returned by
getc and putc to indicate end-of-file
or error.
.ti -10
NULL	The value returned by pointer-valued functions
such as fopen and freopen
to indicate an error.
Also,
the value to be passed to fopen and setbuf
to indicate unbuffered I/O.
.in -10

The functions operate as follows:
.in +10
.ti -10
fopen(file, mode, buffer)
opens the UNIX file named
\fIfile\fP
for I/O of mode
\fImode\fP
which may be
"read" for read,
"write" for write,
or "append" for append.
Only the first character of the mode string
is examined
(so "r" is as good as "read").
When a file is created,
it is created with the mode
specified in the external variable "_Filemode",
which defaults to 0600.
The buffer must be an array declared
"char x[IOBUFSIZ]", or must be "NULL"
to indicate no buffer.
.ti -10
freopen(file, mode, iop)
closes file
\fIiop\fP
and then re-opens it just like fopen.
It keeps the same buffer it had before.
This is normally used on the standard input
(called "stdin") or the standard output (called "stdout").
It returns NULL if the open failed;
otherwise, it returns
\fIiop\fP.
.ti -10
fclose(iop)
closes the named file.
If it is an output file,
it is flushed.
.ti -10
setbuf(iop, buffer, bufsize)
sets the buffer for file
\fIiop\fP
to be
\fIbuffer\fP.
This is most commonly used on
standard input or output.
Additionally,
the buffer size is set to be
\fIbufsize\fP.
.ti -10
fflush(iop)
flushes file
\fIiop\fP.
If
\fIiop\fP
is an output file,
any data in the buffer
is output to the file;
if it is an input file,
the remainder of the buffer (if any)
is discarded.
It returns EOF on error,
zero on success.
.ti -10
rewind(iop)
resets the file iop to the load point.
.ti -10
fread(iop, area, count)
.ti -10
fwrite(iop, area, count)
These two calls
operate like read and write,
except that they are buffered.
.ti -10
getchar()
returns the next character
from the standard input.
It returns zero on end-of-file or error.
Note that zero is a legal character.
.ti -10
putchar(c)
outputs
\fIc\fP
to the standard output.
Control characters are printed as backslash escapes;
use putc for outputing cursor control
and such things.
.ti -10
getpw(uid, buf)
acts just like getpw
in the system library,
except that it uses
these buffering routines.
Also,
if
\fIbuf\fP
is
.B NULL
the password file is closed.
This is important for overlays.
.in -10
.sh "SEE ALSO"
getpw(III),
getc(III),
putc(III)
