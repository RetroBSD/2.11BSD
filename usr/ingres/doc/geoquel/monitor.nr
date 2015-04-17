.th MONITOR GEOQUEL 3/16/79
.sh NAME
monitor \- interactive terminal monitor
.sh DESCRIPTION
This is the section of
\*(GQ that manages the query buffer.
As characters are typed on the terminal, they are entered into
a query buffer, where they can be edited at any time.
A "*" is printed at the start of each line.
This prompt is to remind you that
it is waiting
for you to continue.

A `\e' is the signal to the monitor that an action is to be taken
according to the character following `\e'.
Either the upper-case or
lower-case letter can be used for any monitor command.
.de xx
.lp +10 10
..
.s3
.xx
\er	Erase the entire query (reset the query buffer).
The former contents of the buffer are irretrievably lost.
.s3
.xx
\ep	Print the current query.
The contents of the buffer are printed on the user's terminal.
.s3
.xx
\ee	Enter the \*(UU text editor (see ED in the \*(UU Programmer's Manual);
use the ED command `w' followed by `q' to return to
\*(GQ.
.s3
.xx
\eg	Process the current query (go).
The display interpreter portion of \*(GQ is
called and your interactions are executed.
.s3
.xx
\ea	Append to the query buffer.
Typing \ea after completion of a query will override the
auto-clear feature and guarantees that the query buffer
will not be reset.
.s3
.xx
\et	Print out the current time of day.
.s3
.xx
\es	Escape to the \*(UU shell. Typing a control-d will cause
you to exit the shell and return to \*(GQ.
.s3
.xx
\ec	Clear the screen.
On terminals that can diplay graphic output,
the screen is completely cleared.
.s3
.xx
\eq	Exit from \*(GQ.
.s3
.xx
\e<any other character>
.br
Ignore any possible special meaning of character following `\e'.
This allows line editing characters and the `\e' itself
to be input as literal characters.
.i0

In a typical \*(GQ session a query is typed and when
it is complete the user types \eg.
After the query has been processed the monitor
will print:

.in +8
continue
*
.in

.br
If the next thing typed is a command
which effects the query buffer
(i.e. \ep, \ee, \eg, or \ea)
then the previous query is saved in the query buffer
and any new information will be appended to
the existing query.
If instead the next thing typed is a query,
then the query buffer is automatically reset and the
next line is accepted as a new query.

When calling the editor or the shell,
\*(GQ first tries to execute "/usr/bin/ingres_ed"
or "/usr/bin/ingres_sh" respectively.
If this fails,
"/bin/ed" or "/bin/sh" is called.
Thereby allowing the \*(II super-user
to change the default editor or
shell to something other than the standard
\*(UU facilities.

It is important that you do not use the "e" command inside
the editor; if you
do, the (obscure) name of the query buffer will be forgotten.
.sh "SEE ALSO"
.nf
quel(quel)
.fi
.sh DIAGNOSTICS
.de xx
.lp +20 20
..
.xx
go	You may begin a fresh query.
.s3
.xx
continue	The previous query is finished and you
are back in the monitor.
.s3
.xx
Executing . . .	The query is being processed by \*(GQ.
.s3
.xx
>>ed	You have entered the \*(UU text editor.
.s3
.xx
monitor<<	You have left the text editor,
or shell and have returned
to \*(GQ.
.s3
.xx
>>sh	You have escaped to the \*(UU shell.
.s3
.xx
Funny character nnn converted to blank
.br
\*(GQ maps non-printing ASCII characters into blanks; this message
indicates that one such conversion has just been made.
.i0
.sh BUGS
The rubout key
may do very strange things
while you are in the editor or the shell.
It should halt processing and return control
to the monitor section.

The \*(II monitor expanded command names should be accepted.

Some method for specifying the editor and shell to
use would be nice.
