#include "rm.h"

#if NRM > 0
int	rmstrategy(), rmread(), rmwrite(), rmroot();
extern	struct	buf	rmtab;
#define	rmopen		nulldev
#define	rmclose		nulldev
#define	_rmtab		&rmtab
#else
#define	rmopen		nodev
#define	rmclose		nodev
#define	rmroot		nulldev
#define	rmstrategy	nodev
#define	rmread		nodev
#define	rmwrite		nodev
#define	_rmtab		((struct buf *) NULL)
#endif	NRM

/* rm = XX */
	rmopen,		rmclose,	rmread,		rmwrite,
	nodev,		nulldev,	0,		SELECT(seltrue)

/* rm = X */
	rmopen,		rmclose,	rmstrategy,	rmroot,		_rmtab,
