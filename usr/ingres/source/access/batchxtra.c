# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"../batch.h"

rmbatch()
{
	char		*batchname();
	register char	*p;
	register int	i;

	p = batchname();
	if (i = close(Batch_fp))
		syserr("rmbatch:can't close %s %d", p, i);
	if (i = unlink(p))
		syserr("rmbatch:can't unlink %s %d", p, i);
	Batchhd.mode_up = 0;
	return (0);
}

char *
batchname()
{
	extern char	*Fileset;
	char		*ztack();

	return(ztack("_SYSbatch", Fileset));
}
