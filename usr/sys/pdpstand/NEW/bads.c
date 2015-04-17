/*
 * SCCSID: @(#)bads.c	3.1	3/26/87
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
 * ULTRIX-11 standalone quick disk surface verifer
 *
 * Fred Canter   2/26/83
 * Jerry Brenner 12/16/82
 *
 * Functionality:
 *
 * Scan:
 * This routine prints the contents of the bad sector file
 * on a disk pack, per DEC standard 144.
 * It also scans a selected area for the disk for bad blocks
 * by reading the data, this is a very gross check at best.
 *
 *	This code arbitrarily limits the maximum number of
 *	bad sectors to the sectors per track count of that disk.
 *	e.g. RK06/07  maximum of 22 bad sectors.
 *
 */

#include "sa_defs.h"
#include <sys/param.h>
#include <sys/bads.h>

/*
 * This programs accesses physical devices only.
 * Must use 512 instead of BSIZE (1024 for new file system).
 * Fred Canter 6/12/85
 */
#undef	BSIZE
#define	BSIZE	512

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
	"rk05",	0,	"rk",	4872L, 		12, 0, 0, 0,
	"rl01",	0,	"rl",	10240L,		20, 0, 0, 0,
	"rl02",	0,	"rl",	20480L,		20, 0, 0, 0,
	"ml11",	NP,	"hp",	8192L,	 	16, 0, 0, 0,
	"ml11_0", HP,	"hp",	8192L,	 	16, 0, 0, 0,
	"ml11_1", HM,	"hm",	8192L,	 	16, 0, 0, 0,
	"ml11_2", HJ,	"hj",	8192L,	 	16, 0, 0, 0,
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
	"rp02",	0,	"rp",	10L*20L*200L,	10, 0, 0, 0,
	"rp03",	0,	"rp",	10L*20L*400L,	10, 0, 0, 0,
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
/*
 * RA disks can't do full track reads because of buffer size.
 * UDA50 buffering makes < full track reads plenty big enough.
 * # sectors must divide evenly into total disk size
 */
	"ra60",	0,	"ra",	400176L,	28, 0, 0, 0,
	"ra80",	0,	"ra",	236964L,	28, 0, 0, 0,
	"ra81",	0,	"ra",	891072L,	32, 0, 0, 0,
	"rx50",	0,	"rx",	800L,		10, 0, 0, 0,
	"rx33",	0,	"rx",	2400L,		10, 0, 0, 0,
	"rd31",	0,	"rd",	41560L,		20, 0, 0, 0,
	"rd32",	0,	"rd",	83204L,		22, 0, 0, 0,
	"rd51",	0,	"rd",	21600L,		18, 0, 0, 0,
	"rd52",	0,	"rd",	60480L,		18, 0, 0, 0,
	"rd53",	0,	"rd",	138672L,	18, 0, 0, 0,
	"rd54",	0,	"rd",	311200L,	20, 0, 0, 0,
	"rc25",	0,	"rc",	50902L,		31, 0, 0, 0,
	0,
};

struct	dkbad dkbad;
struct dkinfo *dip;
struct bt_bad *bt;
int i;
daddr_t	bn;
long nblk, nbo;
int fd, rcnt;
long	atol();
char	line[20];
char	fn[30];		/* file spec i.e., hp(0,0) */
char	dt[20];	/* disk type rm03, etc */
char	dn[2];		/* drive number */
struct {
	int t_cn;
	int t_tn;
	int t_sn;
}da;
char	buf[(BSIZE+4)*32];
union {
	long serl;
	int  seri[2];
}dsk;
char	*bblock = "Block	Cyl	Trk	Sec\n";

extern int argflag;	/* 0=interactive, 1=called by sdload */

main()
{
	printf("\n\nQuick Bad Block Scan Program\n");
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
		printf("\n`%s' not a vaild disk type !\n", dt);
rt_xit:
		if(argflag)
			exit(FATAL);
		else
			goto retry;
	}
	if(dip->di_size == 0L){
		printf("\n`%s' not supported by standalone code !\n", dt);
		goto rt_xit;
	}
	if(dip->di_nsect == 0){
		printf("\n`%s' not supported by Bads !\n", dt);
		goto rt_xit;
	}
	printf("\nUnit number: ");
	gets(dn);
	if((strlen(dn) != 1) || (dn[0] < '0') || (dn[0] > '3')) {
		printf("\nUnits 0 -> 3 only !\n");
		goto rt_xit;
	}
	sprintf(fn, "%s(%s,0)", dip->di_name, dn);
	dskopen(0);
	if(dip->di_ntrak){
		bn = dip->di_size - dip->di_nsect;
		printf("\nPrint bad sector file <[y] or n> ? ");
		gets(line);
		if(line[0] == 'y' || line[0] == '\0')
			if(showbad()) {
				printf("\n\7\7\7Invalid or No bad sector file.\n");
				printf("Disk pack must be initialized with\n");
				printf("DSKINIT before use with ULTRIX-11.\n\n");
				if(argflag)
					exit(NO_BBF);
			}
	}
	printf("\nScan disk pack for bad blocks <[y] or n> ? ");
	gets(line);
	if(line[0] == 'y' || line[0] == '\0')
		scanbad();
	close(fd);
	goto retry;
}

showbad(){
	if(readbad()){
		return(1);
	}
	dsk.seri[0] = dkbad.bt_csnh;
	dsk.seri[1] = dkbad.bt_csnl;
	printf("Cartridge serial number: %D\n", dsk.serl);
	switch(dkbad.bt_flag) {
		case -1:
			printf("\nAlignment cartridge !\n");
			return(1);
			break;
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
			printf("Pack has Invalid Bad Sector file\n");
			return(-1);
		}
		bn=((long)bt->bt_cyl *
		   (long)dip->di_ntrak+(long)(bt->bt_trksec>>8)) *
		   (long)dip->di_nsect + (long)(bt->bt_trksec&0377);
		printf("%D\t  %u\t  %d\t  %d\n", bn, bt->bt_cyl,
			bt->bt_trksec>>8, bt->bt_trksec&0377);
	}
	return(0);
}

scanbad(){
	int badcnt, rsize, j;
	long rbn, rrbn;

	printf("\nBlock offset: ");
	gets(line);
	if(strlen(line) > 6 || (nbo = atol(line)) < 0 || nbo > dip->di_size){
		printf("\nBad offset !\n");
		return(1);
	}
	printf("\n# of blocks <cr for full pack>: ");
	gets(line);
	if(line[0] == '\0')
		nblk = dip->di_size;
	else
		nblk = atol(line);
	if(nblk <= 0) {
		printf("\nBad # of blocks !\n");
		return(1);
	}
	if((nbo + nblk) > dip->di_size){
		nblk = dip->di_size - nbo;
		printf("Offset + # of blocks too large\n");
		printf("Truncating # of blocks to %D\n", nblk);
	}
	rsize = dip->di_nsect * BSIZE;
	printf("READING\n");
	for(badcnt = bn = 0; bn<nblk;) {
		lseek(fd, (bn+nbo)*BSIZE, 0);
		BAD_CMD->r[0] = BAD_CHK;
		if(rsize <= 0 || rsize > (BSIZE*dip->di_nsect))
			rsize = BSIZE*dip->di_nsect;
		rcnt = read(fd, buf, rsize);
		if(rcnt < 0){
			rbn = bn+nbo;
			if(dip->di_ntrak){
			    btoa(rbn);
			    printf("\n\nFATAL Error at ");
			    printf("Block %D\t cyl %d\t  trk %d\t  sec %d\n"
			       , rbn, da.t_cn, da.t_tn, da.t_sn);
			}
			else {
				printf("\n\nBAD BLOCK IN CLUSTER: ");
				printf("finding actual block number\n");
				rrbn = bn+nbo;
				for(j=0; ((j*BSIZE)<rsize); j++) {
					lseek(fd, (long)(rrbn*BSIZE), 0);
					rcnt = read(fd, (char *)&buf, BSIZE);
					if(rcnt != BSIZE) {
					    badcnt++;
					    printf("%s%D\t\t\t\t%s", bblock,
						rrbn,
						"BAD BLOCK\n");
					}
					rrbn++;
				}
				goto cont;
			}
			exit(FATAL);
		}
		else if(rcnt !=  rsize){
			badcnt++;
			rbn = (bn+nbo) + (rcnt/BSIZE);	
			if(dip->di_ntrak){
			    btoa(rbn);
				printf("%s%D\t%d\t%d\t%d\t%s"
				, bblock
				, rbn, da.t_cn, da.t_tn, da.t_sn
				,"BAD BLOCK\n\n");
			}
			else
			    printf("%s%D\t\t\t\t\t%s",bblock, rbn, "BAD BLOCK\n");
			bn = ++rbn - nbo;
			rsize = (dip->di_nsect - ((rcnt/BSIZE)+1))* BSIZE;
		}
		else{
	cont:
			bn += dip->di_nsect;
			bn -= bn%dip->di_nsect;
			rsize = dip->di_nsect * BSIZE;
		}
		if(badcnt >= dip->di_nsect && dip->di_ntrak){
			printf("\n\nTOO MANY BAD BLOCKS ON THIS PACK.\n");
			printf("ONLY %d BAD BLOCKS ALLOWED ON AN %s DISK\n"
				, dip->di_nsect, dip->di_type);
			printf("DO NOT USE THIS PACK\n");
			exit(FATAL);
		}
	}
	printf("\n%D blocks checked\n", bn);
	printf("%d bad blocks found\n", badcnt);
	if(argflag && badcnt)
		exit(HASBADS);
	else
		return(0);
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
		printf("\nCan't read bad sector file for %s unit %s !\n"
		, dip->di_type, dn);
		exit(FATAL);
	}
	bt = dkbad.bt_badb;
	if(dkbad.bt_mbz || (dkbad.bt_csnl == 0 && dkbad.bt_csnh == 0)){
		dkbad.bt_mbz = -1;
		return(1);
	}
	return(0);
}

dskopen(mode)
int mode;
{
	if((fd = open(fn, mode)) <= 0) {
		printf("\nCan't open %s !\n", fn);
		exit(FATAL);
	}
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
