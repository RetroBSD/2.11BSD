#! /bin/sh
#
#	@(#)diction.sh	4.5.1	(2.11BSD)	1996/10/25
#
D=/usr/bin
B=/usr/libexec/diction
echo $*
rest=
flag=
nflag=
mflag=-me
lflag=-ml
kflag=
file=
for i
do case $i in
 -f) flag=-f;shift; file=$1; shift; continue;;
-n) nflag=-n;shift; continue;;
-k) kflag=-k;shift; continue;;
 -mm) mflag=$1; shift; continue;;
-ms) mflag=$1;shift;continue;;
-me) mflag=$1;shift;continue;;
-ma) mflag=$1;shift;continue;;
-ml) lflag=$1;shift;continue;;
*) rest=$*; break;;
esac
done
 $D/deroff $kflag $lflag $mflag $rest^$B/dprog -d $nflag $flag $file
