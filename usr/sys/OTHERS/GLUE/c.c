#include "cat.h"
#if	NCT > 0
int	ctopen(), ctclose(), ctwrite();
#else
#define	ctopen		nodev
#define	ctclose		nodev
#define	ctwrite		nodev
#endif	NCT

#include "dn.h"
#if	NDN > 0
int	dnopen(), dnclose(), dnwrite();
#else
#define	dnopen		nodev
#define	dnclose		nodev
#define	dnwrite		nodev
#endif	NDN

#include	"rf.h"
#if	NRF > 0
int	rfstrategy(), rfread(), rfwrite();
extern	struct	buf	rftab;
#define	rfopen		nulldev
#define	rfclose		nulldev
#define	_rftab		&rftab
#else
#define	rfopen		nodev
#define	rfclose		nodev
#define	rfstrategy	nodev
#define	rfread		nodev
#define	rfwrite		nodev
#define	_rftab		((struct buf *) NULL)
#endif	NRF

#include "rp.h"
#if	NRP > 0
int	rpstrategy(), rpread(), rpwrite();
extern	struct	buf	rptab;
#define	rpopen		nulldev
#define	rpclose		nulldev
#define	_rptab		&rptab
#else
#define	rpopen		nodev
#define	rpclose		nodev
#define	rpstrategy	nodev
#define	rpread		nodev
#define	rpwrite		nodev
#define	_rptab		((struct buf *) NULL)
#endif	NRP

#include "hs.h"
#if	NHS > 0
int	hsstrategy(), hsread(), hswrite(), hsroot();
extern	struct	buf	hstab;
#define	_hstab		&hstab
#define	hsopen		nulldev
#define	hsclose		nulldev
#else
#define	hsopen		nodev
#define	hsclose		nodev
#define	hsstrategy	nodev
#define	hsread		nodev
#define	hswrite		nodev
#define	hsroot		nulldev
#define	_hstab		((struct buf *) NULL)
#endif	NHS

/* rp = 1 */
	rpopen,		rpclose,	rpstrategy,	nulldev,	_rptab,
/* rf = 2 */
	rfopen,		rfclose,	rfstrategy,	nulldev,	_rftab,
/* hs = 5 */
	hsopen,		hsclose,	hsstrategy,	hsroot,		_hstab,


/* dn = 7 */
	dnopen,		dnclose,	nodev,		dnwrite,
	nodev,		nulldev,	0,		SELECT(nodev)
/* rf = 10 */
	rfopen,		rfclose,	rfread,		rfwrite,
	nodev,		nulldev,	0,		SELECT(seltrue)
/* rp = 11 */
	rpopen,		rpclose,	rpread,		rpwrite,
	nodev,		nulldev,	0,		SELECT(seltrue)
/* hs = 13 */
	hsopen,		hsclose,	hsread,		hswrite,
	nodev,		nulldev,	0,		SELECT(seltrue)
/* ct = 27 */
	ctopen,		ctclose,	nodev,		ctwrite,
	nodev,		nodev,		0,		SELECT(seltrue)
