echo	updating $1 $2 $3 $4 $5 $6 $7 $8 $9 in .../lib/monitor
cc	-c -O	$1 $2 $3 $4 $5 $6 $7 $8 $9
ar	vu	../../lib/monitor
rm	*.o
echo	done
