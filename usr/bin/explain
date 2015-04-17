#! /bin/sh
#
#	@(#)explain.sh	4.5.1	(2.11BSD)	1996/10/25
#
D=/usr/share/misc/explain.d
while	echo 'phrase?'
	read x
do
	case $x in
	[a-z]*)	sed -n /"$x"'.*	/s/\(.*\)	\(.*\)/use "\2" for "\1"/p' $D
	esac
done
