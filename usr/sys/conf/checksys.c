/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)checksys.c	1.6 (2.11BSD) 1998/12/5
 */

/*
 * checksys
 *	checks the system size and reports any limits exceeded.
 */

#include "param.h"
#include "user.h"
#include "file.h"
#include "ioctl.h"
#include "clist.h"
#include "a.out.h"
#include "stdio.h"
#include "namei.h"
#include "msgbuf.h"

/* Round up to a click boundary. */
#define	cround(bytes)	((bytes + ctob(1) - 1) / ctob(1) * ctob(1));
#define	round(x)	(ctob(stoc(ctos(btoc(x)))))

#define	KB(val)		((u_int)(val * 1024))

#define	N_END		0
#define	N_NBUF		1
#define	N_PROC		4
#define	N_NINODE	9
#define	N_CLIST		14
#define	N_RAM		15
#define	N_XITDESC	16
#define	N_QUOTDESC	17
#define	N_NAMECACHE	18
#define	N_IOSIZE	19
#define	N_NLOG		20
#define	N_NUMSYMS	21

	struct	nlist	nl[N_NUMSYMS];

char	*names[] = {
	"_end",				/*  0 */
	"_nbuf",			/*  1 */
	"_buf",				/*  2 */
	"_nproc",			/*  3 */
	"_proc",			/*  4 */
	"_ntext",			/*  5 */
	"_text",			/*  6 */
	"_nfile",			/*  7 */
	"_file",			/*  8 */
	"_ninode",			/*  9 */
	"_inode",			/* 10 */
	"_ncallout",			/* 11 */
	"_callout",			/* 12 */
	"_ucb_clist",			/* 13 */
	"_nclist",			/* 14 */
	"_ram_size",			/* 15 */
	"_xitdesc",			/* 16 */
	"_quotdesc",			/* 17 */
	"_namecache",			/* 18 */
	"__iosize",			/* 19 */
	"_nlog"				/* 20 */
	};

static struct exec obj;
static struct ovlhdr ovlhdr;
static int fi;

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	long size, totsize, ramdisk, getval();
	int errs = 0, texterrs = 0, ninode;

	if (argc != 2) {
		fputs("usage: checksys unix-binary\n", stderr);
		exit(-1);
	}
/*
 * Can't (portably) initialize unions, so we do it at run time
*/
	for (i = 0; i < N_NUMSYMS; i++)
		nl[i].n_un.n_name = names[i];
	if ((fi = open(argv[1], O_RDONLY)) < 0) {
		perror(argv[1]);
		exit(-1);
	}
	if (read(fi, &obj, sizeof(obj)) != sizeof(obj)) {
		fputs("checksys: can't read object header.\n", stderr);
		exit(-1);
	}
	if (obj.a_magic == A_MAGIC5 || obj.a_magic == A_MAGIC6)
		if (read(fi, &ovlhdr, sizeof(ovlhdr)) != sizeof(ovlhdr)) {
			fputs("checksys: can't read overlay header.\n", stderr);
			exit(-1);
		}
	switch(obj.a_magic) {

	/*
	 *	0407-- nonseparate I/D "vanilla"
	 */
	case A_MAGIC1:
	case A_MAGIC2:
		size = obj.a_text + obj.a_data + obj.a_bss;
		if (size > KB(48)) {
			printf("total size is %ld, max is %u, too large by %ld bytes.\n", size, KB(48), size - KB(48));
			errs++;
		}
		totsize = cround(size);
		break;

	/*
	 *	0411-- separate I/D
	 */
	case A_MAGIC3:
		size = obj.a_data + obj.a_bss;
		if (size > KB(48)) {
			printf("data is %ld, max is %u, too large by %ld bytes.\n", size, KB(48), size - KB(48));
			errs++;
		}
		totsize = obj.a_text + cround(size);
		break;

	/*
	 *	0430-- overlaid nonseparate I/D
	 */
	case A_MAGIC5:
		if (obj.a_text > KB(16)) {
			printf("base segment is %u, max is %u, too large by %u bytes.\n", obj.a_text, KB(16), obj.a_text - KB(16));
			errs++;
			texterrs++;
		}
		if (obj.a_text <= KB(8)) {
			printf("base segment is %u, minimum is %u, too small by %u bytes.\n", obj.a_text, KB(8), KB(8) - obj.a_text);
			errs++;
		}
		size = obj.a_data + obj.a_bss;
		if (size > KB(24)) {
			printf("data is %ld, max is %u, too large by %ld bytes.\n", size, KB(24), size - KB(24));
			errs++;
		}
		/*
		 *  Base and overlay 1 occupy 16K and 8K of physical
		 *  memory, respectively, regardless of actual size.
		 */
		totsize = KB(24) + cround(size);
		/*
		 *  Subtract the first overlay, it will be added below
		 *  and it has already been included.
		 */
		totsize -= ovlhdr.ov_siz[0];
		goto checkov;
		break;

	/*
	 *	0431-- overlaid separate I/D
	 */
	case A_MAGIC6:
		if (obj.a_text > KB(56)) {
			printf("base segment is %u, max is %u, too large by %u bytes.\n", obj.a_text, KB(56), obj.a_text - KB(56));
			errs++;
		}
		if (obj.a_text <= KB(48)) {
			printf("base segment is %u, min is %u, too small by %u bytes.\n", obj.a_text, KB(48), KB(48) - obj.a_text);
			errs++;
		}
		size = obj.a_data + obj.a_bss;
		if (size > KB(48)) {
			printf("data is %ld, max is %u, too large by %ld bytes.\n", size, KB(48), size - KB(48));
			errs++;
		}
		totsize = obj.a_text + cround(size);

checkov:
		for (i = 0;i < NOVL;i++) {
			totsize += ovlhdr.ov_siz[i];
			if (ovlhdr.ov_siz[i] > KB(8)) {
				printf("overlay %d is %u, max is %u, too large by %u bytes.\n", i + 1, ovlhdr.ov_siz[i], KB(8), ovlhdr.ov_siz[i] - KB(8));
				errs++;
				texterrs++;
			}
			/*
			 * check for zero length overlay in the middle of
			 * non-zero length overlays (boot croaks when faced
			 * with this) ...
			 */
			if (i < NOVL-1
			    && ovlhdr.ov_siz[i] == 0
			    && ovlhdr.ov_siz[i+1] > 0) {
				printf("overlay %d is empty and there are non-empty overlays following it.\n", i + 1);
				errs++;
				texterrs++;
			}
		}
		break;

	default:
		printf("magic number 0%o not recognized.\n", obj.a_magic);
		exit(-1);
	}

	(void)nlist(argv[1], nl);

	if (!nl[N_NINODE].n_type) {
		puts("\"ninode\" not found in namelist.");
		exit(-1);
	}
	ninode = getval(N_NINODE);
	if (!texterrs)
		{
		if (nl[N_PROC].n_value >= 0120000) {
			printf("The remapping area (0120000-0140000, KDSD5)\n\tcontains data other than the proc, text and file tables.\n\tReduce other data by %u bytes.\n", nl[N_PROC].n_value - 0120000);
			errs++;
			}
		}
	totsize += (getval(N_NBUF) * MAXBSIZE);
	if (nl[N_CLIST].n_value)
		totsize += cround(getval(N_CLIST) * (long)sizeof(struct cblock));
	if (nl[N_RAM].n_type)
		totsize += getval(N_RAM)*512;
	if (nl[N_QUOTDESC].n_type)
		totsize += 8192;
	if (nl[N_XITDESC].n_type)
		totsize += (ninode * 3 * sizeof (long));
	if (nl[N_NAMECACHE].n_type)
		totsize += (ninode * sizeof(struct namecache));
	if (nl[N_IOSIZE].n_type)
		totsize += getval(N_IOSIZE);
	if (nl[N_NLOG].n_type)
		totsize += (getval(N_NLOG) * MSG_BSIZE);
	totsize += ctob(USIZE);
	printf("System will occupy %ld bytes of memory (including buffers and clists).\n", totsize);
	for (i = 0; i < N_NUMSYMS; i++) {
		if (!(i % 3))
			putchar('\n');
		printf("\t%10.10s {0%06o}", nl[i].n_un.n_name+1, nl[i].n_value);
	}
	putchar('\n');
	if (errs)
		puts("**** SYSTEM IS NOT BOOTABLE. ****");
	exit(errs);
}

/*
 *  Get the value of an initialized variable from the object file.
 */
static long
getval(indx)
	int indx;
{
	register int novl;
	u_int ret;
	off_t offst;

	if ((nl[indx].n_type&N_TYPE) == N_BSS)
		return((long)0);
	offst = nl[indx].n_value;
	offst += obj.a_text;
	offst += sizeof(obj);
	if (obj.a_magic == A_MAGIC2 || obj.a_magic == A_MAGIC5)
		offst -= (off_t)round(obj.a_text);
	if (obj.a_magic == A_MAGIC5 || obj.a_magic == A_MAGIC6) {
		offst += sizeof ovlhdr;
		if (obj.a_magic == A_MAGIC5)
			offst -= (off_t)round(ovlhdr.max_ovl);
		for (novl = 0;novl < NOVL;novl++)
			offst += (off_t)ovlhdr.ov_siz[novl];
	}
	(void)lseek(fi, offst, L_SET);
	read(fi, &ret, sizeof(ret));
	return((long)ret);
}
