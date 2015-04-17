#include "vp.h"
#if NVP > 0
int	vpopen(), vpclose(), vpwrite(), vpioctl();
#else
#define	vpopen		nodev
#define	vpclose		nodev
#define	vpwrite		nodev
#define	vpioctl		nodev
#endif	NVP

/* vp = XX */
	vpopen,		vpclose,	nodev,		vpwrite,
	vpioctl,	nulldev,	0,		SELECT(nodev)

