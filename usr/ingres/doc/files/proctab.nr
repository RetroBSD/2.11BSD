.th PROCTAB FILES 3/14/79
.sh NAME
\&.../files/proctab6.3 \- \*(II runtime configuration information
.sh DESCRIPTION
The file
.bd \&.../files/proctab6.3
describes the runtime configuration
of the \*(II system.
.s3
The process table
is broken up
into logical sections,
separated by lines
with a single dollar sign.
The first section
describes the configuration
of the system
and the parameters
to pass to each system module.
All other sections
contain strings
which may be macro-substituted
into the first section.
.s3
Each line of the first section
describes a single process.
The lines consist of a series
of colon-separated fields.
.s3
The first field
contains the pathname
of the module
to be executed.
.s3
The second field
is a set of flags
which allow the line
to be ignored
in certain cases.
If this field is null,
the line is accepted;
otherwise,
it should be a series of items
of the form
``+\-=\c
.it X\c
\&'',
where any of
``+\-=''
may be omitted,
and
.it X
is a flag
which may appear
on the \*(II command line.
The characters
``+\-=''
are interpreted as the sense of the flag:
``+'' will accept the line
if the flag
``+\c
.it X\c
\&'' is stated on the command line,
``\-''
will accept if
``\-\c
.it X
\&'' is stated,
and ``='' will accept
if the ``\c
.it X\c
\&'' flag is not stated at all.
These may be combined in the forms
``+='' and ``\-=''.
For example,
the field:
.s3
	+=&
.s3
will acept the line
if the EQUEL flag
(``&'')
is stated as
``+&'' or is not stated,
but the line will be ignored
if the ``\-&'' flag is stated.
.s3
If any flag item rejects the line,
the entire line is rejected.
.s3
The third field is a status word.
The number in this word
is expressed in octal.
The bits have the following meaning:
.nf
.s3
	000010	close diagnostic output
	000004	close standard input
	000002	run in user's directory, not database
	000001	run as the user, not as \*(II
.s3
.fi
The fourth field
is a file name
to which the standard output
should be redirected.
It is useful for debugging.
.s3
The fifth field
describes the pipes
which should be connected
to this process.
The field must be six characters long,
with the characters corresponding to the internal variables
R_up, W_up, R_down, W_down, R_front, and W_front
respectively.
The characters may be a question mark,
which leaves the pipe closed;
a digit,
which is filled in from the file descriptors
provided by the EQUEL flag or the ``@'' flag;
or a lower case letter,
which is connected via a pipe
with any other pipes
having the same letter;
this last action is done on the fly
to conserve file descriptors.
.s3
The sixth and subsequent fields
are arbitrary parameters
to be sent to the modules.
There must be a colon
after the fifth field
even if no parameters are present,
but there need not be a colon after the last parameter.
.s3
The last module executed
(usually the last line in the first section)
becomes the parent
of all the other processes.
.s3
The second through last sections
of the process table
consist of a single line
which names the section
followed by arbitrary text.
The pathname field
and all parameter fields
of each line
of the first section
are scanned for strings of the form ``$name'';
this string is replaced by the text
from the corresponding section.
For convenience,
the name
.bd $pathname
is automatically defined
to be the pathname
of the root
of the \*(II subtree.
.s3
The DBU routines
want to see two parameters.
The first parameter
is the pathname of the ``ksort'' routine.
The second parameter
is a series of lines
of the form:
.s3
	command_name:place_list
.s3
where command_name is the name of one of the possible
\*(II commands executed by the DBU routines,
and place_list is a comma-separated list
of the actual location(s) of that command.
Each ``place'' is a two-character
descriptor:
the first character is the overlay
in which that command resides,
and the second character
is the function
within that overlay.
If a command is in more than one place,
\*(II will try to avoid
calling in another overlay.
For example:
.s3
	create:a0,m1
.s3
means that the
.it create
command
may be found in overlay ``a''
function 0
or in overlay ``m'' function 1.
If already in overlay ``a'' or ``m''
the create command resident in that overlay
will be called;
otherwise,
overlay ``a'' will be called.
.sh "EXAMPLE"
The following example
will execute a three process system
unless the ``&'' flag is specified
(as ``\-&''),
when a two-process system will be executed
with the monitor dropped out
and the calling (EQUEL) program
in its place.
Notice that there are two lines
for the parser entry,
one for the EQUEL case
and one for the non-EQUEL case.
In the EQUEL case,
output from the parser is diverted
to a file called ``debug.out''.

.nf
.in +4
$pathname/bin/overlaya::000014::bc??23:$pathname/bin/ksort:$dbutab
$pathname/bin/parser:+=&:000014::adcb??:
$pathname/bin/parser:\-&:000014:debug.out:01cb??:
$pathname/bin/monitor:+=&:000003::??da??:$pathname/files/startup
$
dbutab
create:a0,m1
destroy:a1,m2
modify:m0
help:a2
$
.fi
.in -4
