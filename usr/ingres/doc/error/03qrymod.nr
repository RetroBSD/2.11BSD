.th QRYMOD ERROR 3/30/79
.sh NAME
Query Modification error message summary
.sh SYNOPSIS
Error numbers 3000 \- 3999.
.sh DESCRIPTION
These error messages are generated by the Query Modification
module.
These errors include syntactic and semantic problems
from view, integrity, and protection definition,
as well as run time errors \-
such as inability to update a view,
or a protection violation.
.sh ERRORS
.lp +8 8
3310	%0 on view %1: cannot update some domain
.s3
You tried to perform operation
%0 on a view;
however, that update is not defined.
.s3
.lp +8 8
3320	%0 on view %1: domain occurs in qualification of view
.s3
It is not possible to update a domain in the qualification of
a view, since this could cause the tuple to disappear
from the view.
.s3
.lp +8 8
3330	%o on view %1: update would result in more than one query
.s3
You tried to perform some update
on a view
which would update two underlying relations.
.s3
.lp +8 8
3340	%0 on view %1: views do not have TID's
.s3
You tried to use the Tuple IDentifier field
of a view, which is undefined.
.s3
.lp +8 8
3350	%0 on view %1: cannot update an aggregate value
.s3
You cannot update a value which is defined in the view
definition as an aggregate.
.s3
.lp +8 8
3360	%0 on view %1: that update might be non-functional
.s3
There is a chance that the resulting update would be non-functional,
that is, that it may have some unexpected side effects.
\*(II takes the attitude that it is better
to not try the update.
.s3
.lp +8 8
3490	INTEGRITY on %1: cannot handle aggregates yet
.s3
You cannot define integrity constraints
which include aggregates.
.s3
.lp +8 8
3491	INTEGRITY on %1: cannot handle multivariable constraints
.s3
You cannot define integrity constraints
on more than a single variable.
.s3
.lp +8 8
3492	INTEGRITY on %1: constraint does not initially hold
.s3
When you defined the constraint,
there were already tuples in the relation
which did not satisfy the constraint.
You must fix the relation so that the constraint holds
before you can declare the constraint.
.s3
.lp +8 8
3493	INTEGRITY on %1: is a view
.s3
You can not define integrity constraints
on views.
.s3
.lp +8 8
3494	INTEGRITY on %1: You must own \*a%1\*a
.s3
You must own the relation
when you declare integrity constraints.
.s3
.lp +8 8
3500	%0 on relation %1: protection violation
.s3
You have tried to perform an operation
which is not permitted to you.
.s3
.lp +8 8
3590	PERMIT: bad terminal identifier "%2"
.s3
In a
.it permit
statement,
the terminal identifier field was improper.
.s3
.lp +8 8
3591	PERMIT: bad user name "%2"
.s3
You have used a user name
which is not defined on the system.
.s3
.lp +8 8
3592	PERMIT: Relation \*a%1\*a not owned by you
.s3
You must own the relation before
issuing protection constraints.
.s3
.lp +8 8
3593	PERMIT: Relation \*a%1\*a must be a real relation (not a view)
.s3
You can not define permissions on views.
.s3
.lp +8 8
3594	PERMIT on %1: bad day-of-week \*a%2\*a
.s3
The day-of-week code was unrecognized.
.s3
.lp +8 8
3595	PERMIT on %1: only the DBA can use the PERMIT statement
.s3
Since only the DBA can have shared relations,
only the DBA can issue
.it permit
statements.
.s3
.lp +8 8
3700	Tree buffer overflow in query modification
.lp +8 8
3701	Tree build stack overflow in query modification
.s3
Bad news.
An internal buffer has overflowed.
Some expression is too large.
Try making your expressions smaller.
