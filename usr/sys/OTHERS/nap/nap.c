#include "nap.h"
#if NNAP > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/proc.h>

#define N_NAP	32
#define NAPPRI	31

int ttnap[N_NAP];

/*ARGSUSED*/
napioctl(dev, cmd, addr, flags)
dev_t	dev;
caddr_t addr;
{
	int wakeup();
	int *ptr;

	for(ptr=ttnap; *ptr && (ptr <= &ttnap[N_NAP]); ptr++)
		;
	if(ptr == &ttnap[N_NAP]) {
		u.u_error = ENXIO;
		return;
	}

	*ptr = u.u_procp->p_pid;
	timeout(wakeup, ptr, cmd);
	sleep(ptr, NAPPRI);
	*ptr = 0;
}

/*ARGSUSED*/
napclose(dev, flags)
dev_t	dev;
{
	int *ptr;

	for(ptr=ttnap; (*ptr != u.u_procp->p_pid) && (ptr <= &ttnap[N_NAP]); ptr++)
		;
	if(*ptr == u.u_procp->p_pid)
		*ptr = 0;
}
#endif	NNAP
