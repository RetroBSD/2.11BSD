# include	"../ingres.h"
# include	"../access.h"

formatpg(d, n)
struct descriptor	*d;
long			n;
{
	struct accbuf	buf;
	register char	*p;
	long		zero;

	if (Acc_head == 0)
		acc_init();
	zero = 0;
	if (lseek(d->relfp, zero, 0) < 0)
		return (-2);
	buf.rel_tupid = d->reltid;
	buf.filedesc = d->relfp;
	for (p = (char *)&buf; p <= buf.linetab; p++)
		*p = NULL;
	buf.nxtlino = 0;
	buf.linetab[0] = buf.firstup - &buf;
	buf.ovflopg = 0;
	for (buf.mainpg = 1; buf.mainpg < n; (buf.mainpg)++)
	{
		if (write(buf.filedesc, &buf, PGSIZE) != PGSIZE)
			return (-3);
	}
	buf.mainpg = 0;
	if (write(buf.filedesc, &buf, PGSIZE) != PGSIZE)
		return (-4);
	Accuwrite += n;
	return (0);
}
