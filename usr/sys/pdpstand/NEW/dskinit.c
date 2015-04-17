/*
 * SCCSID: @(#)dskinit.c	3.0	4/21/86
 */
/*
 *		LICENSED FROM DIGITAL EQUIPMENT CORPORATION
 *			       COPYRIGHT (c)
 *		       DIGITAL EQUIPMENT CORPORATION
 *			  MAYNARD, MASSACHUSETTS
 *			      1985, 1986, 1987
 *			   ALL RIGHTS RESERVED
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
 *	NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
 *	EQUIPMENT CORPORATION.  DIGITAL MAKES NO REPRESENTATIONS ABOUT
 *	THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  IT IS
 *	SUPPLIED "AS IS" WITHOUT EXPRESSED OR IMPLIED WARRANTY.
 *
 *	IF THE REGENTS OF THE UNIVERSITY OF CALIFORNIA OR ITS LICENSEES
 *	MODIFY THE SOFTWARE IN A MANNER CREATING DERIVATIVE COPYRIGHT
 *	RIGHTS, APPROPRIATE COPYRIGHT LEGENDS MAY BE PLACED ON THE
 *	DERIVATIVE WORK IN ADDITION TO THAT SET FORTH ABOVE.
 */

/*
 * ULTRIX-11 standalone disk surface verifer and formater
 *
 * Fred Canter   11/1/82
 * Jerry Brenner 12/16/82
 * Fred Canter 9/28/85
 *	Changes to allow format of unformatted packs.
 *
 * Functionality:
 *
 * Add:
 * This routine adds an entry to the bad sector file and
 * marks that sector as bad on the disk.
 *
 * Verify:
 * This routine formats (optional), and surface verifies a disk.
 * Up to 8 data patterns are utilized, including the worst
 * case pattern for the disk type.  The current bad sector file,
 * if valid, is used unless otherwise specified.
 *
 * BUGS:
 *	This code arbitrarily limits the maximum number of
 *	bad sectors to the sectors per track count of that disk.
 *	e.g. RK06/07  maximum of 22 bad sectors.
 *
 */

#include <sys/param.h>
#include <sys/bads.h>
#include "sa_defs.h"

/*
 * This programs accesses physical devices only.
 * Must use 512 instead of BSIZE (1024 for new file system).
 * Fred Canter 6/12/85
 */
#undef	BSIZE
#define	BSIZE	512

#define READ 1
#define WRITE 0

/*
 *	BAD144 info for disk bad blocking. A zero entry in
 *	di_size indicates that disk type has no bad blocking.
 */

#define	NP	-1
#define	HP	1
#define	HM	2
#define	HJ	3

struct dkinfo {
	char	*di_type;	/* type name of disk */
	int	di_flag;	/* prtdsk() flags */
	char	*di_name;	/* ULTRIX-11 disk name */
	long	di_size;	/* size of entire volume in blocks */
	int	di_nsect;	/* sectors per track */
	int	di_ntrak;	/* tracks per cylinder */
	int	di_wcpat[2];	/* worst case pattern */
} dkinfo[] = {
	"rk06",	0,	"hk",	22L*3L*411L,	22, 3, 0135143, 072307,
	"rk07",	0,	"hk",	22L*3L*815L,	22, 3, 0135143, 072307,
	"rm02",	NP,	"hp",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm02_0", HP,	"hp",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm02_1", HM,	"hm",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm02_2", HJ,	"hj",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm03",	NP,	"hp",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm03_0", HP,	"hp",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm03_1", HM,	"hm",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm03_2", HJ,	"hj",	32L*5L*823L,	32, 5, 0165555, 0133333,
	"rm05",	NP,	"hp",	32L*19L*823L,	32, 19, 0165555, 0133333,
	"rm05_0", HP,	"hp",	32L*19L*823L,	32, 19, 0165555, 0133333,
	"rm05_1", HM,	"hm",	32L*19L*823L,	32, 19, 0165555, 0133333,
	"rm05_2", HJ,	"hj",	32L*19L*823L,	32, 19, 0165555, 0133333,
	"rp04",	NP,	"hp",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp04_0", HP,	"hp",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp04_1", HM,	"hm",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rm04_2", HJ,	"hj",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp05",	NP,	"hp",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp05_0", HP,	"hp",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp05_1", HM,	"hm",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp05_2", HJ,	"hj",	22L*19L*411L,	22, 19, 0165555, 0133333,
	"rp06",	NP,	"hp",	22L*19L*815L,	22, 19, 0165555, 0133333,
	"rp06_0", HP,	"hp",	22L*19L*815L,	22, 19, 0165555, 0133333,
	"rp06_1", HM,	"hm",	22L*19L*815L,	22, 19, 0165555, 0133333,
	"rp06_2", HJ,	"hj",	22L*19L*815L,	22, 19, 0165555, 0133333,
	0,
};

/*
 * Pointer into the argument buffer,
 * used to flush the cartridge serial number
 * if it is not used by DSKINIT.
 * (see prf.c - getchar())
 */
extern char *argp;
struct	dkbad dkbad;
struct dkinfo *dip;
struct bt_bad *bt;
struct bt_bad tbt;
int i, badcnt, csn_used;
long	bn, atob();
long nblk, nbo;
int fd, rcnt;
long	atol();
char	line[20];
char	fn[30];		/* file spec i.e., hp(0,0) */
char	dt[20];	/* disk type rm03, etc */
char	dn[2];		/* drive number */
int	dskpat[8][2] = {
	0, 0,
	0125252, 0125252,
	0252525, 0252525,
	0177777, 0177777,
	0, 0,
	0, 0177777,
	0177777, 0,
	1, 1,
};

struct hk_fmt {
	int hkcyl;
	int hktrk;
	int hkxor;
};
struct hp_fmt {
	int hpcyl;
	int hptrk;
	long hpkey;
	int hpdat[256];
};
struct hm_fmt {
	int hmcyl;
	int hmtrk;
	int hmdat[256];
};
union {
	long	serl;		/* pack serial number as a long */
	int	seri[2];	/* serial number as two int's */
}dsk;

long	badbn[64];	/* array to hold bad block numbers */
int	newbad;
struct {
	int t_cn;
	int t_tn;
	int t_sn;
}da;
char	buf[(BSIZE+4)*32];
int	patbuf[BSIZE/sizeof(int)];
char	*bblock = "Block	Cyl	Trk	Sec\n";

int	argflag;	/* 0=interactive, 1=called by SDLOAD */

main()
{
	printf("\n\nDisk Surface Verify and Format Program\n");
retry:
	printf("\nDisk type <cr to exit, ? for list of disks>: ");
	gets(dt);
	if(dt[0] == '?'){
		prtdsk();
		goto retry;
	}
	if(strlen(dt) < 1)
		exit(NORMAL);
	for(dip=dkinfo; dip->di_type; dip++)
		if(strcmp(dip->di_type, dt) == 0)
			break;
	if(dip->di_type == 0) {
		printf("\n`%s' not a vaild disk type!\n", dt);
rt_xit:
		if(argflag)
			exit(FATAL);
		else
			goto retry;
	}
	if(dip->di_size == 0){
		printf("\n`%s' not supported in standalone mode!\n", dt);
		goto rt_xit;
	}
	if(dip->di_nsect == 0 || dip->di_ntrak == 0){
		printf("\n`%s' : No bad block support!\n", dt);
		goto rt_xit;
	}
	printf("\nUnit number: ");
	gets(dn);
	if((strlen(dn) != 1) || (dn[0] < '0') || (dn[0] > '3')) {
		printf("\nUnits 0 -> 3 only!\n");
		goto rt_xit;
	}
	sprintf(fn, "%s(%s,0)", dip->di_name, dn);
	/* force open, even if driver can't read the bad sector file */
	BAD_CMD->r[0] = BAD_CHK;
	dskopen(0);
	bn = dip->di_size - dip->di_nsect;	/* first sector of last track */
/*
 * MUST always show bad sector file, because
 * that is where we check if the pack is an 
 * alignment cartridge and exit if so.
	printf("\nPrint bad sector file <[y] or n> ? ");
	gets(line);
	if(line[0] == 'y' || line[0] == '\0')
*/
	if(showbad()) {
		printf("\nPack has invalid or no bad sector file\n");
		if(argflag)	/* discard sdload's answer (no question) */
			while(*argp++ != '\r') ;
	} else {	/* ask only if pack has valid bad sector file */
		printf("\n\nEnter additional bad blocks into bad sector ");
		printf("file <y or [n]> ? ");
		gets(line);
		if(line[0] == 'y')
			addbad();
	}
	printf("\n\nInitialize Pack <y or [n]> ? ");
	gets(line);
	if(line[0] == 'y')
		initdsk();
	goto retry;
}

showbad(){
	if(readbad()){
		printf("Not a valid bad sector file\n");
		return(1);
	}
	dsk.seri[0] = dkbad.bt_csnh;
	dsk.seri[1] = dkbad.bt_csnl;
	printf("Cartridge serial number: %D\n", dsk.serl);
	switch(dkbad.bt_flag) {
		case -1:
			printf("\nAlignment cartridge!\n");
			exit(FATAL);
		case 0:
			break;
		default:
			printf("\nBad sector file flag word = %o\n"
				    , dkbad.bt_flag);
			return(1);
	}
	printf("Block\t  Cyl\t  Trk\t  Sec\n");
	for(i=0; i<(BSIZE/sizeof(long)); i++, bt++) {
		if(bt->bt_cyl == 0177777)
			break;
		if(bt->bt_cyl == 0 && bt->bt_trksec == 0){
			dkbad.bt_mbz = -1;
			return(-1);
		}
		bn = atob(bt);
		printf("%D\t  %d\t  %d\t  %d\n", bn, bt->bt_cyl,
			bt->bt_trksec>>8, bt->bt_trksec&0377);
	}
	return(0);
}

addbad(){
	int chngd, cn, tn, sn, cnt;

	for(cnt = 0; cnt < dip->di_nsect; cnt++)
		badbn[cnt] = -1L;
	BAD_CMD->r[0] = BAD_CHK;	/* force open */
	dskopen(2);
	if(readbad()){
		/* Have to initialize bads */
		printf("Not a valid bad sector file\n");
		printf("Please initialize pack first\n");
		return(0);
	}
	if(dkbad.bt_flag == -1 && dkbad.bt_mbz == 0){
		printf("Alignment Cartridge\n");
		return(1);
	}
	printf("Adding to bad sector information\n");
	printf("Format is (cylinder track sector)enter values in decimal <cr to end>\n");
	bt = dkbad.bt_badb;
	for(i = 0; i < 126; i++, bt++){
		tomany(i);
		if(bt->bt_cyl == 0177777 || (bt->bt_cyl == 0
		   && bt->bt_trksec == 0))
			break;
	}
	for(chngd = cnt = 0; ;){
		tomany(i);
		gets(line);
		if(line[0] == '\0')
		{
			if(chngd){
				sortbn(0);
				setbads();
				sortbn(1);
				writbad();
				dskopen(2);
				if(strcmp(dip->di_name, "hk") == 0){
				   for(cnt = 0; (bn = badbn[cnt]) > 0L;cnt++){
					bn = (bn/dip->di_nsect)*dip->di_nsect;
					lseek(fd, bn*512, 0);
					write(fd, buf, dip->di_nsect*512);
				   }
				}
				showbad();
			}
			else
				printf("\nBad sector file not changed\n");
			return(0);
		}
		sscanf(line, "%d %d %d", &cn, &tn, &sn);
		if(cn >= dip->di_size/(dip->di_ntrak*dip->di_nsect)
		  || tn >= dip->di_ntrak || sn >= dip->di_nsect){
			printf("ERROR in specification. Max of ");
			printf("%o cyls  %o tracks %o sectors\n"
				, dip->di_size/(dip->di_ntrak*dip->di_nsect)
				, dip->di_ntrak, dip->di_nsect);
			continue;
		}
		if(chkdup(cn, tn, sn)){
			printf("\nThat sector is already marked bad\n");
			continue;
		}
		chngd++;
		tbt.bt_cyl = cn;
		tbt.bt_trksec = (tn<<8)|sn;
		bn = atob(&tbt);
		badbn[cnt++] = bn;
		i++;
	}
}

chkdup(cyl, trk, sec)
int cyl, trk, sec;
{
	register struct bt_bad *bpnt;
	int cnt;

	bpnt = dkbad.bt_badb;
	for(cnt = 0; cnt < 126; cnt++, bpnt++){
		if(bpnt->bt_cyl == 0177777)
			break;
		if(bpnt->bt_cyl == cyl
		   && bpnt->bt_trksec == ((trk << 8)|sec))
			return(1);
	}
	return(0);
}

initdsk(){
	int patcnt, baddat, cnt, cnt1, fmt, chk, chk1;
	int *tbuf;
	unsigned rsize;
	long bbn, tbn, maxbn;

	for(cnt = 0; cnt <= dip->di_nsect; cnt++)
		badbn[cnt] = -1;
	printf("Format Disk <y or [n]> ? ");
	gets(line);
	if(line[0] == 'y')
		fmt = 1;
	else
		fmt = 0;
	if(fmt == 1)	/* force open only if formatting */
		BAD_CMD->r[0] = BAD_CHK;
	else
		BAD_CMD->r[0] = 0;
	dskopen(2);
	printf("Use current bad sector file ? <[y] or n> ");
	gets(line);
	csn_used = 0;
	if(line[0] == 'n')
		initbads();
	else if(readbad()){
		printf("Bad Sector file Invalid - Initializing\n");
		csn_used++;
		initbads();
	}
	if(argflag && (csn_used == 0))	/* throw away cartridge SN */
		while(*argp++ != '\r') ;
/*
 * Load all entries in the current bad sector
 * file into badbn[] (flag them as bad blocks) so
 * they will not get lost when the disk is formatted.
 */
	if(fmt) {
		cnt1 = 0;
		for(cnt=0; cnt<126; cnt++) {
			if(dkbad.bt_badb[cnt].bt_cyl == -1)
				break;
			tbt.bt_cyl = dkbad.bt_badb[cnt].bt_cyl;
			tbt.bt_trksec = dkbad.bt_badb[cnt].bt_trksec;
			badbn[cnt1++] = atob(&tbt);
		}
	}
	while(1){
		printf("\nnumber of patterns <1 to 8> ?");
		gets(line);
		patcnt = atoi(line);
		if((strcmp(dip->di_name, "hk")) == 0
		   && (patcnt <= 0 || patcnt > 8) && fmt)
			continue;
		else if(patcnt < 0 || patcnt > 8)
			continue;
		break;
	}
	patcnt--;
	if(fmt != 0){
		if(formatdsk())
			exit(FATAL);
		printf("Format complete\n");
		setbads();
		for(cnt=0; badbn[cnt] != -1; cnt++)
			badbn[cnt] = -1;
	}
	dskpat[0][0] = dip->di_wcpat[0];
	dskpat[0][1] = dip->di_wcpat[1];
	if(fmt)
		maxbn = dip->di_size;
	else
		maxbn = dip->di_size - (dip->di_nsect * 2);
	for(i = patcnt, badcnt = 0; i >= 0 ; i--){
		if(strcmp(dip->di_name, "hk") || i != 0){
			ldbuf(dskpat[i][0],dskpat[i][1], i, 0);
		}
		if(i == 7)
			printf("\nPattern %d = %s", i+1, "Random");
		else{
		    printf("\nPattern %d = ", i+1);
		    fulprt(dskpat[i][1]);
		    fulprt(dskpat[i][0]);
		}
		rsize = BSIZE*dip->di_nsect;
		printf("\nWRITING\n");
		for(bn = 0; bn < maxbn ;){
			if((strcmp(dip->di_name, "hk")) == 0 && i == 0){
				if((bn/(dip->di_ntrak*dip->di_nsect))&01)
					/* odd cylinder */
					ldbuf(dskpat[i][1],dskpat[i][1],i, 0);
				else
					/* even cylinder */
					ldbuf(dskpat[i][0],dskpat[i][0],i, 0);
			}
			lseek(fd, bn * BSIZE, 0);
			BAD_CMD->r[0] = BAD_CHK;
			if(rsize <= 0 || rsize > (BSIZE*dip->di_nsect))
				rsize = BSIZE * dip->di_nsect;
			rcnt = write(fd, buf, rsize);
			if(rcnt < 0){
				btoa(bn);
				printf("\n\nFATAL Error at ");
				printf("Block %D\t cyl %d\t  trk %d\t  sec %d\n"
				  , bn, da.t_cn, da.t_tn, da.t_sn);
				exit(FATAL);
			}
			else if(rcnt != rsize){
				bbn = bn + (rcnt/BSIZE);	
				if(badblk(bbn)){
					badcnt++;
					btoa(bbn);
					btoa(bbn);
					printf("%s%D\t%d\t%d\t%d\t%s"
					, bblock
					, bbn, da.t_cn, da.t_tn, da.t_sn
					,"BAD SECTOR - WRITE ERROR\n\n");
				}
				rsize = (dip->di_nsect-((rcnt/BSIZE)))* BSIZE;
				bn = ++bbn;
			}
			else{
				rsize = BSIZE*dip->di_nsect;
				bn += dip->di_nsect;
				bn -= bn%dip->di_nsect;
			}
		}
		rsize = BSIZE*dip->di_nsect;
		printf("\nREADING (with ECC disabled)\n");
		for(bn = 0; bn < maxbn;){
			if((strcmp(dip->di_name, "hk")) == 0 && i == 0){
				if((bn/(dip->di_ntrak*dip->di_nsect))&01)
					/* odd cylinder */
					ldbuf(dskpat[i][1],dskpat[i][1],i, 1);
				else
					/* even cylinder */
					ldbuf(dskpat[i][0],dskpat[i][0],i, 1);
			}
			lseek(fd, bn * BSIZE, 0);
			BAD_CMD->r[0] = BAD_CHK;
/*
 * Fred Canter -- 8/20/85
 *	We want blocks with ECC errors to be revectored,
 *	whether or not we formatted the disk.
			if(fmt)
*/
				BAD_CMD->r[0] |= BAD_NEC;
			rcnt = read(fd, buf, rsize);
			if(rcnt < 0){
				btoa(bn);
				printf("\n\nFATAL Error at ");
				printf("Block %D\t cyl %d\t  trk %d\t  sec %d\n"
				  , bn, da.t_cn, da.t_tn, da.t_sn);
				exit(FATAL);
			}
			else if(rcnt != rsize){
				bbn = bn + (rcnt/BSIZE);	
				if(badblk(bbn)){
					badcnt++;
					btoa(bbn);
					printf("%s%D\t%d\t%d\t%d\t%s"
					, bblock
					, bbn, da.t_cn, da.t_tn, da.t_sn
					,"BAD SECTOR\n\n");
				}
				rsize = (dip->di_nsect-((rcnt/BSIZE)+1))* BSIZE;
				tbn = ++bbn;
			}
			else{
				rsize = BSIZE*dip->di_nsect;
				tbn =  bn + dip->di_nsect;
				tbn -= tbn%dip->di_nsect;
			}
			for(chk = 0; chk < (rcnt/BSIZE); chk++){
			    baddat = 0;
			    tbuf = buf + (chk*BSIZE);
			    for(chk1=0; chk1<(BSIZE/sizeof(int)); chk1++){
				if(*tbuf != patbuf[chk1]){
				    bbn = bn + chk;
				    btoa(bbn);
					printf("%s%D\t%d\t%d\t%d\t%s"
					, bblock
					, bbn, da.t_cn, da.t_tn, da.t_sn
					,"DATA COMPARE ERROR  ");
				    printf("sb ");
				    fulprt(patbuf[chk1]);
				    printf(" is ");
				    fulprt(*tbuf);
				    printf("\n");
				    if(++baddat > 4)
					break;
				}
				tbuf++;
			    }
			    if(baddat){
				bbn = bn + chk;
				if(badblk(bbn)){
					badcnt++;
					btoa(bbn);
					printf("%s%D\t%d\t%d\t%d\t%s"
					, bblock
					, bbn, da.t_cn, da.t_tn, da.t_sn
					,"BAD SECTOR\n\n");
				}
			    }
			}
			clrbuf();	
			bn = tbn;
		}
	}
	printf("\n\nVerify Complete\n");
	if(badcnt > 0 || newbad || fmt){
		sortbn(0);
		setbads();
		sortbn(1);
		writbad();
	}
	dskopen(2);
	if((strcmp(dip->di_name, "hk")) == 0){
		for(cnt = 0; (bn = badbn[cnt]) > 0L;cnt++){
			bn = (bn/dip->di_nsect)*dip->di_nsect;
			lseek(fd, bn*512, 0);
			write(fd, buf, dip->di_nsect*512);
		}
	}
	if(badcnt > 0)
		printf("%d%sbad blocks found\n"
			, badcnt, newbad?" ":" additional ");
	else
		printf("No%sbad blocks found\n",newbad?" ":" additional ");
	showbad();
}

initbads()
{
	register struct bt_bad *pnt;
	int cnt;

	newbad++;
	while(1) {
		printf("Cartridge serial number ? ");
		gets(line);
		for(cnt=0; line[cnt]; cnt++)
			if((line[cnt] < '0') || (line[cnt] > '9'))
				break;
		if(line[cnt] != '\0') {
			printf("\nSerial number - numbers only!\n");
			continue;
		}
		dsk.serl = atol(line);
		if(dsk.serl <= 0L) {
			printf("\nSerial number - must be greater than zero!\n");
			continue;
		}
		break;
	}
	dkbad.bt_csnl = dsk.seri[1];
	dkbad.bt_csnh = dsk.seri[0];
	dkbad.bt_mbz = 0;
	dkbad.bt_flag = 0;
	for(cnt = 0, pnt = dkbad.bt_badb; cnt < 126; cnt++, pnt++)
		pnt->bt_cyl = pnt->bt_trksec = 0177777;
}

formatdsk()
{
	struct hk_fmt *whk;
	struct hp_fmt *whp;
	struct hm_fmt *whm;
	int tcnt, scnt, tsize, wsize, rsec;
	long seekpnt;

	printf("Begin Format <y or [n]> ? ");
	gets(line);
	if(line[0] != 'y'){
		if(newbad)
			return(1);
		return(0);
	}
	printf("\nFormating %s %s\n", dip->di_type, dn);
	tcnt = 0;
	if(strcmp(dip->di_name, "hk") == 0){
		tsize = dip->di_nsect;
		wsize = tsize * sizeof(struct hk_fmt);
		for(; tcnt < (dip->di_size/tsize); tcnt++){
			whk = buf;
			for(scnt = 0; scnt < tsize; scnt++, whk++){
				whk->hkcyl = tcnt/dip->di_ntrak;
				whk->hktrk = (tcnt%dip->di_ntrak)<<5;
				whk->hktrk |= (scnt&037);
				whk->hktrk |= 0140000;
				whk->hkxor = (whk->hkcyl ^ whk->hktrk);
			}
			seekpnt =  (long)tcnt*dip->di_nsect*BSIZE;
			fmtfunc(WRITE, seekpnt, wsize);
		}
	}
	else if(strncmp(dip->di_type, "rm", 2) == 0){
		tsize = dip->di_nsect*sizeof(struct hm_fmt);
		for(; tcnt < (dip->di_size/dip->di_nsect); tcnt++){
			whm = buf;
			for(scnt = 0; scnt < dip->di_nsect; scnt++, whm++){
				whm->hmcyl = tcnt/dip->di_ntrak;
				whm->hmtrk = (tcnt%dip->di_ntrak)<<8;
				whm->hmtrk |= (scnt&037);
				whm->hmcyl |= 0150000;
			}
			seekpnt =  (long)tcnt*dip->di_nsect*BSIZE;
			fmtfunc(WRITE, seekpnt, tsize);
		}
	}
	else if(strcmp(dip->di_name, "hp") == 0
	  || strcmp(dip->di_name, "hm") == 0){
		tsize = dip->di_nsect*sizeof(struct hp_fmt);
		for(; tcnt < (dip->di_size/dip->di_nsect); tcnt++){
			whp = buf;
			for(scnt = 0; scnt < dip->di_nsect; scnt++,whp++){
				whp->hpcyl = (tcnt/dip->di_ntrak);
				whp->hptrk = (tcnt%dip->di_ntrak)<<8;
				whp->hptrk |= (scnt&037);
				whp->hpcyl |= 010000;
			}
			seekpnt =  (long)tcnt*dip->di_nsect*BSIZE;
			fmtfunc(WRITE, seekpnt, tsize);
		}
	}
	else{
		printf("Cannot format %s's\n", dip->di_type);
		return(1);
	}
	return(0);
}

badblk(bbn)
long bbn;
{
	register struct bt_bad *pnt;
	int cnt, cnt1, trksec;

	if(bbn >= (dip->di_size - (dip->di_nsect *2))){
		printf("Bad Block %D in Disk Bad Sector Area\n", bbn);
		printf("Pack Unacceptable for ULTRIX-11\n");
		exit(BADPACK);
	}
	btoa(bbn);
	trksec = da.t_tn << 8;
	trksec |= da.t_sn&0377;
	pnt = dkbad.bt_badb;
	for(cnt = 0; cnt < dip->di_nsect; cnt++, pnt++){
		if(pnt->bt_cyl == 0177777)
			break;
		if(pnt->bt_cyl == da.t_cn && pnt->bt_trksec == trksec)
			return(0);
	}
	for(cnt = 0; cnt < dip->di_nsect; cnt++){
		if(badbn[cnt] == bbn)
			return(0);
		if(badbn[cnt] == -1)
			break;
	}
	tomany(cnt);
	badbn[cnt] = bbn;
	return(1);
}

clrbuf()
{
	int cnt, *tbuf, pat;

	if(patbuf[0] == 0 && patbuf[1] == 0)
		pat = -1;
	else
		pat = 0;
	for(cnt = 0,tbuf = buf;cnt < (sizeof(buf)/sizeof(int));cnt++,tbuf++)
		*tbuf = pat;
}

ldbuf(lpat, hpat, flg, patflg)
int lpat, hpat, flg, patflg;
{
	int *rbuf, *tpat;
	int cnt, cnt1;

	if(flg == 7)
		srand(1);
	rbuf = patbuf;
	for(cnt = 0; cnt < ((BSIZE/sizeof(int))/2); cnt++) {
		if(flg == 7) {
			*(rbuf++) = rand();
			*(rbuf++) = rand();
		} else {
			*(rbuf++) = lpat;
			*(rbuf++) = hpat;
		}
	}
	if(patflg)
		return;
	for(cnt = 0, rbuf = buf; cnt < dip->di_nsect; cnt++){
		for(cnt1=0, tpat=patbuf; cnt1<(BSIZE/sizeof(int)); cnt1++)
			*(rbuf++) = *(tpat++);
	}
}

btoa(bn)
long bn;
{
	da.t_cn = bn/(dip->di_ntrak*dip->di_nsect);
	da.t_sn = bn%(dip->di_ntrak*dip->di_nsect);
	da.t_tn = da.t_sn/dip->di_nsect;
	da.t_sn = da.t_sn%dip->di_nsect;
}

readbad()
{
	int cnt;

	dkbad.bt_mbz = -1;
	bn = dip->di_size - dip->di_nsect;	/* first sector of last track */
	for(cnt = 0; cnt < 5; cnt++){
		lseek(fd, BSIZE * bn, 0);
		printf("\nBad sector file at block %D of %s\n", bn, fn);
		if(read(fd, &dkbad, sizeof(struct dkbad))
		  != sizeof(struct dkbad)) {
			bn += 2;
			continue;
		}
		break;
	}
	if(cnt >= 5){
		printf("\nCan't read bad sector file for %s unit %s!\n",
		dip->di_type, dn);
		printf("\nCAUTION: disk pack probably needs formatting!\n\n");
		dkbad.bt_mbz = -1;	/* force next to fail */
	}
	bt = dkbad.bt_badb;
	if(dkbad.bt_mbz || (dkbad.bt_csnl == 0 && dkbad.bt_csnh == 0)){
		dkbad.bt_mbz = -1;
		return(1);
	}
	return(0);
}

writbad()
{
	int cnt;

	bn = dip->di_size - dip->di_nsect;
	printf("\nWriting bad sector file");
	printf(" starting at block %D of %s\n", bn, fn);
	for(cnt = 0; cnt < 5; cnt++){
		lseek(fd, BSIZE * bn, 0);
		BAD_CMD->r[0] = BAD_CHK;
		write(fd, &dkbad, 512);
		bn += 2;
	}
}

fmtfunc(flag, pnt, size)
int flag;
long pnt;
int size;
{
	int rcnt;

	lseek(fd, pnt, 0);
	BAD_CMD->r[0] = BAD_FMT;
	if(flag == WRITE){
		if((rcnt = write(fd, buf, size)) != size){
			printf("FATAL: write format returned ");
			printf("%d  should be %d\n", rcnt , size);
			exit(FATAL);
		}
	}
	else
		if((rcnt = read(fd, buf, size)) != size){
			printf("FATAL: read format returned ");
			printf("%d  should be %d\n", rcnt , size);
			exit(FATAL);
		}
}

dskopen(mode)
int mode;
{
	if(fd > 0)
		close(fd);
	if((fd = open(fn, mode)) <= 0) {
		printf("\nCan't open %s!\n", fn);
		exit(FATAL);
	}
}

tomany(num)
int num;
{
	if(num > dip->di_nsect){
		printf("\nToo many bad blocks. Do not use this Pack\n");
		exit(BADPACK);
	}
}

fulprt(val)
int val;
{
	int cnt, cnv;
	char c;

	for(cnt = 5; cnt >= 0; cnt--){
		cnv = val&(07<<(cnt*3));
		if(cnt == 5)
			c = ((cnv>>1)&~0100000)>>14;
		else
			c = cnv>>(cnt*3);
		printf("%c",c+060);
	}
}

long atob(bt)
struct bt_bad *bt;
{
	long blkn;

	blkn = (long)bt->bt_cyl * (long)(dip->di_ntrak * dip->di_nsect);
	blkn += (long)((bt->bt_trksec >> 8) * dip->di_nsect);
	blkn += ((long)bt->bt_trksec & 0377);
	return(blkn);
}

sortbn(merge)
{
	long lbn, tbn[64];
	int max, cnt, cnt1, li;
	struct bt_bad *pt;

	for(cnt = 0; cnt < 64; cnt++)
		tbn[cnt] = -1L;
	if(merge)
		for(cnt = 0, pt = dkbad.bt_badb; ;cnt++, pt++){
			if(pt->bt_cyl == 0177777)
				break;
			tbn[cnt] = atob(pt);
		}
	else
		cnt = 0;
	for(cnt1 = 0; badbn[cnt1] != -1L; cnt1++){
		tbn[cnt] = badbn[cnt1];
		cnt++;
	}
	max = cnt;
	cnt = cnt1 = 0;
	for(lbn = tbn[0], li = 0; cnt < max; ){
		if(lbn == 0 && tbn[cnt1] > 0){
			lbn = tbn[cnt1];
			li = cnt1;
		}
		if(tbn[cnt1] == 0L){
			cnt1++;
			continue;
		}
		if(tbn[cnt1] == -1L && lbn > 0){
			badbn[cnt] = tbn[li];
			cnt++;
			tbn[li] = 0L;
			cnt1 = 0;
			lbn = 0;
			continue;
		}
		if(tbn[cnt1] == -1L)
			break;
		if(tbn[cnt1] < lbn){
			li = cnt1;
			lbn = tbn[cnt1];
		}
		cnt1++;
	}
	badbn[cnt] = -1L;
	if(merge){
		for(cnt = 0, pt = dkbad.bt_badb; ;cnt++, pt++){
			if(badbn[cnt] == -1L)
				break;
			btoa(badbn[cnt]);
			pt->bt_cyl = da.t_cn;
			pt->bt_trksec = da.t_tn << 8;
			pt->bt_trksec |= da.t_sn&0377;
		}
		for(;cnt < 126; cnt++, pt++)
			pt->bt_cyl = pt->bt_trksec = 0177777;
	}
}

setbads()
{
	struct hk_fmt *whk;
	struct hp_fmt *whp;
	struct hm_fmt *whm;
	int cnt, tcnt, scnt, tsize, wsize, csec;
	int	found;
	long seekpnt, bbn;
	struct {
		int cylnum;
		int trksecnum;
	}dskadr;

	if(strcmp(dip->di_name, "hk") == 0){
		tsize = dip->di_nsect;
		wsize = tsize * sizeof(struct hk_fmt);
		for(cnt = 0; badbn[cnt] != -1; ){
			tcnt = badbn[cnt]/tsize;
			seekpnt =  (long)tcnt*dip->di_nsect*BSIZE;
			fmtfunc(READ, seekpnt, wsize);
			whk = buf;
			found = 0;
			for(scnt = 0; scnt < dip->di_nsect; scnt++, whk++){
				dskadr.cylnum = whk->hkcyl;
				dskadr.trksecnum = (whk->hktrk&0340)<<3;
				dskadr.trksecnum |= whk->hktrk&037;
				if((bbn = atob(&dskadr)) == badbn[cnt]){
					found++;
					whk->hktrk &= ~040000;
					whk->hkxor = (whk->hkcyl ^ whk->hktrk);
					btoa(bbn);
					prtvec(bbn);
					if(badbn[++cnt] == -1)
						break;
				}
			}
			if(found == 0) {
			    printf("\nBad block not found in sector headers!\n");
			    exit(BADPACK);
			}
			fmtfunc(WRITE, seekpnt, wsize);
		}
	}
	else if(strncmp(dip->di_type, "rm", 2) == 0){
		tsize = sizeof(struct hm_fmt);
		for(cnt = 0; badbn[cnt] != -1; cnt++){
			seekpnt =  (long)(badbn[cnt]*BSIZE);
			fmtfunc(READ, seekpnt, tsize);
			whm = buf;
			if((whm->hmcyl & 0140000) != 0140000)
				continue;
			whm->hmcyl &= ~040000;
			btoa(badbn[cnt]);
			prtvec(badbn[cnt]);
			fmtfunc(WRITE, seekpnt, tsize);
		}
	}
	else if(strcmp(dip->di_name, "hp") == 0
	  || strcmp(dip->di_name, "hm") == 0){
		tsize = sizeof(struct hp_fmt);
		for(cnt = 0; badbn[cnt] != -1; cnt++){
			seekpnt =  (long)(badbn[cnt]*BSIZE);
			fmtfunc(READ, seekpnt, tsize);
			whp = buf;
			if((whp->hpcyl & 010000) == 0)
				continue;
			whp->hpcyl &= ~010000;
			btoa(badbn[cnt]);
			prtvec(badbn[cnt]);
			fmtfunc(WRITE, seekpnt, tsize);
		}
	}
}

prtvec(bbn)
long bbn;
{
	printf("Revectoring block #%D", bbn);
	printf("  cyl = %d  trk = %d  sec = %d\n"
		,da.t_cn, da.t_tn, da.t_sn);
}

prtdsk()
{
	struct dkinfo *dp;

	printf("\nDisk\tULTRIX\tSize in");
	printf("\nName\tName\tBlocks\tComments");
	printf("\n----\t----\t------\t--------");
	for(dp=dkinfo; dp->di_type; dp++){
		if(dp->di_flag == NP)
			continue;
		printf("\n%s\t", dp->di_type);
		printf("%s\t", dp->di_name);
		printf("%D\t", dp->di_size);
		if(dp->di_flag == HP)
			printf("(first  ) RH11/RH70 Controller");
		if(dp->di_flag == HM)
			printf("(second ) RH11/RH70 Controller");
		if(dp->di_flag == HJ)
			printf("(third  ) RH11/RH70 Controller");
	}
	printf("\n");
}
