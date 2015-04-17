#include "rm.h"

#if	NRM > 0				/* RJM02/RWM03, RM02/03/05 */
	DEVTRAP(254,	rmintr,	br5)
#endif

#if NRM > 0				/* RJM02/RWM03, RM02/03/05 */
	HANDLER(rmintr)
#endif

