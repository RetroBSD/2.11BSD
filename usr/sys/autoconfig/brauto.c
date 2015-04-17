#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"
#include "brreg.h"

/*
 * Can't seem to force an interrupt reliably, so... we just return
 * the fact that we exist.  autoconfig has already checked for our CSR.
 */
brprobe(addr, vector)
	struct brdevice *addr;
	int vector;
{
/*
	stuff(BR_IDE | BR_IDLE | BR_GO, (&(addr->brcs.w)));
	DELAY(10L);
	stuff(0, (&(addr->brcs.w)));
*/
	return(ACP_EXISTS);
}
