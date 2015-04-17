# include	"../ingres.h"
# include	"../aux.h"
# include	"../batch.h"
# include	"../access.h"

/*
**	RESETREL -- will change a relation to an empty heap.  This is only
**		to be used on temporary relations and should only be called
**		by the DECOMP process.
*/

resetrel(pc, pv)
int	pc;
char	**pv;

{
	extern struct descriptor	Reldes;
	struct descriptor		desc;
	char				relname[MAXNAME + 4];
	long				lnum;

	opencatalog("relation", 2);
	while (*pv != -1)
	{
		if (openr(&desc, -1, *pv))
			syserr("RESETREL: openr %s", *pv);
		if (!bequal(Usercode, desc.relowner, sizeof desc.relowner))
			syserr("RESETREL: not owner of %s", *pv);
		ingresname(desc.relid, desc.relowner, relname);
		if ((desc.relfp = creat(relname, FILEMODE)) < 0)
			syserr("RESETREL: create %s", relname);
		lnum = 1;
		if (formatpg(&desc, lnum))
			syserr("RESETREL: formatpg %s", relname);
		desc.reltups = 0;
		desc.relspec = M_HEAP;
		desc.relprim = 1;
		close(desc.relfp);
		if (replace(&Reldes, &desc.reltid, &desc, FALSE) < 0)
			syserr("RESETREL: replace rel %s", relname);
		pv++;
	}
	return (0);
}
