#! /bin/sh
#
#	@(#)suggest.sh	4.4.1	(2.11BSD)	1996/10/25
#
trap 'rm $$; exit' 1 2 3 15
D=/usr/share/misc/explain.d
while echo "phrase?";read x
do
cat >$$ <<dn
/$x.*	/s/\(.*\)	\(.*\)/use "\2" for "\1"/p
dn
case $x in
[a-z]*)
sed -n -f $$ $D; rm $$;;
*) rm $$;;
esac
done
