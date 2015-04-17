static char *
RcsId = "$Header: /usr/src/local/RCS/buildcore.c,v 1.2 83/04/26 21:24:33 crl Exp $";

/*
 * buildcore - rebuild an executable file from a core image
 */
 
#include <stdio.h>
#include <a.out.h>
#include <sys/param.h>
#include <sys/stat.h>
#undef DIRSIZ
#include <dir.h>
#include <sys/user.h>

char	*efilename;		/* executable filename */
char	*cfilename = "core";	/* core filename */
char	*afilename = "a.out";	/* new executable file */
FILE	*efile;
FILE	*cfile;
FILE	*afile;			/* a.out */
char	*progname;		/* our name */
unsigned olddata;		/* size of data in executable */
struct user u;			/* copy of user structure */
struct exec e;			/* a.out header */
struct stat sbuf;		/* for stat */
struct ovlhdr overlay;		/* overlay header */

char	*rindex();
FILE	*fopen();

main(argc, argv)
int argc;
char *argv[];
{
	register unsigned i;
	register unsigned j;
	register c;

	if ((progname = rindex(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		progname++;

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", progname);
		exit(1);
	}
	efilename = argv[1];
	if ((efile = fopen(efilename, "r")) == NULL) {
		fprintf(stderr, "%s: can't read %s\n", progname, efilename);
		exit(1);
	}
	if ((cfile = fopen(cfilename, "r")) == NULL) {
		fprintf(stderr, "%s: can't read %s\n", progname, cfilename);
		exit(1);
	}
	if ((afile = fopen(afilename, "w")) == NULL) {
		fprintf(stderr, "%s: can't write %s\n", progname, afilename);
		exit(1);
	}
	if (fread(&u, sizeof u, 1, cfile) != 1) {
		fprintf(stderr, "%s: error reading user structure\n",
				   progname);
		exit(1);
	}
	fseek(cfile, (long)ctob(USIZE), 0);   /* seek past all of user area */
	if (fread(&e, sizeof e, 1, efile) != 1) {
		fprintf(stderr, "%s: error reading old exec header\n",
					progname);
		exit(1);
	}

	/*
	 * check for a support magic number
	 */
	switch(e.a_magic) {
	  case A_MAGIC1:		/* normal */
	  case A_MAGIC4:		/* manual overlay */
	  case A_MAGIC5:		/* non-sep auto overlay */
		fprintf(stderr, "%s: unsupported executable type %o\n",
		 			progname, e.a_magic);
		exit(1);
	}
	/*
	 * build the a.out header
	 */
	olddata = e.a_data;		/* save it for an offset */
	e.a_data = ctob(u.u_dsize);	/* get new data size from core */
	e.a_bss = 0;			/* can't use bss anymore */

	if (fwrite(&e, sizeof e, 1, afile) != 1) {
		fprintf(stderr, "%s: error in writing %s header\n",
					progname, afilename);
		exit(1);
	}
	/*
	 * write out overlay header if overlayed
	 */
	if (e.a_magic == A_MAGIC6) {
		if (fread(&overlay, sizeof overlay, 1, efile) != 1) {
			fprintf(stderr, "%s: error reading overlay header\n",
							progname);
			exit(1);
		}
		if (fwrite(&overlay, sizeof overlay, 1, afile) != 1) {
			fprintf(stderr, "%s: error writing overlay header\n",
							progname);
			exit(1);
		}
	}
	/*
	 * now write out the base text segment
	 */	
	i = e.a_text;
	if (i) do {
		if ((c = getc(efile)) == EOF) {
			fprintf(stderr, "%s: unexpected EOF reading base text segment\n", progname);
			exit(1);
		}
		putc(c, afile);
	} while (--i);
	/*
	 * write out overlays
	 */
	if (e.a_magic == A_MAGIC6) {
		for (i = 0; overlay.ov_siz[i] != 0 && i < NOVL; i++) {
			j = overlay.ov_siz[i];
			do {
				if ((c = getc(efile)) == EOF) {
				fprintf(stderr, "%s: unexpected EOF reading overlays \n", progname);
				exit(1);
				}
				putc(c, afile);
			} while (--j);
		}
	}
	/*
	 * and the new data segment
	 */	
	i = ctob(u.u_dsize);
	if (i) do {
		if ((c = getc(cfile)) == EOF) {
			fprintf(stderr, "%s: unexpected EOF reading core data, i = %d\n", progname, i);
			exit(1);
		}
		putc(c, afile);
	} while (--i);
	/*
	 * skip over the data segment in the executable
	 * and copy the rest of the file to the new a.out
	 */
	fseek(efile, (long) olddata, 1);
	while((c = getc(efile)) != EOF)
		putc(c, afile);
	/*
	 * make the new file executable
	 */
	fstat(fileno(afile), &sbuf);
	if ((sbuf.st_mode & 0111) == 0) {
		sbuf.st_mode |= 0111;
		chmod(afilename, sbuf.st_mode);
	}
}
