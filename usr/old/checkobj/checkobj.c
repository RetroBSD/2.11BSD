/*
 * checkobj
 *	checks if a file can be executed.
 *	looks at type, size, and requirements of floating point.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>

#define	UNIX	"/unix"		/* for namelist (to get maxmem) */
#define	MEM	"/dev/kmem"	/* to read maxmem from */
#define SSEG	8192		/* pdp 11 segment size (bytes) */
#define MAXDATA	(56*1024L)	/* maximum process data size */
#define MAXTEXT	(64*1024L)	/* maximum process text size */
#define SETD	0170011		/* setd instruction */

#define	fullseg(siz)	((((long)siz + (SSEG-1))/SSEG) * SSEG)
#define	segs(siz)	((int)(((long)siz + (SSEG-1))/SSEG))
#define lctob(cliks)	((long)(unsigned)cliks * ctob(1))

struct exec obj;
struct ovlhdr	ovlhdr;

	struct nlist nl[2];
	struct nlist fpsim[3];

int	sflag;
int	fflag;

main(argc,argv) int argc;char **argv;
{
	int x,fail=0;
	char *myname;

	nl[0].n_un.n_name = "maxmem";
	fpsim[0].n_un.n_name = "fptrap";
	fpsim[1].n_un.n_name = "fltused";
	myname = argv[0];
	argc--,argv++;
	while (argc > 1 && argv[0][0]=='-') {
	    while (*++*argv)
		switch (**argv) {
		case 'f':
			fflag++;
			break;
		case 's':
			sflag++;
			break;
		default:
			if (atoi(*argv) == 40) {
				sflag++;
				fflag++;
				goto nxt;
			} else goto usage;
		}
nxt:
		argc--, argv++;
	}
	if (argc > 1)
		for(x = 0 ; x<argc ; x++)
		{
			printf("%s:\t",argv[x]);
			fail |= check(argv[x]);
			printf("\n");
		}
	else if (argc == 1)
		fail = check(argv[0]);
	else
	{
usage:
		printf("Usage: %s [-f] [-40] <obj-file> ...\n", myname);
		exit(10);
	}
	exit(fail);
}


int check(file) char *file;
{
	register i;
	int fi, ins, fail, maxmem;
	long size, tsize=0L, totsize, bytesmax;

	fail = 0;
	if ((fi=open(file,0)) < 0)
	{
		perror(file);
		return(1);
	}
	if (read(fi,&obj,sizeof(obj)) != sizeof(obj))
	{
		printf("not an object file.\n");
		close(fi);
		return(1);
	}
	if (obj.a_magic == A_MAGIC5 || obj.a_magic == A_MAGIC6)
		if (read(fi,&ovlhdr,sizeof(ovlhdr)) != sizeof(ovlhdr)) {
			printf("not an object file.\n");
			close(fi);
			return(1);
		}
	switch(obj.a_magic)
	{

	case A_MAGIC1:
		size = (long)obj.a_text + obj.a_data + obj.a_bss;
		totsize = size;
		break;

	case A_MAGIC2:
	case A_MAGIC4:
		size = fullseg(obj.a_text) + obj.a_data + obj.a_bss;
		totsize = (long)obj.a_text + obj.a_data + obj.a_bss;
		break;

	case A_MAGIC3:
		tsize = fullseg(obj.a_text);
		size = (long) obj.a_data + obj.a_bss;
		totsize = (long)obj.a_text + obj.a_data + obj.a_bss;
		break;

	case A_MAGIC5:
		size = fullseg(obj.a_text) + obj.a_data + obj.a_bss;
		size += fullseg(ovlhdr.max_ovl);
		totsize = (long)obj.a_text + obj.a_data + obj.a_bss;
		for (i=0; i<NOVL; i++)
			totsize += ovlhdr.ov_siz[i];
		break;

	case A_MAGIC6:
		tsize = fullseg(obj.a_text) + fullseg(ovlhdr.max_ovl);
		size = obj.a_data + obj.a_bss;
		totsize = (long)obj.a_text + obj.a_data + obj.a_bss;
		for (i=0; i<NOVL; i++)
			totsize += ovlhdr.ov_siz[i];
		break;

	default:
		printf("not an object file.\n");
		close(fi);
		return(1);
	}

	if (sflag) {
	    if (tsize>0) {
		fail++;
		printf("uses separate i/d\n");
	    }
	    if (size > MAXDATA) {
		fail++;
		if (tsize)
		    printf("Data segment too big (limit %u)\n",
			(unsigned)MAXDATA);
		else {
		    printf("Too big");
		    if (ovlhdr.max_ovl != 0)
printf(": More than 8 segments used (%d base, %d overlay, %d data, 1 stack)\n",
			segs(obj.a_text), segs(ovlhdr.max_ovl),
			segs(obj.a_data+(long)obj.a_bss));
		    else
		    if ((long)obj.a_text+obj.a_data+obj.a_bss <= MAXDATA)
			printf(": try loading without -n\n");
		    /*
		     * Algorithm to determine if overlaying will work:
		     * assume one segment for base, rest of data divided into
		     * NOVL overlays, plus data and one segment for stack.
		     */
		    else if (segs((obj.a_text-SSEG)/NOVL)
			+ segs((long)obj.a_data+obj.a_bss) + 2 <= 8)
			printf(" (limit %u): Try overlaying\n",
			    (unsigned)MAXDATA);
		    else printf(" (limit %u bytes)\n", (unsigned)MAXDATA);
		}
	    } else if (tsize) {
		/*
		 * Was separate I/D.  Offer helpful hints.
		 */
		if (((long)fullseg(obj.a_text)
		    + (long)obj.a_data + obj.a_bss) <= MAXDATA)
			printf("Try loading with -n\n");
		else if (((long)obj.a_text +
		    (long)obj.a_data + obj.a_bss) <= MAXDATA)
			printf("Try loading without -n or -i\n");
		else if (ovlhdr.max_ovl==0 && segs((obj.a_text-SSEG)/NOVL)
		    + segs((long)obj.a_data+obj.a_bss) + 2 <= 8)
			printf("Too big (limit %u bytes): try overlaying\n",
			    (unsigned)MAXDATA);
	    }
	} else {
		/*
		 * Not sflag.
		 */
		if (tsize > MAXTEXT) {
		    /*
		     * Can only happen when overlaid (text is 16 bits)
		     */
		    fail++;
		    printf("More than 8 text segments: %d base, %d overlay\n",
			segs(obj.a_text), segs(ovlhdr.max_ovl));
		}
		if (size > MAXDATA) {
		    fail++;
		    printf("%s too big (limit %u bytes)\n",
			tsize? "Data segment ": "", (unsigned)MAXDATA);
		    if (tsize == 0)
			printf("Try loading with -i\n");

		}
	}
	maxmem = MAXMEM;
	nlist(UNIX, nl);
	if (nl[0].n_value) {
		int f = open(MEM, 0);
		if (f>0) {
			lseek(f, (off_t)nl[0].n_value, 0);
			read(f, &maxmem, sizeof maxmem);
		}
		close(f);
	}
	bytesmax = lctob(maxmem);
	if (totsize > bytesmax) {
		fail++;
    printf("Total size %D bytes, too large for this computer (%D maximum)\n",
		totsize, bytesmax);
	}

	if (fflag) {
	    /*
	     * fetch first instruction.
	     * if setd, then might require fpsim.
	     */

	    if ((read(fi,&ins,2)!=2))
	    {
		if (fail)printf(", ");
		printf("not an object file.\n");
		close(fi);
		return(1);
	    }
	    if ( ins == SETD )
	    {
		nlist(file,fpsim);
		if (fpsim[1].n_type != N_UNDF && fpsim[0].n_type == N_UNDF)
		{
			printf("Uses floating point\n");
			fail = 1;
		}
	    }
	}
	close(fi);
	return(fail);
}
