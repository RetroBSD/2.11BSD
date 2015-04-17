#include "hp.h"
#if NHP > 0
int	hpstrategy(), hproot();
extern	struct	buf	hptab;
#define	hpopen		nulldev
#define	hpclose		nulldev
#define	_hptab		&hptab
#else
#define	hpopen		nodev
#define	hpclose		nodev
#define	hproot		nulldev
#define	hpstrategy	nodev
#define	_hptab		((struct buf *) NULL)
#endif	NHP

/* hp = 6 */
	hpopen,		hpclose,	hpstrategy,	hproot,		_hptab,
/* hp = 14 */
	hpopen,		hpclose,	hpread,		hpwrite,
	nodev,		nulldev,	0,		SELECT(seltrue),
	hpstrategy,
