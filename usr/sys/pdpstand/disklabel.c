/*
 * Public domain, June 1995
 *
 *	@(#)disklabel.c	1.4 (2.11BSD GTE) 1996/5/2
*/

#define	DKTYPENAMES
#include "../h/param.h"
#include "saio.h"

	int	Nolabelerr = 1;		/* Inhibit spurious label error msgs */
	char	module[] = "disklabel";	/* This program's name */
	char	line[80], device[80];

extern	long	atol();
extern	struct	iob	iob[];
extern	struct	devsw	devsw[];

main()
	{
	register int	f;
	register struct	disklabel *lp;
	struct	iob	*io;

	printf("%s\n", module);

	while	(1)
		{
		printf("Disk? ");
		gets(line);
		if	(!line[0])
			continue;
		strcpy(device, line);
		f = open(device, F_WRITE);
		if	(f < 0)
			{
			printf("Error opening '%s' for writing\n", device);
			continue;
			}
		io = &iob[f - 3];
		if	(io->i_flgs & F_TAPE)
			{
			printf("Can not label tapes.\n");
			continue;
			}
		break;
		}
/*
 * The open() will have retrieved the label sector.  This explicit read
 * is for debugging and testing because the driver's open routine may be
 * under development and the automatic retrieval of the label is commented
 * out.
*/
	if	(devlabel(io, READLABEL) < 0)
		{
		printf("Can not read label sector from '%s'\n", device);
		return;		/* back to Boot */
		}
	lp = &io->i_label;
	if	(lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
		 dkcksum(lp))
		{
		printf("'%s' is unlabeled or the label is corrupt.\n", device);
		printf("Proceed? [y/n] ");
		if	(gyon() == 'n')
			return;		/* back to Boot */
/*
 * We next call the driver's entry which attempts to identify the drive
 * and set up a default (1 partition) label with the best guess as to
 * geometry, etc.  If this call fails then the driver does not support
 * labels ('nullsys' is being used in the device table).
*/
		if	(devlabel(io, DEFAULTLABEL) < 0)
			{
			printf("The '%s' driver does not support labels.\n",
				devsw[io->i_ino.i_dev].dv_name);
			return;		/* back to Boot */
			}
		}
	doit(io);
	return;
	}

doit(io)
	register struct iob *io;
	{
	int	c, dirty = 0;
	struct	disklabel *lp = &io->i_label;

	while	(1)
		{
		printf("d(isplay) D(efault) m(odify) w(rite) q(uit)? ");
		c = egetchar("dDmwq");
		switch	(c)
			{
			case	'm':
				modifylabel(lp);
				overlapchk(lp);
				(void)checklabel(lp);
/*
 * We make the assumption that the label was modified - the dirty
 * flag is set so we can ask if the changes should be discarded on a 'q'.
*/
				dirty = 1;
				break;
			case	'w':
				overlapchk(lp);
				(void)checklabel(lp);
				devlabel(io, WRITELABEL);
/*
 * Changed label was committed to disk so we can clear the dirty flag now
 * and not ask the user if the changes should be kept when a 'q'uit is done.
*/
				dirty = 0;
				break;
			case	'd':
				displaylabel(lp);
				break;
			case	'D':
				devlabel(io, DEFAULTLABEL);
				dirty = 1;
				break;
			case	'q':
				if	(!dirty)
					return;
				printf("Label changed. Discard changes [y/n]? ");
				if	(gyon() == 'n')
					break;
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

modifylabel(lp)
	register struct disklabel *lp;
	{
	register int	c;

	while	(1)
		{
		printf("modify\n");
		printf("d(isplay) g(eometry) m(isc) p(artitions) q(uit)? ");
		c = egetchar("dgmpq");
		switch	(c)
			{
			case	'd':
				displaylabel(lp);
				break;
			case	'g':
				dogeometry(lp);
				break;
			case	'm':
				domisc(lp);
				break;
			case	'p':
				dopartitions(lp);
				break;
			case	'q':
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

dogeometry(lp)
	struct	disklabel *lp;
	{
	register int c;

	while	(1)
		{
		printf("modify geometry\n");
		printf("d(isplay) s(ector/trk) t(rk/cyl) c(yl) S(ector/cyl) q(uit)? ");
		c = egetchar("dstcSq");
		switch	(c)
			{
			case	'd':
				displaylabel(lp);
				break;
			case	's':
				fillin_int(&lp->d_nsectors,
					"sectors/track [%d]: ", 512);
				break;
			case	't':
				fillin_int(&lp->d_ntracks,
					"tracks/cylinder [%d]: ", 127);
				break;
			case	'c':
				fillin_int(&lp->d_ncylinders,
					"cylinders [%d]: ", 4096);
				break;
			case	'S':
				lp->d_secpercyl= lp->d_nsectors * lp->d_ntracks;
				fillin_int(&lp->d_secpercyl,
					"sectors/cylinder [%d]: ", 65535);
				break;
			case	'q':
				if	(lp->d_secpercyl == 0)
					lp->d_secpercyl = lp->d_nsectors *
							lp->d_ntracks;
				lp->d_secperunit = (long)lp->d_ncylinders * 
							lp->d_secpercyl;
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

domisc(lp)
	register struct disklabel *lp;
	{
	register int c;
	char	junk[32];

	while	(1)
		{
		printf("modify misc\n");
		printf("d(isplay) t(ype) n(ame) l(abel) f(lags) r(pm) D(rivedata) q(uit)? ");
		c = egetchar("dtnlfrDq");
		switch	(c)
			{
			case	'd':
				displaylabel(lp);
				break;
			case	't':
				if	(lp->d_type >= DKMAXTYPES)
					lp->d_type = 0;
				printf("type [%s]: ", dktypenames[lp->d_type]);
				gets(line);
				if	(line[0])
					lp->d_type = findtype(dktypenames,
								line,
								lp->d_type);
				break;
			case	'n':
				strncpy(junk, lp->d_typename,
						sizeof (lp->d_typename));
				junk[sizeof (lp->d_typename)] = '\0';
				printf("disk [%s]: ", junk);
				gets(line);
				if	(line[0])
					strncpy(lp->d_typename, line,
						sizeof (lp->d_typename));
				break;
			case	'l':
				strncpy(junk, lp->d_packname,
						sizeof (lp->d_packname));
				junk[sizeof (lp->d_packname)] = '\0';
				printf("label [%s]: ", junk);
				gets(line);
				if	(line[0])
					strncpy(lp->d_packname, line,
						sizeof (lp->d_packname));
				break;
			case	'f':
				doflags(lp);
				break;
			case	'r':
				fillin_int(&lp->d_rpm, "rpm [%d]: ", 32767);
				break;
			case	'D':
				dodrivedata(lp);
				break;
			case	'q':
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

dodrivedata(lp)
	struct disklabel *lp;
	{
	register u_long *ulp;
	register int	i;

	for	(i = 0, ulp = lp->d_drivedata; i < NDDATA; i++, ulp++)
		{
		printf("modify misc drivedata\n");
		printf("drivedata #%d [%D]: ", i, *ulp);
		gets(line);
		if	(line[0])
			*ulp = atol(line);
		}
	return;
	}

doflags(lp)
	register struct disklabel *lp;
	{
	register int	c;

	while	(1)
		{
		printf("modify misc flags\n");
		printf("d(isplay) c(lear) e(cc) b(adsect) r(emovable) q(uit)? ");
		c = egetchar("dcerbq");
		switch	(c)
			{
			case	'c':
				lp->d_flags = 0;
				break;
			case	'd':
				displaylabel(lp);
				break;
			case	'r':
				lp->d_flags |= D_REMOVABLE;
				break;
			case	'e':
				lp->d_flags |= D_ECC;
				break;
			case	'b':
				lp->d_flags |= D_BADSECT;
				break;
			case	'q':
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

dopartitions(lp)
	struct	disklabel *lp;
	{
	register int	c;
	int	i;

	while	(1)
		{
		printf("modify partitions\n");
		printf("d(isplay) n(umber) s(elect) q(uit)? ");
		c = egetchar("dsnq");
		switch	(c)
			{
			case	'd':
				displaylabel(lp);
				break;
			case	'n':
				printf("Number of partitions (%d max) [%d]? ",
					MAXPARTITIONS, lp->d_npartitions);
				gets(line);
				if	(line[0] == '\0')
					i = lp->d_npartitions;
				else
					i = atoi(line);
				if	(i > 0 && i <= MAXPARTITIONS)
					lp->d_npartitions = i;
				break;
			case	's':
				printf("a b c d e f g h q(uit)? ");
				i = getpartnum();
				if	(i < 0)
					break;
				if	(i >= lp->d_npartitions)
					lp->d_npartitions = i + 1;
				dopartmods(lp, i);
				break;
			case	'q':
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

dopartmods(lp, part)
	struct	disklabel *lp;
	int	part;
	{
	char	pname = 'a' + part;
	int	i, c;
	register struct partition *pp = &lp->d_partitions[part];
	u_int	cyl;
	daddr_t	off, sec, size;

	printf("sizes and offsets may be given as sectors, cylinders\n");
	printf("or cylinders plus sectors:  6200, 32c, 19c10s respectively\n");

	while	(1)
		{
		printf("modify partition '%c'\n", pname);
		printf("d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? ");
		c = egetchar("dztosfFq");
		switch	(c)
			{
			case	'z':
				pp->p_size = 0;
				pp->p_offset = 0;
				pp->p_fstype = FS_UNUSED;
				break;
			case	'd':
				displaylabel(lp);
				break;
			case	't':
				if	(pp->p_fstype >= FSMAXTYPES)
					pp->p_fstype = FS_UNUSED;
				printf("'%c' fstype [%s]: ", pname,
					fstypenames[pp->p_fstype]);
				gets(line);
				if	(line[0])
					pp->p_fstype = findtype(fstypenames,
								line,
								pp->p_fstype);
				break;
			case	'o':
				printf("'%c' offset [%D]: ",pname,pp->p_offset);
				gets(line);
				if	(line[0] == '\0')
					break;
				i = parse_sec_cyl(lp, line, &sec, &cyl);
				if	(i < 0)
					break;
				if	(cyl)
					off = (long)lp->d_secpercyl * cyl;
				else
					off = 0;
				off += sec;
				pp->p_offset = off;
				break;
			case	's':
				printf("'%c' size [%D]: ", pname, pp->p_size);
				gets(line);
				if	(line[0] == '\0')
					break;
				i = parse_sec_cyl(lp, line, &sec, &cyl);
				if	(i < 0)
					break;
				if	(cyl)
					size = (long)lp->d_secpercyl * cyl;
				else
					size = 0;
				size += sec;
				pp->p_size = size;
				break;
			case	'f':
				printf("'%c' frags/fs-block [1]: ", pname);
				gets(line);
				if	(line[0] == '\0')
					break;
				i = atoi(line);
				if	(i <= 0 || i > 255)
					{
					printf("frags/block <= 0 || > 255\n");
					pp->p_frag = 1;
					}
				else
					pp->p_frag = i;
				break;
			case	'F':	/* Not really used at the present */
				printf("'%c' frag size [1024]: ", pname);
				gets(line);
				if	(line[0] == '\0')
					break;
				i = atoi(line);
				if	(i <= 0 || (i % NBPG))
					{
					printf("fragsize <= 0 || ! % NBPG\n");
					pp->p_fsize = 0;
					}
				else
					pp->p_fsize = i;
				break;
			case	'q':
				if	(pp->p_fsize == 0)
					pp->p_fsize = 1024;
				if	(pp->p_frag == 0)
					pp->p_frag = 1;
				return;
			default:
				break;
			}
		}
/* NOTREACHED */
	}

getpartnum()
	{
	register int c;
	
	c = egetchar("abcdefghq");
	if	(c == 'q')
		return(-1);
	return(c - 'a');
	}

findtype(list, name, deflt)
	char	**list;
	char	*name;
	int	deflt;
	{
	register char **cpp;
	int	i;

	for	(i = 0, cpp = list; *cpp; cpp++, i++)
		{
		if	(strcmp(*cpp, name) == 0)
			return(i);
		}
	printf("%s not found in list.  The possible choices are:\n\n", name);
	for	(cpp = list; *cpp; cpp++)
		printf("  %s\n", *cpp);
	putchar('\n');
	return(deflt);
	}

/*
 * Sizes and offsets can be specified in four ways:
 *
 *    Number of sectors:  32678
 *    Number of cylinders:  110c
 *    Number of cylinders and sectors:  29c14s
 *    Number of sectors and cylinders:  22s134c
 * 
 * The trailing 's' or 'c' can be left off in the last two cases.
 *
 * The geometry section of the label must have been filled in previously.
 * A warning is issued if the cylinder or cylinder+sector forms are used
 * and the necessary geometry information is not present.
*/

parse_sec_cyl(lp, line, sec, cyl)
	struct	disklabel *lp;
	char	line[];
	daddr_t	*sec;
	u_int	*cyl;
	{
	register char	*cp;
	int	error = 0;
	long	tmp, tmpcyl = 0, tmpsec = 0;

	for	(tmp = 0, cp = line; *cp; cp++)
		{
		if	(*cp >= '0' && *cp <= '9')
			{
			tmp *= 10;
			tmp += (*cp - '0');
			}
		else if	(*cp == 'c')
			{
			if	(tmpcyl)
				{
				printf("duplicate 'c'ylinder specified\n");
				error = 1;
				break;
				}
			tmpcyl = tmp;
			tmp = 0;
			}
		else if	(*cp == 's')
			{
			if	(tmpsec)
				{
				printf("duplicate 's'ector specified\n");
				error = 1;
				break;
				}
			tmpsec = tmp;
			tmp = 0;
			}
		else
			{
			printf("illegal character '%c'\n", *cp);
			error = 1;
			break;
			}
		}
	if	(error)
		return(-1);

/*
 * At this point if either a 's' or 'c' was encountered in the string then
 * one or both of 'tmpsec' and 'tmpcyl' will be non-zero.  If the trailing 
 * character was omitted we need to figure out which variable gets the 
 * contents left in 'tmp' when the terminating null character was seen. This
 * is because "15c8" and "18s3" are both valid and indicate "15 cylinders +
 * 8 sectors" and "18 sectors + 3 cylinders" respectively.
 *
 * If neither 'tmpsec' or 'tmpcyl' are nonzero then we have a simple sector
 * number in 'tmp'.
*/
	if	(tmpsec || tmpcyl)
		{
		if	(tmpsec)
			tmpcyl = tmp;
		else
			tmpsec = tmp;
		}
	else
		{
		tmpsec = tmp;
		tmpcyl = 0;
		}
/*
 * It is an error condition to specify a number of cylinders and not
 * have previously defined the geometry - it is impossible to calculate
 * the number of sectors in the partition without geometry.
*/
	if	(tmpcyl && lp->d_secpercyl == 0)
		{
		printf("# cylinders specified but no geometry info present!\n");
		return(-1);
		}

/*
 * Sanity check to make sure erroneous number of cylinders is not believed
 * due to truncation (number of cylinders is really a 'u_int')
*/

	if	(tmpcyl > 65535L)
		{
		printf("Number of cylinders specified (%D) is ridiculous!\n",
			tmpcyl);
		return(-1);
		}
	*cyl = (u_int)tmpcyl;
	*sec = tmpsec;
	return(0);
	}

fillin_int(where, fmt, limit)
	u_int	*where;
	char	*fmt;
	u_int	limit;
	{
	u_int	i;

	printf(fmt, *where);
	gets(line);
	if	(line[0])
		{
		i = (u_int)atoi(line);
		if	(i > limit)
			{
			printf("%d is out of bounds (> %d)\n", i, limit);
			return;
			}
		*where = i;
		}
	}

fillin_long(where, fmt, limit)
	u_long	*where;
	char	*fmt;
	u_long	limit;
	{
	u_long	l;

	printf(fmt, *where);
	gets(line);
	if	(line[0])
		{
		l = (u_int)atol(line);
		if	(l > limit)
			{
			printf("%D is out of bounds (> %D)\n", l, limit);
			return;
			}
		*where = l;
		}
	}

gyon()
	{
	register int	c;

	c = egetchar("yYnN");
	if	(c >= 'A' && c <= 'Z')
		c += ('a' - 'A');
	return(c);
	}

egetchar(str)
	char	*str;
	{
	register int c;

	while	(1)
		{
		c = getchar();
		if	(index(str, c))
			break;
		putchar('\007');
		}
	putchar(c);
	putchar('\n');
	return(c);
	}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 *
 * Adapted from the disklabel utility.
 */
checklabel(lp)
	register struct disklabel *lp;
	{
	register struct partition *pp;
	int i, errors = 0;
	char part;

	if	(lp->d_nsectors == 0)
		{
		printf("sectors/track %d\n", lp->d_nsectors);
		return(1);
		}
	if	(lp->d_ntracks == 0)
		{
		printf("tracks/cylinder %d\n", lp->d_ntracks);
		return(1);
		}
	if	(lp->d_ncylinders == 0)
		{
		printf("cylinders/unit %d\n", lp->d_ncylinders);
		errors++;
		}
	if	(lp->d_rpm == 0)
		Warning("revolutions/minute %d", lp->d_rpm);
	if	(lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if	(lp->d_secperunit == 0)
		lp->d_secperunit = (long) lp->d_secpercyl * lp->d_ncylinders;

	for	(i = 0; i < lp->d_npartitions; i++)
		{
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if	(pp->p_size == 0 && pp->p_offset != 0)
			Warning("partition %c: size 0, but offset %d",
			    part, pp->p_offset);
#ifdef notdef
		if (pp->p_size % lp->d_secpercyl)
			Warning("partition %c: size %% cylinder-size != 0",
			    part);
		if (pp->p_offset % lp->d_secpercyl)
			Warning("partition %c: offset %% cylinder-size != 0",
			    part);
#endif
		if	(pp->p_offset > lp->d_secperunit)
			{
			printf("partition %c: offset past end of unit %D %D\n",
				part, pp->p_offset, lp->d_secperunit);
			errors++;
			}
		if	(pp->p_offset + pp->p_size > lp->d_secperunit)
			{
			printf("partition %c: extends past end of unit %D %D %D\n",
			    part, pp->p_offset, pp->p_size, lp->d_secperunit);
			errors++;
			}
		}
	for	(; i < MAXPARTITIONS; i++)
		{
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if	(pp->p_size || pp->p_offset)
			Warning("unused partition %c: size %D offset %D",
			    'a' + i, pp->p_size, pp->p_offset);
		}
	return(errors);
	}

/*VARARGS1*/
Warning(fmt, a1, a2, a3, a4, a5)
	char *fmt;
	{

	printf("Warning, ");
	printf(fmt, a1, a2, a3, a4, a5);
	printf("\n");
	}
