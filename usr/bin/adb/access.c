#include "defs.h"

	MSG	ODDADR;
	MSG	BADDAT;
	MSG	BADTXT;
	MAP	txtmap;
	MAP	datmap;
	int	wtflag;
	char	*errflg;
	int	pid;
	struct	ovlhdr	ovlseg;
	long	ovloff[];
	char	curov;
	int	overlay;
	long	var[];
	static	within();

/* file handling and access routines */

put(adr,space,value)
	long   adr;
{
	acces(WT,adr,space,value);
}

u_int
get(adr, space)
long	adr;
{
	return(acces(RD,adr,space,0));
}

u_int
chkget(n, space)
	long	n;
{
	register int w;

	w = get(n, space);
	chkerr();
	return(w);
}

acces(mode,adr,space,value)
	long	adr;
{
	int	w, w1, pmode, rd, file;
	BKPTR   bkptr, scanbkpt();

	rd = mode==RD;
	IF space == NSP THEN return(0); FI

	IF pid          /* tracing on? */
	THEN IF (adr&01) ANDF !rd THEN error(ODDADR); FI
	     pmode = (space&DSP?(rd?PT_READ_D:PT_WRITE_D):(rd?PT_READ_I:PT_WRITE_I));
	     if (bkptr=scanbkpt((u_int)adr)) {
		if (rd) {
		    return(bkptr->ins);
		} else {
		    bkptr->ins = value;
		    return(0);
		}
	     }
	     w = ptrace(pmode, pid, shorten(adr&~01), value);
	     IF adr&01
	     THEN w1 = ptrace(pmode, pid, shorten(adr+1), value);
		  w = (w>>8)&LOBYTE | (w1<<8);
	     FI
	     IF errno
	     THEN errflg = (space&DSP ? BADDAT : BADTXT);
	     FI
	     return(w);
	FI
	w = 0;
	IF mode==WT ANDF wtflag==0
	THEN    error("not in write mode");
	FI
	IF !chkmap(&adr,space)
	THEN return(0);
	FI

	file = (space&DSP ? datmap.ufd : txtmap.ufd);
	if	(lseek(file,adr, 0) == -1L || 
			(rd ? read(file,&w,2) : write(file,&value,2)) < 1)
		errflg = (space & DSP ? BADDAT : BADTXT);
	return(w);
}

chkmap(adr,space)
	register long       *adr;
	register int	space;
{
	register MAPPTR amap;

	amap=((space&DSP?&datmap:&txtmap));
	switch(space&(ISP|DSP|STAR)) {

		case ISP:
			IF within(*adr, amap->b1, amap->e1)
			THEN *adr += (amap->f1) - (amap->b1);
				break;
			ELIF within(*adr, amap->bo, amap->eo)
			THEN *adr += (amap->fo) - (amap->bo);
				break;
			FI
			/* falls through */

		case ISP+STAR:
			IF within(*adr, amap->b2, amap->e2)
			THEN *adr += (amap->f2) - (amap->b2);
				break;
			ELSE goto err;
			FI

		case DSP:
			IF within(*adr, amap->b1, amap->e1)
			THEN *adr += (amap->f1) - (amap->b1);
				break;
			FI
			/* falls through */

		case DSP+STAR:
			IF within(*adr, amap->b2, amap->e2)
			THEN *adr += (amap->f2) - (amap->b2);
				break;
			FI
			/* falls through */

		default:
		err:
			errflg = (space&DSP ? BADDAT: BADTXT);
			return(0);
	}
	return(1);
}

setovmap(ovno)
	char	ovno;
	{
	register MAPPTR amap;

	if ((!overlay) || (ovno < 0) || (ovno > NOVL))
		return;
	amap = &txtmap;
	IF ovno == 0
	THEN    amap->eo = amap->bo;
		amap->fo = 0;
	ELSE    amap->eo = amap->bo + ovlseg.ov_siz[ovno-1];
		amap->fo = ovloff[ovno-1];
	FI
	var[VARC] = curov = ovno;
	if (pid)
		choverlay(ovno);
}

static
within(adr,lbd,ubd)
	long	adr, lbd, ubd;
{
	return(adr>=lbd && adr<ubd);
}
