#if	!defined(lint) && !defined(DOSCCS)
static	char sccsid[] = "@(#)nm.c 2.11BSD 1/22/94";
#endif
/*
 * nm - print name list. string table version
 */
#include <sys/types.h>
#include <sys/dir.h>
#include <ar.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
#include <sys/file.h>
#include "archive.h"
#include <string.h>

	CHDR	chdr;

#define	SELECT	archive ? chdr.name : *xargv

	char	aflg, gflg, nflg, oflg, pflg, uflg, rflg = 1, archive;
	char	**xargv;
	char	*strp;
	union {
		char	mag_armag[SARMAG+1];
		struct	xexec mag_exp;
	} mag_un;

	off_t	off, skip();
extern	off_t	ftell();
extern	char	*malloc();
extern	long	strtol();
	int	compare(), narg, errs;

main(argc, argv)
	int	argc;
	char	**argv;
{

	if (--argc>0 && argv[1][0]=='-' && argv[1][1]!=0) {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'n':
			nflg++;
			continue;
		case 'g':
			gflg++;
			continue;
		case 'u':
			uflg++;
			continue;
		case 'r':
			rflg = -1;
			continue;
		case 'p':
			pflg++;
			continue;
		case 'o':
			oflg++;
			continue;
		case 'a':
			aflg++;
			continue;
		default:
			fprintf(stderr, "nm: invalid argument -%c\n",
			    *argv[0]);
			exit(2);
		}
		argc--;
	}
	if (argc == 0) {
		argc = 1;
		argv[1] = "a.out";
	}
	narg = argc;
	xargv = argv;
	while (argc--) {
		++xargv;
		namelist();
	}
	exit(errs);
}

namelist()
{
	char	ibuf[BUFSIZ];
	register FILE	*fi;

	archive = 0;
	fi = fopen(*xargv, "r");
	if (fi == NULL) {
		error(0, "cannot open");
		return;
	}
	setbuf(fi, ibuf);

	off = 0;
	fread((char *)&mag_un, 1, sizeof(mag_un), fi);
	if (strncmp(mag_un.mag_armag, ARMAG, SARMAG)==0) {
		archive++;
		off = SARMAG;
	}
	else if (N_BADMAG(mag_un.mag_exp.e)) {
		error(0, "bad format");
		goto out;
	}
	rewind(fi);

	if (archive) {
		nextel(fi);
		if (narg > 1)
			printf("\n%s:\n", *xargv);
	}
	do {
		off_t	o, curpos, stroff;
		long	strsiz;
		register i, n;
		struct nlist *symp = NULL;
		struct	nlist sym;

		curpos = ftell(fi);
		fread((char *)&mag_un.mag_exp, 1, sizeof(struct xexec), fi);
		if (N_BADMAG(mag_un.mag_exp.e))
			continue;

		o = N_SYMOFF(mag_un.mag_exp);
		fseek(fi, curpos + o, L_SET);
		n = mag_un.mag_exp.e.a_syms / sizeof(struct nlist);
		if (n == 0) {
			error(0, "no name list");
			continue;
		}

		i = 0;
		if (strp)
			free(strp), strp = 0;
		while (--n >= 0) {
			fread((char *)&sym, 1, sizeof(sym), fi);
			if (sym.n_un.n_strx == 0)
				continue;
			if (gflg && (sym.n_type & N_EXT) == 0)
				continue;
			if (uflg && (sym.n_type & N_TYPE) == N_UNDF && sym.n_value)
				continue;
			i++;
		}

		fseek(fi, curpos + o, L_SET);
		symp = (struct nlist *)malloc(i * sizeof (struct nlist));
		if (symp == 0)
			error(1, "out of memory");
		i = 0;
		n = mag_un.mag_exp.e.a_syms / sizeof(struct nlist);
		while (--n >= 0) {
			fread((char *)&sym, 1, sizeof(sym), fi);
			if (sym.n_un.n_strx == 0)
				continue;
			if (gflg && (sym.n_type & N_EXT) == 0)
				continue;
			if (uflg && (sym.n_type & N_TYPE) == N_UNDF && sym.n_value)
				continue;
			symp[i++] = sym;
		}
		stroff = curpos + N_STROFF(mag_un.mag_exp);
		fseek(fi, stroff, L_SET);
		if (fread(&strsiz, sizeof (long), 1, fi) != 1)
			error(1, "no string table");
		strp = (char *)malloc((int)strsiz);
		if (strp == NULL || strsiz > 48 * 1024L)
			error(1, "ran out of memory");
		if (fread(strp+sizeof(strsiz),(int)strsiz-sizeof(strsiz),1,fi) != 1)
			error(1, "error reading strings");
		for (n = 0; n < i; n++)
			symp[n].n_un.n_name = strp + (int)symp[n].n_un.n_strx;

		if (pflg==0)
			qsort(symp, i, sizeof(struct nlist), compare);
		if ((archive || narg>1) && oflg==0)
			printf("\n%s:\n", SELECT);
		psyms(symp, i);
		if (symp)
			free((char *)symp), symp = NULL;
		if (strp)
			free((char *)strp), strp = NULL;
	} while(archive && nextel(fi));
out:
	fclose(fi);
}

psyms(symp, nsyms)
	register struct nlist *symp;
	int nsyms;
{
	register int n, c;

	for (n=0; n<nsyms; n++) {
		c = symp[n].n_type;
		if (c == N_FN)
			c = 'f';
		else switch (c&N_TYPE) {

		case N_UNDF:
			c = 'u';
			if (symp[n].n_value)
				c = 'c';
			break;
		case N_ABS:
			c = 'a';
			break;
		case N_TEXT:
			c = 't';
			break;
		case N_DATA:
			c = 'd';
			break;
		case N_BSS:
			c = 'b';
			break;
		case N_REG:
			c = 'r';
			break;
		default:
			c = '?';
			break;
		}
		if (uflg && c!='u')
			continue;
		if (oflg) {
			if (archive)
				printf("%s:", *xargv);
			printf("%s:", SELECT);
		}
		if (symp[n].n_type&N_EXT)
			c = toupper(c);
		if (!uflg) {
			if (c=='u' || c=='U')
				printf("      ");
			else
				printf(N_FORMAT, symp[n].n_value);
			printf(" %c ", c);
		}
		if (symp[n].n_ovly)
			printf("%s %d\n", symp[n].n_un.n_name,
				symp[n].n_ovly & 0xff);
		else
			printf("%s\n", symp[n].n_un.n_name);
	}
}

compare(p1, p2)
register struct nlist *p1, *p2;
{

	if (nflg) {
		if (p1->n_value > p2->n_value)
			return(rflg);
		if (p1->n_value < p2->n_value)
			return(-rflg);
	}
	return (rflg * strcmp(p1->n_un.n_name, p2->n_un.n_name));
}

nextel(af)
FILE *af;
{
	
	fseek(af, off, L_SET);
	if (get_arobj(af) < 0)
		return(0);
	off += (sizeof (struct ar_hdr) + chdr.size + 
		(chdr.size + chdr.lname & 1));
	return(1);
}

error(n, s)
char *s;
{
	fprintf(stderr, "nm: %s:", *xargv);
	if (archive) {
		fprintf(stderr, "(%s)", chdr.name);
		fprintf(stderr, ": ");
	} else
		fprintf(stderr, " ");
	fprintf(stderr, "%s\n", s);
	if (n)
		exit(2);
	errs = 1;
}

/*
 * "borrowed" from 'ar' because we didn't want to drag in everything else
 * from 'ar'.  The error checking was also ripped out, basically if any
 * of the criteria for being an archive are not met then a -1 is returned
 * and the rest of 'ld' figures out what to do.
*/

typedef struct ar_hdr HDR;
static char hb[sizeof(HDR) + 1];	/* real header */

/* Convert ar header field to an integer. */
#define	AR_ATOI(from, to, len, base) { \
	bcopy(from, buf, len); \
	buf[len] = '\0'; \
	to = strtol(buf, (char **)NULL, base); \
}

/*
 *	read the archive header for this member.  Use a file pointer 
 *	rather than a file descriptor.
 */
get_arobj(fp)
	FILE *fp;
{
	HDR *hdr;
	register int len, nr;
	register char *p;
	char buf[20];

	nr = fread(hb, 1, sizeof(HDR), fp);
	if (nr != sizeof(HDR))
		return(-1);

	hdr = (HDR *)hb;
	if (strncmp(hdr->ar_fmag, ARFMAG, sizeof(ARFMAG) - 1))
		return(-1);

	/* Convert the header into the internal format. */
#define	DECIMAL	10
#define	OCTAL	 8

	AR_ATOI(hdr->ar_date, chdr.date, sizeof(hdr->ar_date), DECIMAL);
	AR_ATOI(hdr->ar_uid, chdr.uid, sizeof(hdr->ar_uid), DECIMAL);
	AR_ATOI(hdr->ar_gid, chdr.gid, sizeof(hdr->ar_gid), DECIMAL);
	AR_ATOI(hdr->ar_mode, chdr.mode, sizeof(hdr->ar_mode), OCTAL);
	AR_ATOI(hdr->ar_size, chdr.size, sizeof(hdr->ar_size), DECIMAL);

	/* Leading spaces should never happen. */
	if (hdr->ar_name[0] == ' ')
		return(-1);

	/*
	 * Long name support.  Set the "real" size of the file, and the
	 * long name flag/size.
	 */
	if (!bcmp(hdr->ar_name, AR_EFMT1, sizeof(AR_EFMT1) - 1)) {
		chdr.lname = len = atoi(hdr->ar_name + sizeof(AR_EFMT1) - 1);
		if (len <= 0 || len > MAXNAMLEN)
			return(-1);
		nr = fread(chdr.name, 1, (size_t)len, fp);
		if (nr != len)
			return(-1);
		chdr.name[len] = 0;
		chdr.size -= len;
	} else {
		chdr.lname = 0;
		bcopy(hdr->ar_name, chdr.name, sizeof(hdr->ar_name));

		/* Strip trailing spaces, null terminate. */
		for (p = chdr.name + sizeof(hdr->ar_name) - 1; *p == ' '; --p);
		*++p = '\0';
	}
	return(1);
}
