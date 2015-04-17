#! /bin/csh -f
# generic notes update script. looks in appropriate subdirectory and
# sends updates to the specified sites.
# the subdirectories are in /usr/spool/notes/.utilities
# the files are named grade.site -- grade is a single character
# and is used to force specific ordering in the wildcard expansion
#
# Ray Essick, May 1986.
#
# This script can be invoked by cron directly. It helps to be "notes".
# if you have a cron that does setuid/setgid for you, try something like:
#	(System V cron)
# 0 * * * * /usr/spool/notes/.utilities/cron.csh hourly > /usr/adm/notes/hourly
# 0 0 * * * /usr/spool/notes/.utilities/cron.csh daily > /usr/adm/notes/daily
#	(4.2 cron
# 0 * * * * su notes < /.../hourly.csh > /usr/adm/notes/hourly
# where "hourly.csh" contains:
#	exec /usr/spool/notes/.utilities/cron.csh hourly
#
#
#	check arguments, set up variables
#
if ( $#argv != 1 ) then
	echo Usage: $0 "<frequency>"
	exit 1
endif
set frequency	= $1
set nfutil	= /usr/spool/notes/.utilities
set exclude	= /usr/spool/notes/.utilities/short.names
#
echo -n "	" ${frequency} "NOTESFILE UPDATES begin: " ; date
echo " Lock files existing before updates: "; /bin/ls -l /usr/spool/notes/.locks
cd ${nfutil}
if ( ! -d ${frequency} ) then
	echo "no subdirectory for frequency" ${frequency}
	exit 2
endif
cd ${frequency}
#
# actually go do it
# note that we use "-a" on the nfxmit line which means that articles
# from the news subsystem are also moved.
#
foreach i ( ?.* )
	set sys = `echo ${i} | sed 's/.\.\(.*\)/\1/'`
	echo -n " ------ Sending to system ${sys} at: "; date
	time /usr/bin/nfxmit -a -d${sys} -f ${i} -f ${exclude}
end
#
# clean up everything and quit
#
echo " Lock files existing after updates: "; /bin/ls -l /usr/spool/notes/.locks
echo -n "	" ${frequency} "NOTESFILES UPDATES completed: " ; date
exit 0
