#include "param.h"
#include "dnreg.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

dnprobe(addr,vector)
	struct dndevice	*addr;
	int vector;
{
	stuff(DN_MINAB | DN_INTENB | DN_DONE, (&(addr->dnisr[0])));
	DELAY(5L);
	stuff(0, (&(addr->dnisr[0])));
	return(ACP_IFINTR);
}
