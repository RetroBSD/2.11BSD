.th TIMEFIX UNIX 3/14/79
.sh NAME
timefix \- patch \*(II binary code for correct timezone.
.sh SYNOPSIS
.bd timefix
[
.bd \-u
] [
.bd \-s\c
.it N
] [
.bd \-t\c
.it yyyzzz
] [
.bd \-d\c
.it X
] filename ...
.sh DESCRIPTION
.it Timefix
can be used to change the \*(II system to use local time.
As delivered,
the \*(II system assumes it is in the Pacific Time Zone,
eight hours behind Greenwich Mean Time.
.it Timefix
can automatically patch the necessary \*(II
code to reflect the true timezone of your installation.
.s3
\*(II uses the \*(UU supplied routine ctime(III) for
converting the \*(UU base time to local time.
.it Timefix
is first compiled on your system in order to
get the
.it ctime
routine for your installation.
It then can correct the necessary \*(II programs
at which point your \*(II system will be correct and
not have to be changed again.
.s3
To use
.it timefix
you must be logged in as
.bd ingres
and
then use the
.it setup
command:
.s3
	setup timefix
.s3
This command performs the following steps:
(1) compiles timefix.c,
(2) runs timefix on the necessary \*(II programs.
.s3
If for any reason,
timefix.c cannot be compiled on your system,
another method exists for updating \*(II.
.it Timefix
can be run with the local time expressed
using the
.bd \-s
.bd \-t
and
.bd \-d
flag options.
Their meanings are:
.s3
.de xx
.lp +12 12
.if n \\$1\\$2\t\c
.if t \fB\\$1\fP\fI\\$2\fP\t\c
..
.xx \-s N
.it N
is the number of seconds ahead/behind GMT.
.xx \-t yyyzzz
.it yyy
and
.it zzz
are the two, three character timezone names.
.xx \-d X
if
.it X
is 0, then daylight savings time is disabled,
otherwise daylight savings time will occur according to the
rules specified in ctime(III).
.i0
.s3
The flags can be given in any order.
You must be logged in as
.bd ingres
and then use the
.it setup
command
giving the proper time conversions.
For example,
in Hartford, Connecticut you would type:
.s3
	setup timefix \-s18000 \-tESTEDT \-d1
.s3
If the
.bd \-u
flag is specified then
.it timefix
reports what it would do but does not actually
update the files.
.sh EXAMPLES
setup timefix
.br
setup timefix \-s18000 \-tESTEDT \-d1
.sh DIAGNOSTICS
.it Timefix
depends on the existence of a namelist.
If the namelist (i.e. symbol table) is not present then
an error message is given.
If all the symbols related to the
.it ctime
routine cannot be
found then it is assumed that the file need not be corrected.
If the file cannot be opened for reading and writing then
an error message is issued.
.sh BUGS
.it Timefix
will not work for timezones which are \*(+-10 or
\*(+-11 hours from GMT.
.sh "SEE ALSO"
ctime(III)
