.TH CXREF 1 "Georgia Tech"
.SH NAME
cxref \- cross reference C source files
.SH SYNOPSIS
.B cxref
[
.B \-FSCcfis
] [
.B \-w
.IR width " ]"
[files]
.SH DESCRIPTION
.PP
.I Cxref
reads the named C source files and produces on the standard output
a cross reference of all the identifiers and constants in the files.
Constants are integer constants (12, 0421, 0x1A),
floating point constants (123.45, 0.2e-4),
string constants ("this is a string\en"),
and character constants ('a', '\e033').
Identifiers, character constants, and string constants
are sorted lexicographically, i.e. according to the machine collating
sequence (7-bit ASCII on the Vax or the Pyramid).
Integer and floating point constants are sorted numerically.
The trailing 'l' or 'L' on long integer constants will not show
up in the output listing.
.PP
If no files are named,
.I cxref
reads the standard input. For multiple files, the argument "\-"
(a single dash) indicates that the standard input should be read
at that point.
.PP
If arguments are given, they must come before any file names.
.PP
.I Cxref
recognizes the following arguments:
.RS
.TP
.B \-F
Fold case in comparison.  By default, case is distinct in comparison of
identifiers and string and character constants.
.RI ( cxref
simply passes the "\-F" option on to
.IR sort (1)
as "\-f".)
.TP
.B \-S
Cross reference all files separately.
The default action is to cross reference all named files together.
.TP
.B \-c
Leave character constants out of the cross reference listing.
.TP
.B \-f
Leave floating point constants out of the cross reference listing.
.TP
.B \-i
Leave integer constants out of the cross reference listing.
.TP
.B \-s
Leave string constants out of the cross reference listing.
.TP
.B \-C
Leave
.I all
constants, character, string, integer, and floating point, out of
the cross reference listing.
By default, all types of constants are included in the cross reference.
.TP
.BI "\-w " width
Make the output be
.I width
columns wide.
The output width will never be less than 51 or more than 132 columns.
.I Cxref
silently adjusts incorrect settings to the nearest allowable setting.
If no width is specified, the output will default to 80 columns wide.
.RE
.PP
.IR Cxref " does " not
include #include files, or expand macro definitions.  Files named
in #include lines can be listed on the command line if they should
also be cross referenced.
.PP
If a quoted string has an escaped newline in it (see ``The C Programming
Language'', page 181, or Section 2.5 of the C Reference Manual),
it will show up inside the string in the output listing as \eN.
This is to make it visible to the programmer, and to keep the
various filters which
.I Cxref
uses to actually do the work from getting terribly confused.
.PP
.I Cxref
is best run in the background, with its output redirected into
a file or the line printer spooler
.IR lpr (1),
since it reads all
the named files, using
.IR sort (1)
as an intermediate pass.
The sorting can take time which the user can probably put to more productive
use.
.SH DIAGNOSTICS
.PP
Self explanatory.
.SH BUGS
.PP
Systems running UNIX 4.0 and later already have a program named
.IR cxref .
Therefore, on those systems, this program should be renamed.
.PP
.I Cxref
does not do any formatting on its output (other than to
insure that it writes the proper number of columns),
so it should probably be run piping its output into
.IR pr (1).
.PP
Floating point constants are converted to a common format for sorting,
therefore they may appear in the output in a format different from
(but numerically equivalent to) their form in the original source code.
.SH "SEE ALSO"
.IR lex (1),
.IR lpr (1),
.IR pr (1),
.IR sort (1)
.SH FILES
.TP
/tmp/cxr.$$.*
temporary files for integer and floating point contstants.
.I Cxref
removes these files when it is through.
.SH AUTHOR
.PP
.nf
Arnold Robbins
School of Information and Computer Science
Georgia Institute of Technology
Atlanta, Geogia  30332

UUCP:	gatech!arnold
CSNET:	arnold@gatech
ARPANET:	arnold%gatech.csnet@csnet-relay.arpa
.fi

Copyright (c) 1984 by Arnold Robbins.
All rights reserved.
This program may not be sold, but may be distributed
provided this notice is included.
