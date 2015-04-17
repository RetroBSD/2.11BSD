#include "defs.h"
#include <sys/file.h>

	MAP	txtmap;
	MAP	datmap;
	int	wtflag;
	int	kernel;
	int	fcor;
	int	fsym;
	long	maxfile;
	long	maxstor;
	long	txtsiz;
	long	datsiz;
	long	bss;
	long	datbas;
	long	stksiz;
	char	*errflg;
	int	magic;
	long	entrypt;
	long	var[];
	int	argcount;
	int	signo;
	u_int	corhdr[ctob(USIZE)/sizeof(u_int)];
	u_int	*uar0 = UAR0;
	char	*symfil  = "a.out";
	char	*corfil  = "core";
	struct	ovlhdr	ovlseg;
	long	ovlsiz;
	long	ovloff[NOVL+1];
	char	startov;
	int	overlay;
	off_t	symoff, stroff;

setsym()
{
	struct	xexec	hdr;
	int ovly;

	bzero(&txtmap, sizeof (txtmap));
	fsym=getfile(symfil,1);
	txtmap.ufd=fsym;

	if	(read(fsym, &hdr, sizeof (hdr)) >= (int)sizeof (hdr.e))
		{
		magic= hdr.e.a_magic;
		txtsiz = hdr.e.a_text;
		datsiz = hdr.e.a_data;
		bss = hdr.e.a_bss;
		entrypt = hdr.e.a_entry;

		txtmap.f1 = N_TXTOFF(hdr.e);
		symoff = N_SYMOFF(hdr);
		stroff = N_STROFF(hdr);

		switch (magic)
			{
			case 0407:
				txtmap.e1 = txtmap.e2 = txtsiz+datsiz;
				txtmap.f2 = txtmap.f1;
				break;
			case 0410:
				txtmap.e1 = txtsiz;
				txtmap.b2 = round(txtsiz, TXTRNDSIZ);
				txtmap.e2 = txtmap.b2+datsiz;
				txtmap.f2 = txtsiz + txtmap.f1;
				break;
			case 0405:
			case 0411:
				txtmap.e1 = txtsiz;
				txtmap.e2 = datsiz;
				txtmap.f2 = txtsiz + txtmap.f1;
				break;
			case 0430:
			case 0431:
				ovlseg = hdr.o;
				txtmap.e1 = txtsiz;
				txtmap.bo = round(txtsiz, TXTRNDSIZ);
				FOR ovly = 0; ovly < NOVL; ovly++
				DO      ovloff[ovly] = ovlsiz + txtsiz 
						+ txtmap.f1;
					ovlsiz += ovlseg.ov_siz[ovly];
				OD
				IF magic == 0430
				THEN    txtmap.b2 =
					round(txtmap.bo+(long)ovlseg.max_ovl, TXTRNDSIZ);
				FI
				txtmap.f2 = txtmap.f1 + txtsiz+ovlsiz;
				txtmap.e2 = txtmap.b2 + datsiz;
				overlay = 1;
				break;
			default:
				magic = 0;
				txtsiz = 0;
				datsiz = 0;
				bss = 0;
				entrypt = 0;
			}
		datbas = txtmap.b2;
		symINI(&hdr);
		}
	IF magic==0 THEN txtmap.e1=maxfile; FI
}

setcor()
{
	fcor=getfile(corfil,2);
	datmap.ufd=fcor;
	IF read(fcor, corhdr, sizeof corhdr)==sizeof corhdr
	THEN    IF !kernel
		THEN    txtsiz = ((U*)corhdr)->u_tsize << 6;
			datsiz = ((U*)corhdr)->u_dsize << 6;
			stksiz = ((U*)corhdr)->u_ssize << 6;
			datmap.f1 = ctob(USIZE);
			datmap.b2 = maxstor-stksiz;
			datmap.e2 = maxstor;
		ELSE    datsiz = round(datsiz+bss,64L);
			stksiz = (long) ctob(USIZE);
			datmap.f1 = 0;
			datmap.b2 = 0140000L;
			datmap.e2 = 0140000L + ctob(USIZE);
		FI
		switch (magic)
			{
			case 0407:
				datmap.b1 = 0;
				datmap.e1 = txtsiz+datsiz;
				IF kernel
				THEN    datmap.f2 = (long)corhdr[KA6] *
						0100L;
				ELSE    datmap.f2 = ctob(USIZE)+txtsiz+datsiz;
				FI
				break;

			case 0410:
				datmap.b1 = round(txtsiz, TXTRNDSIZ);
				datmap.e1 = datmap.b1+datsiz;
				datmap.f2 = datsiz+ctob(USIZE);
				break;

			case 0405:
			case 0411:
			case 0431:
				datmap.b1 = 0;
				datmap.e1 = datsiz;
				IF kernel
				THEN datmap.f2 = (long)corhdr[KA6] *
					0100L;
				ELSE datmap.f2 = datsiz+ctob(USIZE);
				FI
				break;

			case 0430:
				datmap.b1 = round(round(txtsiz,
					TXTRNDSIZ)+ovlseg.max_ovl,
					TXTRNDSIZ);
				datmap.e1 = datmap.b1+datsiz;
				IF kernel
				THEN    datmap.b1 = 0;
					datmap.f2 = (long)corhdr[KA6] *
						0100L;
				ELSE    datmap.f2 = datsiz+ctob(USIZE);
				FI
				break;

			default:
				magic = 0;
				datmap.b1 = 0;
				datmap.e1 = maxfile;
				datmap.f1 = 0;
				datmap.b2 = 0;
				datmap.e2 = 0;
				datmap.f2 = 0;
			}
		datbas = datmap.b1;
		if (!kernel && magic) {
			/*
			 * Note that we can no longer compare the magic
			 * number of the core against that of the object
			 * since the user structure no longer contains
			 * the exec header ...
			 */
			register u_int *ar0;
			ar0 = (u_int *)(((U *)corhdr)->u_ar0);
			if (ar0 > (u_int *)0140000
			    && ar0 < (u_int *)(0140000 + ctob(USIZE))
			    && !((unsigned)ar0&01))
				uar0 = (u_int *)&corhdr[ar0-(u_int *)0140000];
			if (overlay) {
				startov = ((U *)corhdr)->u_ovdata.uo_curov;
				var[VARC] = (long)startov;
				setovmap(startov);
			}
			/* else dig out __ovno if overlaid? */
		}
	ELSE    datmap.e1 = maxfile;
	FI
}

getfile(filnam,cnt)
	char	*filnam;
	int	cnt;
{
	register int f;

	IF strcmp("-",filnam)
	THEN    f = open(filnam,wtflag);
		IF f < 0 ANDF argcount>cnt
		THEN    IF wtflag
			THEN    f = open(filnam, O_CREAT|O_TRUNC|wtflag,644);
			FI
			IF f < 0
			THEN printf("cannot open `%s'\n", filnam);
			FI
		FI
	ELSE    f = -1;
	FI
	return(f);
}
