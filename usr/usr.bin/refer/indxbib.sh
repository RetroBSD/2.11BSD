#! /bin/sh
#
#	@(#)indxbib.sh	4.1.1	(2.11BSD)	1996/10/23
#
#	indxbib sh script
#
if test $1
	then /usr/libexec/refer/mkey $* | /usr/libexec/refer/inv $1_
	mv $1.ia_ $1.ia
	mv $1.ib_ $1.ib
	mv $1.ic_ $1.ic
else
	echo 'Usage:  indxbib database [ ... ]
	first argument is the basename for indexes
	indexes will be called database.{ia,ib,ic}'
fi
