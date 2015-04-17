#include "bk.h"

#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
caddr_t	ttwrite();
#endif


#if NBK > 0
	bkopen, bkclose, bkread, ttwrite,
	bkioctl, bkinput, nodev, nulldev,
#else
	nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev,
#endif
