.th PARSER ERROR 3/30/79
.sh NAME
Parser error message summary
.sh SYNOPSIS
Error numbers 2000 \- 2999.
.sh DESCRIPTION
The following errors can be generated
by the parser.
The parser reads your query
and translates it into the appropriate internal form;
thus,
almost
all of these errors indicate syntax or type conflict
problems.
.sh ERRORS
.lp +8 8
2100	line %0, Attribute \*a%1\*a not in relation \*a%2\*a
.s3
This indicates that in a given line of the executed workspace the
indicated attribute name is not a domain in the indicated relation.
.s3
.lp +8 8
2103	line %0, Function type does not match type of attribute \*a%1\*a
.s3
This error will be returned if a function expecting numeric data
is given a character string or vice versa.
For example, it is
illegal to take the SIN of a character domain.
.s3
.lp +8 8
2106	line %0, Data base utility command buffer overflow
.s3
This error will result if a utility command is too long for the
buffer space allocated to it in the parser.
You must shorten the
command or recompile the parser.
.s3
.lp +8 8
2107	line %0, You are not allowed to update this relation: %1
.s3
This error will be returned if you attempt to update any system relation
or secondary index
directly in \s-2QUEL\s0 (such as the RELATION relation).
Such operations which
compromise the integrity of the data base are not allowed.
.s3
.lp +8 8
2108	line %0, Invalid result relation for APPEND \*a%1\*a
.s3
This error message will occur if you execute an append command to
a relation that does not exist, or that you cannot access.  For
example, append to junk( ... ) will fail if junk does not exist.
.s3
.lp +8 8
2109	line %0, Variable \*a%1\*a not declared in RANGE statement
.s3
Here, a symbol was used in a \s-2QUEL\s0 expression in a place where a
tuple variable was expected and this symbol was not defined
via a RANGE statement.
.s3
.lp +8 8
2111	line %0, Too many attributes in key for INDEX
.s3
A secondary index may have no more than 6 keys.
.s3
.lp +8 8
2117	line %0, Invalid relation name \*a%1\*a in RANGE statement
.s3
You are declaring a tuple variable which ranges over a relation which
does not exist.
.s3
.lp +8 8
2118	line %0, Out of space in query tree - Query too long
.s3
You have the misfortune of creating a query which is too long for the
parser to digest.
The only options are to shorten the query
or recompile the parser to have more buffer space for the query tree.
.s3
.lp +8 8
2119	line %0, MOD operator not defined for floating point
or character attributes
.s3
The
.it mod
operator is only defined
for integers.
.s3
.lp +8 8
2120	line %0, no pattern match operators allowed in the target list
.s3
Pattern match operators
(such as ``*'')
can only be used in a qualification.
.s3
.lp +8 8
2121	line %0, Only character type domains are allowed in CONCAT operator
.s3
.lp +8 8
2123	line %0, \*a%1.all\*a not defined for replace
.s3
.lp +8 8
2125	line %0, Cannot use aggregates ("avg" or "avgu") on character values
.lp +8 8
2126	line %0, Cannot use aggregates ("sum" or "sumu") on character values
.lp +8 8
2127	line %0, Cannot use numerical functions (ATAN, COS, GAMMA, LOG, SIN,
SQRT, EXP, ABS) on character values
.lp +8 8
2128	line %0, Cannot use unary operators ("+" or "\*-") on character values
.lp +8 8
2129	line %0, Numeric operations (+ \*- * /) not allowed on character values
.s3
Many functions and operators
are meaningless when applied to character values.
.s3
.lp +8 8
2130	line %0, Too many result domains in target list
.s3
Maximum number of result domains is MAXDOM (currently 49).
.s3
.lp +8 8
2132	line %0, Too many aggregates in this query
.s3
Maximum number of aggregates allowed in a query is MAXAGG (currently 49).
.s3
.lp +8 8
2133	line %0, Type conflict on relational operator
.s3
It is not legal
to compare a character type
to a numeric type.
.s3
.lp +8 8
2134	line %0, \*a%1\*a is not a constant operator.
.br
Only \*adba\*a or \*ausercode\*a are allowed.
.s3
.lp +8 8
2135	line %0, You cannot duplicate the name of an existing relation(%1)
.s3
You have tried to create a relation
which would redefine an existing relation.
Choose another name.
.s3
.lp +8 8
2136	line %0, There is no such hour as %1, use a 24 hour clock system
.lp +8 8
2137	line %0, There is no such minute as %1, use a 24 hour clock system
.lp +8 8
2138	line %0, There is no such time as 24:%1, use a 24 hour clock system
.s3
Errors 2136-38 indicate that you have used a bad time
in a
.it permit
statement.
Legal times are from 0:00 to 24:00 inclusive.
.s3
.lp +8 8
2139	line %0, Your database does not support query modification
.s3
You have tried to issue a query modification statement
(\c
.it define\c
),
but the database was created with the
.bd \-q
flag.
To use the facilities made available by query modification,
you must say:
.s3
	creatdb \-e +q
.it dbname
.s3
to the shell.
.s3
.lp +8 8
2500	syntax error on line %0
.br
last symbol read was: %1
.s3
A 2500 error is reported by the parser if it cannot otherwise classify the
error.
One common way to obtain this error is to omit the required
parentheses around the target list.
The parser reports the
last symbol which was obtained from the scanner.
Sometimes, the last
symbol is far ahead of the actual place where the error occurred.
The string "EOF" is used for the last symbol when
the parser has read past the query.
.s3
.lp +8 8
2502	line %0, The word \*a%1\*a, cannot follow a RETRIEVE command,
therefore the command was not executed.
.lp +8 8
2503	line %0, The word \*a%1\*a, cannot follow an APPEND command,
therefore the command was not executed.
.lp +8 8
2504	line %0, The word \*a%1\*a, cannot follow a REPLACE command,
therefore the command was not executed.
.lp +8 8
2505	line %0, The word \*a%1\*a, cannot follow a DELETE command,
therefore the command was not executed.
.lp +8 8
2506	line %0, The word \*a%1\*a, cannot follow a DESTROY command,
therefore the command was not executed.
.lp +8 8
2507	line %0, The word \*a%1\*a, cannot follow a HELP command,
therefore the command was not executed.
.lp +8 8
2508	line %0, The word \*a%1\*a, cannot follow a MODIFY command,
therefore the command was not executed.
.lp +8 8
2509	line %0, The word \*a%1\*a, cannot follow a PRINT command,
therefore the command was not executed.
.lp +8 8
2510	line %0, The word \*a%1\*a, cannot follow a RETRIEVE UNIQUE command,
therefore the command was not executed.
.lp +8 8
2511	line %0, The word \*a%1\*a, cannot follow a DEFINE VIEW command,
therefore the command was not executed.
.lp +8 8
2512	line %0, The word \*a%1\*a, cannot follow a HELP VIEW, HELP INTEGRITY,
or HELP PERMIT command, therefore the command was not executed.
.lp +8 8
2513	line %0, The word \*a%1\*a, cannot follow a DEFINE PERMIT command,
therefore the command was not executed.
.lp +8 8
2514	line %0, The word \*a%1\*a, cannot follow a DEFINE INTEGRITY command,
therefore the command was not executed.
.lp +8 8
2515	line %0, The word \*a%1\*a, cannot follow a DESTROY INTEGRITY or
DESTROY PERMIT command, therefore the command was not executed.
.s3
Errors 2502 through 2515 indicate that after an otherwise valid
query, there was something which could not begin another
command.
The query was therefore aborted,
since this could have been caused by misspelling
.bd where
or something equally as dangerous.
.s3
.lp +8 8
2700	line %0, non-terminated string
.s3
You have omitted the required string terminator (").
.s3
.lp +8 8
2701	line %0, string too long
.s3
Somehow, you have had the persistence or misfortune to enter a
character string constant longer than 255 characters.
.s3
.lp +8 8
2702	line %0, invalid operator
.s3
You have entered a character
which is not alphanumeric,
but which is not a defined operator,
for example, "?".
.s3
.lp +8 8
2703	line %0, Name too long \*a%1\*a
.s3
In \*(II relation names and domain names are limited to 12 characters.
.s3
.lp +8 8
2704	line %0, Out of space in symbol table - Query too long
.s3
Your query is too big
to process.
Try breaking it up
with more
.bd \ego
commands.
.s3
.lp +8 8
2705	line %0, non-terminated comment
.s3
You have left off the comment terminator symbol
(``\*(**/'').
.s3
.lp +8 8
2707	line %0, bad floating constant: %1
.s3
Either your floating constant was incorrectly
specified or it was too large or too small.
Currently, overflow and underflow are not
checked.
.s3
.lp +8 8
2708	line %0, control character passed in pre-converted string
.s3
In \s-2EQUEL\s0 a control character became embedded in
a string and was not caught until the scanner was
processing it.
.s3
.lp +8 8
2709	line %0, buffer overflow in converting a number
.s3
Numbers cannot exceed 256 characters in length.
This shouldn't become a problem until
number formats in \*(II are
increased greatly.
