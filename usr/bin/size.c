#if	defined(DOSCCS) && !defined(lint)
static	char *sccsid = "@(#)size.c	4.4.1 (2.11BSD GTE) 1/1/94";
#endif

/*
 * size
 */

#include	<stdio.h>
#include 	<a.out.h>

int	header;

main(argc, argv)
char **argv;
{
	struct exec buf;
	long sum;
	int gorp,i;
	int err = 0;
	FILE *f;
#ifdef pdp11
	struct ovlhdr	ovlbuf;		/* overlay structure */
	long	coresize;		/* total text size */
	short	skip;			/* skip over overlay sizes of 0 */
#endif

	if (argc==1) {
		*argv = "a.out";
		argc++;
		--argv;
	}
	gorp = argc;
	while(--argc) {
		++argv;
		if ((f = fopen(*argv, "r"))==NULL) {
			printf("size: %s not found\n", *argv);
			err++;
			continue;
		}
		if (fread((char *)&buf, sizeof(buf), 1, f) != 1 ||
		    N_BADMAG(buf)) {
			printf("size: %s not an object file\n", *argv);
			fclose(f);
			err++;
			continue;
		}
		if (header == 0) {
			printf("text\tdata\tbss\tdec\thex\n");
			header = 1;
		}
		printf("%u\t%u\t%u\t", buf.a_text,buf.a_data,buf.a_bss);
		sum = (long) buf.a_text + (long) buf.a_data + (long) buf.a_bss;
		printf("%ld\t%lx", sum, sum);
		if (gorp>2)
			printf("\t%s", *argv);
#ifdef pdp11
		if (buf.a_magic == A_MAGIC5 || buf.a_magic == A_MAGIC6) {
			fread(&ovlbuf,sizeof(ovlbuf),1,f);
			coresize = buf.a_text;
			for (i = 0; i < NOVL; i++)
				coresize += ovlbuf.ov_siz[i];
			printf("\ttotal text: %ld\n\toverlays: ", coresize);
			for (i = 0,skip = 0; i < NOVL; i++) {
				if (!ovlbuf.ov_siz[i]) {
					++skip;
					continue;
				}
				for (;skip;--skip)
					fputs(",0",stdout);
				if (i > 0)
					putchar(',');
				printf("%u", ovlbuf.ov_siz[i]);
			}
		}
#endif
		printf("\n");
		fclose(f);
	}
	exit(err);
}
