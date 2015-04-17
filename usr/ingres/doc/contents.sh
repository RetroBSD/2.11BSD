#! /bin/csh

if (-r toc.nr) rm toc.nr
echo '.th "TABLE OF CONTENTS" INGRES 1995/04/27' > toc.nr
chdir quel
../toc.sh *.nr
chdir ../unix
../toc.sh *.nr
chdir ../files
../toc.sh *.nr
chdir ../error
../toc.sh *.nr
