#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)makewhatis.sh	5.3 (Berkeley) 3/29/86
#
trap "rm -f /tmp/whatis$$; exit 1" 1 2 13 15
MANDIR=${1-/usr/man}
TMPFILE=/tmp/whatis$$
rm -f $TMPFILE
cp /dev/null $TMPFILE
if test ! -d $MANDIR ; then exit 0 ; fi
cd $MANDIR
top=`pwd`
for i in cat*
do
	if [ -d $i ] ; then
		cd $i
		for file in `find . -type f -name '*.0' -print`
		do
			sed -n -f /usr/man/makewhatis.sed $file
		done >> $TMPFILE
		cd $top
	fi
done
rm -f $top/whatis
sort -u $TMPFILE > $top/whatis
chmod 664 whatis >/dev/null 2>&1
rm -f $TMPFILE
exit 0
