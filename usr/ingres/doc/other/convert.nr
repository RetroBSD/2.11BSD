.po 5
.de @h
'sp 4
'tl ''''
'sp 2
..
.de @f
'sp 2
'tl 'CONVERTING DATA BASES''Page %'
'bp
..
.wh 0 @h
.wh -7 @f
.de EX
.in 5
.nf
.ls 1
..
.de EE
.in
.fi
.ls
.sp
..
.de pp
.sp
..
.sp 4
.ce 2
CONVERTING VERSION 5 DATABASES
TO VERSION 6 DATABASES
.sp 4
.pp
If you are currently running version five
of INGRES
and you have databases
which you want to save,
you will have to convert those databases
to be compatible with version six.
.pp
The process of conversion
involves creating a new database,
copying the version 5 relations
into files,
and copying the files
into version 6 relations.
The copy process is automated.
Each user who owns relations
in the database
other than the system catalogs
must execute the copy steps
of this sequence.
.pp
Before executing the "How To Setup" instructions
you must move your copy of INGRES version five
to another place
with different names.
Log in as ingres,
mount the distribution tape,
change to the "source" directory of
the root of the INGRES directory strucure,
and read the shell file "make5.sh" into the setup directory:
.EX
chdir .../source
tp xm0 ../setup/make5.sh
.EE
You should then execute this shell file:
.EX
setup	make5
.EE
This shell file will move all version five object code
to new directories or change their names.
The object code
in .../bin
will be moved to .../bin5.
All the ingres code in /usr/bin
will have a "5" appended to the name.
(NOTE: You must have write permission
in /usr/bin to execute this step
of the shell file.
If you do not,
you will have to perform this operation
by hand
as the UNIX superuser.)
The file /lib/libq.a
will be moved to /lib/libq5.a;
this file may be removed
if you do not intend to use
version 5 of EQUEL in the future.
The .../source, .../lib, and .../doc directories
will be purged.
.pp
Other version 6 directories needed
will be created by this shell file.
You should proceed with reading in
your version 6 tape
from step 3 of the "How To Setup" instructions.
.pp
After you have read in INGRES version 6
you will be ready to start the conversion process.
You will have to create the version six
copy of the database.
Go in as the user
who is to be the Database Administrator
and create the database:
.EX
:login: dba
Password:
% creatdb v6database
.EE
Each user who owns relations
in this database
must execute all the following steps.
.pp
First,
create a directory
which will hold the temporaries
for the conversion process:
.EX
mkdir pathname
.EE
The run copydb5
which will create
a QUEL script
which may be run on version 5
to get the copy files:
.EX
copydb5 v5database pathname
ingres5 v5database < pathname/copy.out
.EE
(WARNING: The full path name of "pathname"
must be specified in the copydb5 command.)
Then copy the files into version six:
.EX
ingres v6database < pathname/copy.in
.EE
.pp
After this process is executed,
you will have a complete version six database.
When you are satisfied that it is correct,
you should destroy the version five database
and the temporary files:
.EX
destroydb5 v5database
rm -r pathname
rmdir pathname
.EE
.pp
Please note that system catalogs
and secondary indicies
are NOT copied,
but are recreated
by the copy.in script.
Also,
relations are modified
to be the same storage structure
that they were in version five,
however,
If it is customary to run purge
at your installation,
be certain to save your relations.
.pp
While the copy scripts are running
the generated commands
are echoed on the standard output.
There may be some error messages generated
when empty character fields are created;
your data will be entered despite the warnings.
If any other errors are reported,
attempt to determine the cause of the problem
before destroying the old database.
