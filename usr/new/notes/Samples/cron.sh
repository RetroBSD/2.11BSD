#! /bin/sh
# generic notes update script. looks in appropriate subdirectory and
# sends updates to the specified sites.
# the subdirectories are in /usr/spool/notes/.utilities
# the files are named grade.site -- grade is a single character
# and is used to force specific ordering in the wildcard expansion
#
#	check arguments, set up variables
#
if [ x$1 = x ];  then
	echo Usage: $0 "<frequency>"
	exit 1
fi
frequency=$1
nfutil=/usr/spool/notes/.utilities
exclude=/usr/spool/notes/.utilities/short.names
#
echo -n "	" ${frequency} "NOTESFILE UPDATES begin: " ; date
echo " Lock files existing before updates: "; /bin/ls -l /usr/spool/notes/.locks
cd ${nfutil}
if [ ! -d ${frequency} ]; then
	echo "no subdirectory for frequency" ${frequency}
	exit 2
fi
cd ${frequency}
#
# actually go do it
#
for i in ?.* ; do
	sys=`echo ${i} | sed 's/.\.\(.*\)/\1/'`
	echo -n " ------ Sending to system ${sys} at: "; date
echo	time /usr/bin/nfxmit -a -d${sys} -f ${i} -f ${exclude}
done
#
# clean up everything and quit
#
echo " Lock files existing after updates: "; /bin/ls -l /usr/spool/notes/.locks
echo -n "	" ${frequency} "NOTESFILES UPDATES completed: " ; date
exit 0
