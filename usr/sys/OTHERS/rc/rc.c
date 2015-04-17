/*
 *	SCCS id	@(#)rc.c	2.1 (Berkeley)	8/5/83
 */

#include "rc.h"
#if	NRC > 0
#include "param.h"
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/seg.h>
#include <sys/time.h>
#include <sys/rcreg.h>

extern	struct rcdevice *RCADDR;

rcread(dev, uio)
dev_t	dev;
struct uio *uio;
{
	struct tm t;

	if(uio->uio_resid != sizeof(t)) {
		u.u_error = EINVAL;
		return;
	}
	t.tm_year = (((RCADDR->rcymd) >> 9) & 0177);
	t.tm_mon = (((RCADDR->rcymd) >> 5) & 017);
	t.tm_mday = ((RCADDR->rcymd) & 037);
	t.tm_hour = (((RCADDR->rchm) >> 8) & 037);
	t.tm_min = ((RCADDR->rchm) & 077);
	t.tm_sec = ((RCADDR->rcsec) & 077);
	t.tm_wday = -1;
	t.tm_yday = -1;
	t.tm_isdst = -1;

	if (copyout((caddr_t)&t, (caddr_t)uio->uio_iov->iov_base, sizeof t) < 0)
		  u.u_error = EFAULT;
	uio->uio_resid -= sizeof t;
}

rcwrite(dev, uio)
dev_t	dev;
struct uio *uio;
{
	register ymd;
	register hm;
	struct	tm t;


	if(uio->uio_resid != sizeof(t)) {
		u.u_error = EINVAL;
		return;
	}
	if (copyin((caddr_t)uio->uio_iov->iov_base,(caddr_t)&t,sizeof(t)) < 0) {
		u.u_error = EINVAL;
		return;
	}
	if (suser()) {
		if((t.tm_year < 0) || (t.tm_year > 99) ||
		   (t.tm_mon < 1) || (t.tm_mon > 12) ||
		   (t.tm_mday < 1) || (t.tm_mday > 31) ||
		   (t.tm_hour < 0) || (t.tm_hour > 23) ||
		   (t.tm_min < 0) || (t.tm_min > 59))
			{
			u.u_error = EINVAL;
			return;
		}
		ymd = ((((t.tm_year) << 4) | t.tm_mon) << 5) | t.tm_mday;
		hm = (t.tm_hour << 8) | t.tm_min;

		do {				/*set rcymd field*/
			RCADDR->rcymd = ymd;
			while (RCADDR->rcymd != ymd)
				;
			RCADDR->rcsec = ymd;
		}  while (RCADDR->rcymd != ymd);

		do {				/*set rthm field*/
			RCADDR->rchm = hm;
			while (RCADDR->rchm != hm)
				;
			RCADDR->rcsec = hm;
		}  while (RCADDR->rchm != hm);
	}
}
#endif	NRC
