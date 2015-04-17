/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)boot.c	3.0 (2.11BSD) 1996/5/9
 */
#include "../h/param.h"
#include "../machine/seg.h"
#include "../machine/koverlay.h"
#include "../h/reboot.h"
#include "saio.h"
#include <a.out.h>

#undef	btoc			/* to save space */
#define	KB	* 1024L

#define	KISD0	((u_short *) 0172300)
#define	KISD3	((u_short *) 0172306)
#define	KDSD0	((u_short *) 0172320)
#undef	KISA0
#define	KISA0	((u_short *) 0172340)
#define	KISA3	((u_short *) 0172346)
#define	KDSA0	((u_short *) 0172360)

#define	SEG_DATA	01
#define	SEG_TEXT	02
#define	SEG_OVLY	04

extern	caddr_t	*bootcsr;	/* csr of boot controller */
extern	int	bootctlr;	/* boot controller number */
extern	int	bootopts;	/* boot options from previous incarnation */
extern	int	bootdev;	/* makedev(major,unit) booted from */
extern	int	checkword;	/* one's complements of bootopts */
extern	int	cputype;	/* 24, 40, 44, 45, 70, or 73 */
extern	bool_t	ksep;		/* is kernel mode currently separated */
extern	bool_t	sep_id;		/* does the cpu support separate I/D? */
extern	int	ndevsw;		/* number of devices in devsw[] */
extern	char	ADJcsr[];	/* adjustments for ROM csr addresses */
extern	char	*itoa();
extern	char	*index();
extern	struct	devsw devsw[];	/* device table */
extern	struct	iob	iob[];	/* I/O descriptor table */

char		module[] = "Boot"; /* this program's name (used by trap) */
bool_t		overlaid = 0;
u_short		pdrproto[16 + NOVL] = {0};
struct exec	exec;
struct ovlhdr	ovlhdr;
int		bootdebug;
unsigned	btoc();

struct	loadmap {
	int	seg_type;
	long	seg_len;
};
struct	loadtable {
	short	lt_magic;
	struct	loadmap	*lt_map;
};

struct	loadmap	load407[] = {
	SEG_DATA,	56 KB,
	0,		0  KB
};
struct	loadmap	load410[] = {
	SEG_TEXT,	48 KB,
	SEG_DATA,	56 KB,
	0,		0  KB
};
struct	loadmap	load411[] = {
	SEG_DATA,	56 KB,
	SEG_TEXT,	64 KB,
	0,		0  KB
};
struct	loadmap	load430[] = {
	SEG_TEXT,	16 KB,			/* minumum, 8 KB + 1 */
	SEG_OVLY,	8  KB,	/*  1 */
	SEG_DATA,	24 KB,
	SEG_OVLY,	8  KB,	/*  2 */
	SEG_OVLY,	8  KB,	/*  3 */
	SEG_OVLY,	8  KB,	/*  4 */
	SEG_OVLY,	8  KB,	/*  5 */
	SEG_OVLY,	8  KB,	/*  6 */
	SEG_OVLY,	8  KB,	/*  7 */
	SEG_OVLY,	8  KB,	/*  8 */
	SEG_OVLY,	8  KB,	/*  9 */
	SEG_OVLY,	8  KB,	/* 10 */
	SEG_OVLY,	8  KB,	/* 11 */
	SEG_OVLY,	8  KB,	/* 12 */
	SEG_OVLY,	8  KB,	/* 13 */
	SEG_OVLY,	8  KB,	/* 14 */
	SEG_OVLY,	8  KB,	/* 15 */
	0,		0  KB
};
struct	loadmap	load431[] = {
	SEG_DATA,	56 KB,			/* minumum, 48 KB + 1 */
	SEG_TEXT,	56 KB,
	SEG_OVLY,	8  KB,	/*  1 */
	SEG_OVLY,	8  KB,	/*  2 */
	SEG_OVLY,	8  KB,	/*  3 */
	SEG_OVLY,	8  KB,	/*  4 */
	SEG_OVLY,	8  KB,	/*  5 */
	SEG_OVLY,	8  KB,	/*  6 */
	SEG_OVLY,	8  KB,	/*  7 */
	SEG_OVLY,	8  KB,	/*  8 */
	SEG_OVLY,	8  KB,	/*  9 */
	SEG_OVLY,	8  KB,	/* 10 */
	SEG_OVLY,	8  KB,	/* 11 */
	SEG_OVLY,	8  KB,	/* 12 */
	SEG_OVLY,	8  KB,	/* 13 */
	SEG_OVLY,	8  KB,	/* 14 */
	SEG_OVLY,	8  KB,	/* 15 */
	0,		0  KB
};

struct	loadtable	loadtable[] = {
	A_MAGIC1,	load407,
	A_MAGIC2,	load410,
	A_MAGIC3,	load411,
	A_MAGIC5,	load430,
	A_MAGIC6,	load431
};

main()
{
	register int i, j, maj;
	int retry = 0, unit, part;
	caddr_t	*adjcsr;
	struct loadtable *setup();
	struct iob *file;
	char	*cp, *defname = "unix", line[64], defdev[64];

	maj = major(bootdev);
	if (maj >= ndevsw)
		_stop("bad major");		/* can't happen */
	adjcsr = (caddr_t *)((short)bootcsr - ADJcsr[maj]);

	for (i = 0; devsw[maj].dv_csr != (caddr_t) -1; i++) {
		if (adjcsr == devsw[maj].dv_csr[i])
			break;
		if (devsw[maj].dv_csr[i] == 0) {
			devsw[maj].dv_csr[i] = adjcsr;
			break;
		}
	}

	if (devsw[maj].dv_csr[i] == (caddr_t *) -1)
		_stop("no free csr slots");
	bootdev &= ~(3 << 6);
	bootdev |= (i << 6);	/* controller # to bits 6&7 */
	bootctlr = i;
	unit = (minor(bootdev) >> 3) & 7;
	part = minor(bootdev) & 7;

	printf("\n%d%s from %s(%d,%d,%d) at 0%o\n", cputype, module, 
		devsw[major(bootdev)].dv_name, bootctlr, unit, part, bootcsr);

	strcpy(defdev, devsw[major(bootdev)].dv_name);
	strcat(defdev, "(");
	strcat(defdev, itoa(bootctlr));
	strcat(defdev, ",");
	strcat(defdev, itoa(unit));
	strcat(defdev, ",");
	strcat(defdev, itoa(part));
	strcat(defdev, ")");

	/*
	 * The machine language will have gotten the bootopts
	 * if we're an autoboot and will pass them along.
	 * If r2 (checkword) was the complement of bootopts,
	 * this is an automatic reboot, otherwise do it the hard way.
	 */
	if (checkword != ~bootopts)
		bootopts = RB_SINGLE | RB_ASKNAME;
	j = -1;
	do {
		if (bootopts & RB_ASKNAME) {
another:
			printf(": ");
			gets(line);
			cp = line;
			if	(*cp == '-')
				{
				dobootopts(cp, &bootopts);
				goto another;
				}
		} else {
			strcpy(line, defdev);
			strcat(line, defname);
			printf(": %s\n", line);
		}
		if (line[0] == '\0') {
			strcpy(line, defdev);
			strcat(line, defname);
			printf(": %s\n", line);
		}
/*
 * If a plain filename (/unix) is entered then prepend the default
 * device, e.g. ra(0,1,0) to the filename.
*/
		cp = index(line, ')');
		if	(!cp)
			{
			bcopy(line, line + strlen(defdev), strlen(line) + 1);
			bcopy(defdev, line, strlen(defdev));
			}
		j = -1;
		if	(cp = index(line, ' '))
			{
			if	((bootflags(cp, &bootopts, "bootfile")) == -1)
				{
				bootopts |= RB_ASKNAME;
				continue;
				}
			*cp = '\0';
			}
		i = open(line, 0);
		if (i >= 0) {
			file = &iob[i - 3];	/* -3 for pseudo stdin/o/e */
			j = checkunix(i, setup(i));
			(void) close(i);
		}
		if (++retry > 2)
			bootopts = RB_SINGLE | RB_ASKNAME;
	} while (j < 0);
	i = file->i_ino.i_dev;
	bootdev = makedev(i, 
		((file->i_ctlr << 6) | (file->i_unit << 3) | file->i_part));
	bootcsr = devsw[i].dv_csr[file->i_ctlr];
	bootcsr = (caddr_t *)((short)bootcsr + ADJcsr[i]);
	printf("%s: bootdev=0%o bootcsr=0%o\n", module, bootdev, bootcsr);
}

struct loadtable *
setup(io)
	register io;
{
	register i;

	exec.a_magic = getw(io);
	exec.a_text = (unsigned) getw(io);
	exec.a_data = (unsigned) getw(io);
	exec.a_bss = (unsigned) getw(io);

	/*
	 * Space over the remainder of the exec header.  We do this
	 * instead of seeking because the input might be a tape which
	 * doesn't know how to seek.
	 */
	getw(io); getw(io); getw(io); getw(io);

	/*
	 * If overlaid, get overlay header.
	 */
	if (exec.a_magic == A_MAGIC5 || exec.a_magic == A_MAGIC6) {
		overlaid++;
		ovlhdr.max_ovl = getw(io);
		for (i = 0; i < NOVL; i++)
			ovlhdr.ov_siz[i] = (unsigned) getw(io);
	}
	for (i = 0; i < sizeof(loadtable) / sizeof(struct loadtable); i++)
		if (loadtable[i].lt_magic == exec.a_magic)
			return(&loadtable[i]);
	printf("Bad magic # 0%o\n", exec.a_magic);
	return((struct loadtable *) NULL);
}

checkunix(io, lt)
	struct loadtable *lt;
{
	char *segname, *toosmall = "Base too small, %dK min\n";
	register int ovseg, segtype;
	register unsigned seglen;
	struct loadmap *lm = lt->lt_map;

	if (lt == (struct loadtable *) NULL)
		return(-1);

	/*
	 * Check and set I & D space requirements.
	 */
	if (exec.a_magic == A_MAGIC3 || exec.a_magic == A_MAGIC6)
		if (!sep_id) {
			printf("Can't load split I&D files\n");
			return(-1);
		} else
			setsep();
	else
		if (sep_id)
			setnosep();

	/*
	 * Check the sizes of each segment.
	 */
	ovseg = 0;
	while (segtype = lm->seg_type) {
		switch (segtype) {
			case SEG_TEXT:
				/*
				 * Round text size to nearest page.
				 */
				if (exec.a_magic == A_MAGIC2)
					seglen = ctob(stoc(ctos(btoc(exec.a_text))));
				else
					seglen = exec.a_text;
				segname = "Text";
				break;

			case SEG_DATA:
				seglen = exec.a_data + exec.a_bss;
				segname = "Data";
				if (exec.a_magic == A_MAGIC1)
					seglen += exec.a_text;
				else
					/*
					 * Force a complaint if the file
					 * won't fit.  It's here instead
					 * of in the SEG_TEXT case above
					 * because it's more likely to be
					 * a data overflow problem.
					 */
					if (exec.a_magic == A_MAGIC2)
						seglen += ctob(stoc(ctos(btoc(exec.a_text))));
				break;

			case SEG_OVLY:
				seglen = ovlhdr.ov_siz[ovseg];
				segname = "Overlay";
				ovseg++;
				break;

			default:
				/*
				 * This ``cannot happen.''
				 */
				printf("seg type botch: %d\n", segtype);
				return(-1);
				/*NOTREACHED*/
		}

		seglen = ctob(btoc(seglen));
		if (((long) seglen) > lm->seg_len) {
			if (segtype == SEG_OVLY)
				printf("%s %d over by %D bytes", segname, ovseg, lm->seg_len -((long) seglen));
			else
				printf("%s over by %D bytes", segname, lm->seg_len -((long) seglen));
			return(-1);
		}
		if (segtype == SEG_TEXT)
		    switch (exec.a_magic) {
			case A_MAGIC5:
			    if (seglen <= 8 KB) {
				printf(toosmall, 8);
				return(-1);
			    }
			    break;
			case A_MAGIC6:
			    if (seglen <= 48 KB) {
				printf(toosmall, 48);
				return(-1);
			    }
			    break;
			default:
			    break;
		    }
		    lm++;
	}
	copyunix(io, lt);
	setregs(lt);
	return(0);
}

copyunix(io, lt)
	register io;
	struct loadtable *lt;
{
	int i;
	bool_t donedata = 0;
	register int addr;
	register unsigned seglen;
	off_t segoff;
	int segtype;
	int nseg, phys, ovseg;
	struct loadmap *lm = lt->lt_map;

	/*
	 * Load the segments and set up prototype PDRs.
	 */
	nseg = 0;
	phys = 0;
	ovseg = 0;
	lm = lt->lt_map;
	while (segtype = lm++->seg_type) {
		segoff = (off_t) N_TXTOFF(exec);
		switch (segtype) {
			case SEG_TEXT:
				seglen = exec.a_text;
				break;

			case SEG_DATA:
				seglen = exec.a_data;
				/*
				 * If this is a 0407 style object, the text
				 * and data are loaded together.
				 */
				if (exec.a_magic != A_MAGIC1) {
					segoff += (off_t) exec.a_text;
					if (overlaid)
						for (i = 0; i < NOVL; i++)
							segoff += (off_t) ovlhdr.ov_siz[i];
				} else
					seglen += exec.a_text;
				donedata++;
				break;

			case SEG_OVLY:
				seglen = ovlhdr.ov_siz[ovseg];
				segoff += (off_t) exec.a_text;
				for (i = 0; i < ovseg; i++)
					segoff += (off_t) ovlhdr.ov_siz[i];
				ovseg++;
				break;
			default:
				printf("copyunix: bad seg type %d\n", segtype);
				seglen=0;
				break;
		}

		if (!seglen)
			continue;
		setseg(phys);
/*
 * ARGH!  Despite (or in spite of) the earlier cautions against seeking and
 * tape devices here is an 'lseek' that caused problems loading split I/D
 * images from tape!
*/
		if (exec.a_magic != A_MAGIC1)
			(void) lseek(io, segoff, 0);
		for (addr = 0; addr < seglen; addr += 2)
			mtpi(getw(io), addr);

		if (segtype == SEG_DATA) {
			clrseg(addr, exec.a_bss);
			seglen += exec.a_bss;
		}

		pdrproto[nseg++] = btoc(seglen);
		if (!donedata)
			seglen = ctob(stoc(ctos(btoc(seglen))));
		phys += btoc(seglen);
	}
}

/*
 * Set the real segmentation registers.
 */
setregs(lt)
	struct loadtable *lt;
{
	register i;
	register u_short *par_base, *pdr_base;
	bool_t donedata = 0;
	int phys, segtype;
	int nseg, ntextpgs, novlypgs, npages, pagelen;
	struct loadmap *lm = lt->lt_map;

	nseg = 0;
	phys = 0;
	ntextpgs = 0;
	novlypgs = 0;

	setseg(0);
	if (exec.a_magic == A_MAGIC1)
		return;

	/*
	 * First deny access to all except I/O page.
	 */
	par_base = KISA0;
	pdr_base = KISD0;
	for (i = 0; i <(ksep ?  8 : 7); i++) {
		*par_base++ = 0;
		*pdr_base++ = NOACC;
	}
	if (ksep) {
		par_base = KDSA0;
		pdr_base = KDSD0;
		for (i = 0; i < 7; i++) {
			*par_base++ = 0;
			*pdr_base++ = NOACC;
		}
	}
	if (overlaid) {
		/*
		 * We must write the prototype overlay register table.
		 * N.B.:  we assume that the table lies in the first 8k
		 * of kernel virtual space, and the appropriate page lies
		 * at physical 0.
		 */
		if (ksep)
			*KDSD0 = ((128 -1) << 8) | RW;
		else
			*KISD0 = ((128 -1) << 8) | RW;
		par_base = &(((u_short *) OVLY_TABLE_BASE)[0]);
		pdr_base = &(((u_short *) OVLY_TABLE_BASE)[1 + NOVL]);
		for (i = 0; i < NOVL; i++) {
			mtpd(0, par_base++);
			mtpd(NOACC, pdr_base++);
		}
	}

	/*
	 * Now set all registers which should be nonzero.
	 */
	lm = lt->lt_map;
	while (segtype = lm++->seg_type) {
		if (!(npages = ctos(pdrproto[nseg])))
			continue;

		switch (segtype) {
			case SEG_TEXT:
				/*
				 * Text always starts at KI0;
				 */
				par_base = KISA0;
				pdr_base = KISD0;
				ntextpgs += npages;
				break;

			case SEG_DATA:
				if (overlaid)
					if (ksep) {
						par_base = I_DATA_PAR_BASE;
						pdr_base = I_DATA_PDR_BASE;
					} else {
						par_base = N_DATA_PAR_BASE;
						pdr_base = N_DATA_PDR_BASE;
					}
				else
					if (ksep) {
						par_base = KDSA0;
						pdr_base = KDSD0;
					} else {
						par_base = &KISA0[ntextpgs + novlypgs];
						pdr_base = &KISD0[ntextpgs + novlypgs];
					}
				donedata++;
				break;

			case SEG_OVLY:
				par_base = &(((u_short *) OVLY_TABLE_BASE)[1 + novlypgs]);
				pdr_base = &(((u_short *) OVLY_TABLE_BASE)[1 + NOVL + 1 + novlypgs]);
				novlypgs += npages;
				break;
		}

		for (i = 0; i < npages; i++) {
			pagelen = MIN(btoc((int)(8 KB)), pdrproto[nseg]);
			if (segtype == SEG_OVLY) {
				mtpd(phys, par_base);
				mtpd(((pagelen - 1) << 8) | RO, pdr_base);
			} else {
				*par_base = phys;
				if (segtype == SEG_TEXT)
					if (ksep)
						*pdr_base = ((pagelen - 1) << 8) | RO;
					else
						/*
						 * Nonseparate kernels will
						 * write into text page 0
						 * when first booted.
						 */
						if (i == 0)
							*pdr_base = ((pagelen - 1) << 8) | RW;
						else
							*pdr_base = ((pagelen - 1) << 8) | RO;
				else
					*pdr_base = ((pagelen - 1) << 8) | RW;
			}
			par_base++, pdr_base++;
			if (donedata)
				phys += pagelen;
			else
				phys += stoc(ctos(pagelen));
			pdrproto[nseg] -= pagelen;
		}
		nseg++;
	}

	/*
	 * Phys now contains the address of the start of
	 * free memory.  We set K[ID]6 now or systrap to
	 * kernel mode will clobber text at 0140000.
	 */
	if (ksep) {
		KDSA0[6] = phys;
		KDSD0[6] = (stoc(1) - 1) << 8 | RW;
	} else {
		KISA0[6] = phys;
		KISD0[6] = (stoc(1) - 1) << 8 | RW;
	}
	if (overlaid)
		mtpd(phys, &(((u_short *) OVLY_TABLE_BASE)[1 + NOVL + 1 + NOVL]));
}

unsigned
btoc(nclicks)
	register unsigned nclicks;
{
	return((unsigned)(((((long) nclicks) + ((long) 63)) >> 6)));
}

/*
 * Couldn't use getopt(3) for a couple reasons:  1) because that would end up 
 * dragging in way too much of libc.a, and 2) the code to build argc
 * and argv would be almost as large as the parsing routines themselves.
*/

char *
arg(cp)
	register char *cp;
	{

	if	((cp = index(cp, ' ')) == NULL)
		return(NULL);
	while	(*cp == ' ' || *cp == '\t')
		cp++;
	if	(*cp == '\0')
		return(NULL);
	return(cp);
	}

/*
 * Flags to boot may be present in two places.  1) At the ': ' prompt enter
 * a line starting with "-bootflags".  2) After the filename.  For example, 
 * to turn on the autoconfig debug flag:
 *
 * : -bootflags -D
 *
 * To force the kernel to use the compiled in root device (which also affects
 * swapdev, pipedev and possibly dumpdev):
 *
 * : -bootflags -R
 *
 * To specify flags on the filename line place the options after the filename:
 *
 * : ra(0,0)unix -D -s
 *
 * will cause the kernel to use the compiled in root device (rather than auto
 * matically switching to the load device) and enter single user mode.
 *
 * Bootflags may also be specified as a decimal number (you will need the
 * sys/reboot.h file to look up the RB_* flags in).  Turning all bootflags off
 * is the special case:
 *
 * : -bootflags 0
 *
 * There is a general purpose 'debug' flag word ("bootdebug") which can be
 * set to any arbitrary 16 bit value.  This can be used when debugging a 
 * driver for example.
 *
 * : -bootdebug 16
*/

#define	BOOTFLAGS	"-bootflags"
#define	BOOTDEBUG	"-bootdebug"

dobootopts(cp, opt)
	register char *cp;
	int *opt;
	{
	char	*bflags = BOOTFLAGS;
	char	*bdebug = BOOTDEBUG;

	if	(strncmp(cp, bdebug, sizeof (BOOTDEBUG) - 1) == 0)
		{
		if	(cp = arg(cp))
			bootdebug = atoi(cp);
		else
			printf("%s = %u\n", bdebug, bootdebug);
		return(0);
		}
	if	(strncmp(cp, bflags, sizeof (BOOTFLAGS) - 1) == 0)
		{
		if	(cp = arg(cp))
			(void) bootflags(cp, &bootopts, bflags);
		else
			printf("%s = %u\n", bflags, bootopts);
		return(0);
		}
	printf("bad cmd: %s\n", cp);
	return(0);
	}

bootflags(cp, pflags, tag)
	register char *cp;
	int *pflags;
	char *tag;
	{
	int first = 1;
	int flags = 0;

	while	(*cp)
		{
		while	(*cp == ' ')
			cp++;
		if	(*cp == '\0')
			break;
		if	(*cp == '-')
			{
			first = 0;
			while	(*++cp)
				switch	(*cp)
					{
					case	' ':
						goto nextarg;
					case	'a':
						flags |= RB_ASKNAME;
						break;
					case	'D':
						flags |= RB_AUTODEBUG;
						break;
					case	'r':
						flags |= RB_RDONLY;
						break;
					case	'R':
						flags |= RB_DFLTROOT;
						break;
					case	's':
						flags |= RB_SINGLE;
						break;
					default:
						goto usage;
					}
				continue;
			}
		if	(first && *cp >= '0' && *cp <= '9')
			{
			*pflags = atoi(cp);
			return(0);
			}
		goto usage;

nextarg: 	;
		}
	if	(first == 0)
		*pflags = flags;
	return(0);
usage:
	printf("usage: %s [ -aDrRs ]\n", tag);
	return(-1);
	}
