/*
 *
 *  Title:
 *	Backup
 *
 *  Decription:
 *	Program to read VMS backup tape
 *
 *  Author:
 *	John Douglas CAREY.
 *	Sven-Ove Westberg    (version 3.0)
 *
 *  Net-addess:
 *	john%monu1.oz@seismo.ARPA
 *	luthcad!sow@enea.UUCP
 *
 *  History:
 *	Version 1.0 - September 1984
 *		Can only read variable length records
 *	Version 1.1
 *		Cleaned up the program from the original hack
 *		Can now read stream files
 *	Version 1.2
 *		Now convert filename from VMS to UNIX
 *			and creates sub-directories
 *	Version 1.3
 *		Works on the Pyramid if SWAP is defined
 *	Version 1.4
 *		Reads files spanning multiple tape blocks
 *	Version 1.5
 *		Always reset reclen = 0 on file open
 *		Now output fixed length records
 *
 *      Version 2.0 - July 1985
 *		VMS Version 4.0 causes a rethink !!
 *		Now use mtio operations instead of opening and closing file
 *		Blocksize now grabed from the label
 *
 *	Version 2.1 - September 1985
 *		Handle variable length records of zero length.
 *
 *	Version 2.2 - July 1986
 *		Handle FORTRAN records of zero length.
 *		Inserted exit(0) at end of program.
 *		Distributed program in aus.sources
 *
 *	Version 2.3 - August 1986
 *		Handle FORTRAN records with record length fields
 *		at the end of a block
 *		Put debug output to a file.
 *		Distributed program in net.sources
 *
 *	Version 3.0 - December 1986
 *		Handle multiple saveset 
 *		Remote tape
 *		Interactive mode
 *		File name selection with meta-characters
 *		Convert ; to : in VMS filenames
 *		Flag for usage of VMS directory structure
 *		Flag for "useless" files  eg. *.exe
 *		Flag for use VMS version in file names
 *		Flag for verbose mode
 *		Flag to list the contents of the tape
 *		Distributed to mod.sources
 *
 *
 *  Installation:
 *
 *	Computer Centre
 *	Monash University
 *	Wellington Road
 *	Clayton
 *	Victoria	3168
 *	AUSTRALIA
 *
 */
#include	<stdio.h>
#include	<ctype.h>

#include	<sys/ioctl.h>
#include	<sys/types.h>
#ifdef REMOTE
#include	<local/rmt.h>
#include	<sys/stat.h>
#endif
#include	<sys/mtio.h>
#include	<sys/file.h>

#ifdef pyr
#define SWAP
#endif pyr

#ifdef sun
#define SWAP
#endif

#ifdef	pdp11
#define	bbh_dol_w_size _bdwopsize
#define	bbh_dol_w_opsys _bdwopsys
#define	bbh_dol_w_subsys _bdwsubsys
#define	bbh_dol_w_applic _bdwapplic
#define	bbh_dol_l_number _bdlnumber
#define	bbh_dol_t_spare_1 _bdtsp1
#define	bbh_dol_w_struclev _bdwstruclev
#define	bbh_dol_w_volnum _bdwvolnum
#define	bbh_dol_l_crc _bdlcrc
#define	bbh_dol_l_blocksize _bdlblocksize
#define	bbh_dol_l_flags _bdlflags
#define	bbh_dol_t_ssname _bdtssname
#define	bbh_dol_w_fid _bdwfid
#define	bbh_dol_w_did _bdwdid
#define	bbh_dol_t_filename _bdtfilename
#define	bbh_dol_b_rtype _bdbrtype
#define	bbh_dol_b_rattrib _bdbrattrib
#define	bbh_dol_w_rsize _bdwrsize
#define	bbh_dol_b_bktsize _bdbbktsize
#define	bbh_dol_b_vfcsize _bdbvfcsize
#define	bbh_dol_w_maxrec _bdwmaxrec
#define	bbh_dol_l_filesize _bdlfilesize
#define	bbh_dol_t_spare_2 _bdtsp2
#define	bbh_dol_w_checksum _bdwchecksum

#define	brh_dol_w_rsize _bhdwrsize
#define	brh_dol_w_rtype _bhwrtyppe
#define	brh_dol_l_flags _bhlflags
#define	brh_dol_l_address _bhladdress
#define	brh_dol_l_spare _bhlspare

#define	bsa_dol_w_size _bawsize
#define	bsa_dol_w_type _bawtype
#define	bsa_dol_t_text _battext

#define	process_file __procf
#define	process_vbn __procv
#endif

struct bbh {
	short	bbh_dol_w_size;
	short	bbh_dol_w_opsys;
	short	bbh_dol_w_subsys;
	short	bbh_dol_w_applic;
	long	bbh_dol_l_number;
	char	bbh_dol_t_spare_1[20];
	short	bbh_dol_w_struclev;
	short	bbh_dol_w_volnum;
	long	bbh_dol_l_crc;
	long	bbh_dol_l_blocksize;
	long	bbh_dol_l_flags;
	char	bbh_dol_t_ssname[32];
	short	bbh_dol_w_fid[3];
	short	bbh_dol_w_did[3];
	char	bbh_dol_t_filename[128];
	char	bbh_dol_b_rtype;
	char	bbh_dol_b_rattrib;
	short	bbh_dol_w_rsize;
	char	bbh_dol_b_bktsize;
	char	bbh_dol_b_vfcsize;
	short	bbh_dol_w_maxrec;
	long	bbh_dol_l_filesize;
	char	bbh_dol_t_spare_2[22];
	short	bbh_dol_w_checksum;
} *block_header;

struct brh {
	short	brh_dol_w_rsize;
	short	brh_dol_w_rtype;
	long	brh_dol_l_flags;
	long	brh_dol_l_address;
	long	brh_dol_l_spare;
} *record_header;

/* define record types */

#define	brh_dol_k_null	0
#define	brh_dol_k_summary	1
#define	brh_dol_k_volume	2
#define	brh_dol_k_file	3
#define	brh_dol_k_vbn	4
#define brh_dol_k_physvol	5
#define brh_dol_k_lbn	6
#define	brh_dol_k_fid	7

struct bsa {
	short	bsa_dol_w_size;
	short	bsa_dol_w_type;
	char	bsa_dol_t_text[1];
} *data_item;

#ifdef	STREAM
char	*def_tapefile = "/dev/rts8";
#else
char	*def_tapefile = "/dev/rmt8";
#endif
char	*tapefile;

char	filename[128];
int	filesize;

char	recfmt;		/* record format */

#define			FAB_dol_C_UDF	0	/* undefined */
#define			FAB_dol_C_FIX	1	/* fixed-length record */
#define			FAB_dol_C_VAR	2	/* variable-length record */
#define			FAB_dol_C_VFC	3	/* variable-length with fixed-length control record */
#define 		FAB_dol_C_STM	4	/* RMS-11 stream record (valid only for sequential org) */
#define			FAB_dol_C_STMLF	5	/* stream record delimited by LF (sequential org only) */
#define 		FAB_dol_C_STMCR	6	/* stream record delimited by CR (sequential org only) */
#define			FAB_dol_C_MAXRFM	6	/* maximum rfm supported */

char	recatt;		/* record attributes */

#define			FAB_dol_V_FTN	0	/* FORTRAN carriage control character */
#define			FAB_dol_V_CR	1	/* line feed - record -carriage return */
#define			FAB_dol_V_PRN	2	/* print-file carriage control */
#define			FAB_dol_V_BLK	3	/* records don't cross block boundaries */

#define	FANO	20

#ifdef	pyr
static struct	bsa	*file_table[FANO];
#else
struct	bsa	*file_table[FANO];
#endif

FILE	*f	= NULL;
int	file_count;
short	reclen;
short	fix;
short	recsize;
int	vfcsize;

#ifdef	NEWD
FILE	*lf;
#endif	NEWD

int	fd;		/* tape file descriptor */
int 	cflag, dflag, eflag, iflag, sflag, tflag, vflag, wflag, xflag, nflag;
int	setnr;
char	**gargv;
int 	goptind, gargc;

#define	LABEL_SIZE	80
char	label[LABEL_SIZE];

char	*block;
int	blocksize;

struct	mtop	op;

FILE *
openfile(fn)
char	*fn;
{
	char	ufn[256];
	char	ans[80];
	char	*p, *q, s, *ext;
	int	procf;

	procf = 1;
	/* copy fn to ufn and convert to lower case */
	p = fn;
	q = ufn;
	while (*p) {
		if (isupper(*p))
			*q = *p - 'A' + 'a';
		else
			*q = *p;
		p++;
		q++;
	}
	*q = '\0';

	/* convert the VMS to UNIX and make the directory path */
	p = ufn;
	q = ++p;
	while (*q) {
		if (*q == '.' || *q == ']') {
			s = *q;
			*q = '\0';
			if(procf && dflag) mkdir(p, 0777);
			*q = '/';
			if (s == ']')
				break;
		}
		*q++;
	}
	*q++;
	if(!dflag) p=q;
	/* strip off the version number */
	while (*q && *q != ';') {
		if( *q == '.') ext = q;
		q++;
	}
	if (cflag) {
		*q = ':';
	}
	else {
		*q = '\0';
	}
	if(!eflag && procf) procf = typecmp(++ext);
	if(procf && wflag) {
		printf("extract %s [ny]",filename);
		fflush(stdout);
		gets(ans);
		if(*ans != 'y') procf = NULL;
	}
	if(procf)
		/* open the file for writing */
		return(fopen(p, "w"));
	else
		return(NULL);
}

typecmp(str)    /* Compare the filename type in str with our list
                   of file type to be ignored.  Return 0 if the
                   file is to be ignored, return 1 if the
                   file is not in our list and should not be ignored. */
register char   *str;
{
        static char *type[] = {
                "exe",          /* vms executable image */
                "lib",          /* vms object library */
                "obj",          /* rsx object file */
                "odl",          /* rsx overlay description file */
                "olb",          /* rsx object library */
                "pmd",          /* rsx post mortem dump */
                "stb",          /* rsx symbol table */
                "sys",          /* rsx bootable system image */
                "tsk",          /* rsx executable image */
		"dir",
		"upd",
		"tlo",
		"tlb",
                ""              /* null string terminates list */
        };
        register int    i;

        i = -1;
        while (*type[++i])
                if (strncmp(str, type[i],3) == 0)
                        return(0);      /* found a match, file to be ignored */
        return(1);                      /* no match found */
}

process_file(buffer)
char	*buffer;
{
	int	i, n;
	char	*p, *q;
	short	dsize, nblk, lnch;

	int	c;
	short	*s;

	int 	procf;

	s = (short *) buffer;

	/* check the header word */
	if (*s != 257) {
		printf("Snark: invalid data header\n");
		exit(1);
	}

	c = 2;
	for (i = 0; i < FANO; i++) {
		file_table[i] = (struct bsa *) &buffer[c];
#ifndef	SWAP
		dsize = file_table[i]->bsa_dol_w_size;
#else
		swap(&file_table[i]->bsa_dol_w_size, &dsize, sizeof(short));
#endif
		c += dsize + 4;
	}

	/* extract file name */
#ifndef	SWAP
	dsize = file_table[0]->bsa_dol_w_size;
#else
	swap(&file_table[0]->bsa_dol_w_size, &dsize, sizeof(short));
#endif
	p = file_table[0]->bsa_dol_t_text;
	q = filename;
	for (i = 0; i < dsize; i++)
		*q++ = *p++;
	*q = '\0';

	/* extract file's record attributes */
#ifndef	SWAP
	dsize = file_table[5]->bsa_dol_w_size;
#else
	swap(&file_table[5]->bsa_dol_w_size, &dsize, sizeof(short));
#endif
	p = file_table[5]->bsa_dol_t_text;
	recfmt = p[0];
	recatt = p[1];
#ifndef	SWAP
	bcopy(&p[2], &recsize, sizeof(short));
#else
	swap(&p[2], &recsize, sizeof(short));
#endif
	vfcsize = p[15];
	if (vfcsize == 0)
		vfcsize = 2;
#ifdef	DEBUG
	printf("recfmt = %d\n", recfmt);
	printf("recatt = %d\n", recatt);
	printf("reclen = %d\n", recsize);
	printf("vfcsize = %d\n", vfcsize);
#endif
#ifndef	SWAP
	bcopy(&p[10], &nblk, sizeof(short));
	bcopy(&p[12], &lnch, sizeof(short));
#else
	swap(&p[10], &nblk, sizeof(short));
	swap(&p[12], &lnch, sizeof(short));
#endif
	filesize = (nblk-1)*512 + lnch;
#ifdef DEBUG
	printf("nbk = %d, lnch = %d\n", nblk, lnch);
	printf("filesize = 0x%x\n", filesize);
#endif

	/* open the file */
	if (f != NULL) {
		fclose(f);
		file_count = 0;
		reclen = 0;
	}
	procf = 0;
	if (goptind < gargc) 
		for(i=goptind; i < gargc; i++) {
			procf |= match(filename,gargv[i]);
		}
	else
		procf = 1;
	if (tflag && procf) 
		printf( " %-35s %8d \n",filename,filesize);
	if (xflag && procf) {
		/* open file */
		f = openfile(filename);
		if(f != NULL && vflag) printf("extracting %s\n", filename);
	}
}
/*
 *
 *  process a virtual block record (file record)
 *
 */
process_vbn(buffer, rsize)
char		*buffer;
unsigned short	rsize;
{
	int	c, i;

	if (f == NULL) {
		return;
	}
	i = 0;
	while (file_count+i < filesize && i < rsize) {
		switch (recfmt) {
		case FAB_dol_C_FIX:
			if (reclen == 0) {
				reclen = recsize;
			}
			fputc(buffer[i], f);
			i++;
			reclen--;
			break;

		case FAB_dol_C_VAR:
		case FAB_dol_C_VFC:
			if (reclen == 0) {
				reclen = *((short *) &buffer[i]);
#ifdef	SWAP
				swap(&reclen, &reclen, sizeof(short));
#endif
#ifdef	NEWD
				fprintf(lf, "---\n");
				fprintf(lf, "reclen = %d\n", reclen);
				fprintf(lf, "i = %d\n", i);
				fprintf(lf, "rsize = %d\n", rsize);
#endif	NEWD
				fix = reclen;
				i += 2;
				if (recfmt == FAB_dol_C_VFC) {
					i += vfcsize;
					reclen -= vfcsize;
				}
			} else if (reclen == fix
					&& recatt == (1 << FAB_dol_V_FTN)) {
					/****
					if (buffer[i] == '0')
						fputc('\n', f);
					else if (buffer[i] == '1')
						fputc('\f', f);
					*** sow ***/
					fputc(buffer[i],f); /** sow **/
					i++;
					reclen--;
			} else {
				fputc(buffer[i], f);
				i++;
				reclen--;
			}
			if (reclen == 0) {
				if (!nflag)
					fputc('\n', f);
				if (i & 1)
					i++;
			}
			break;

		case FAB_dol_C_STM:
		case FAB_dol_C_STMLF:
			if (reclen < 0) {
				printf("SCREAM\n");
			}
			if (reclen == 0) {
				reclen = 512;
			}
			c = buffer[i++];
			reclen--;
			if (c == '\n') {
				reclen = 0;
			}
			fputc(c, f);
			break;

		case FAB_dol_C_STMCR:
			c = buffer[i++];
			if (c == '\r')
				fputc('\n', f);
			else
				fputc(c, f);
			break;

		default:
			fclose(f);
			unlink(filename);
			fprintf(stderr, "Invalid record format = %d\n", recfmt);
			return;
		}
	}
	file_count += i;
}
#ifdef	SWAP
/*
 *
 *  do swapping for Motorola type architectures
 *
 */
swap(from, to, nbytes)
char	*from, *to;
int	nbytes;
{
	int	i, j;
	char	temp[100];

	for (i = 0; i < nbytes; i++)
		temp[i] = from[i];
	for (i = 0, j = nbytes-1; i < nbytes; i++, j--)
		to[i] = temp[j];
}
#endif
/*
 *
 *  process a backup block
 *
 */
process_block(block, blocksize)
char	*block;
int	blocksize;
{

	unsigned short	bhsize, rsize, rtype;
	u_long	bsize, i;

	i = 0;

	/* read the backup block header */
	block_header = (struct bbh *) &block[i];
	i += sizeof(struct bbh);

	bhsize = block_header->bbh_dol_w_size;
	bsize = block_header->bbh_dol_l_blocksize;
#ifdef	SWAP
	swap(&bhsize, &bhsize, sizeof(short));
	swap(&bsize, &bsize, sizeof(long));
#endif

	/* check the validity of the header block */
	if (bhsize != sizeof(struct bbh)) {
		fprintf(stderr, "Snark: Invalid header block size\n");
		exit(1);
	}
	if (bsize != 0 && bsize != blocksize) {
		fprintf(stderr, "Snark: Invalid block size\n");
		exit(1);
	}
#ifdef	DEBUG
	printf("new block: i = %d, bsize = %d\n", i, bsize);
#endif

	/* read the records */
	while (i < bsize) {
		/* read the backup record header */
		record_header = (struct brh *) &block[i];
		i += sizeof(struct brh);

		rtype = record_header->brh_dol_w_rtype;
		rsize = record_header->brh_dol_w_rsize;
#ifdef	SWAP
		swap(&rtype, &rtype, sizeof(short));
		swap(&rsize, &rsize, sizeof(short));
#endif
#ifdef	DEBUG
		printf("rtype = %d\n", rtype);
		printf("rsize = %d\n", rsize);
		printf("flags = 0x%x\n", record_header->brh_dol_l_flags);
		printf("addr = 0x%x\n", record_header->brh_dol_l_address);
		printf("i = %d\n", i);
#endif

		switch (rtype) {

		case brh_dol_k_null:
#ifdef	DEBUG
			printf("rtype = null\n");
#endif
			break;

		case brh_dol_k_summary:
#ifdef	DEBUG
			printf("rtype = summary\n");
#endif
			break;

		case brh_dol_k_file:
#ifdef	DEBUG
			printf("rtype = file\n");
#endif
			process_file(&block[i]);
			break;

		case brh_dol_k_vbn:
#ifdef	DEBUG
			printf("rtype = vbn\n");
#endif
			process_vbn(&block[i], rsize);
			break;

		case brh_dol_k_physvol:
#ifdef	DEBUG
			printf("rtype = physvol\n");
#endif
			break;

		case brh_dol_k_lbn:
#ifdef	DEBUG
			printf("rtype = lbn\n");
#endif
			break;

		case brh_dol_k_fid:
#ifdef	DEBUG
			printf("rtype = fid\n");
#endif
			break;

		default:
			fprintf(stderr, " Snark: invalid record type\n");
			fprintf(stderr, " record type = %d\n", rtype);
			exit(1);
		}
#ifdef pyr
		i = i + rsize;
#else
		i += rsize;
#endif
	}
}

rdhead()
{
	int i, nfound;
	char name[80];
	nfound = 1;
	/* read the tape label - 4 records of 80 bytes */
	while ((i = read(fd, label, LABEL_SIZE)) != 0) {
		if (i != LABEL_SIZE) {
			fprintf(stderr, "Snark: bad label record\n");
			exit(1);
		}
		if (strncmp(label, "VOL1",4) == 0) {
			sscanf(label+4, "%14s", name);
			if(vflag || tflag) printf("Volume: %s\n",name);
		}
		if (strncmp(label, "HDR1",4) == 0) {
			sscanf(label+4, "%14s", name);
			sscanf(label+31, "%4d", &setnr);
		}
		/* get the block size */
		if (strncmp(label, "HDR2", 4) == 0) {
			nfound = 0;
			sscanf(label+5, "%5d", &blocksize);
#ifdef	DEBUG
			printf("blocksize = %d\n", blocksize);
#endif
		}
	}
	if((vflag || tflag) && !nfound) 
		printf("Saveset name: %s   number: %d\n",name,setnr);
	/* get the block buffer */
	block = (char *) malloc(blocksize);
	if (block == (char *) 0) {
		fprintf(stderr, "memory allocation for block failed\n");
		exit(1);
	}
	return(nfound);
}

rdtail()
{
	int i;
	char name[80];
	/* read the tape label - 4 records of 80 bytes */
	while ((i = read(fd, label, LABEL_SIZE)) != 0) {
		if (i != LABEL_SIZE) {
			fprintf(stderr, "Snark: bad label record\n");
			exit(1);
		}
		if (strncmp(label, "EOF1",4) == 0) {
			sscanf(label+4, "%14s", name);
			if(vflag || tflag)
				printf("End of saveset: %s\n\n\n",name);
		}
	}
}

usage(progname)
char	*progname;
{
	fprintf(stderr,
	  "Usage:  %s -{tx}[cdenvw][-s setnumber][-f tapefile]\n",progname);
}
main(argc, argv)
int	argc;
char	*argv[];
{
	
	char *progname;
	int	c, i, eoffl;
	int	selset;
	extern int optind;
	extern char *optarg;

	progname = argv[0];
	if(argc < 2){
		usage(progname);
		exit(1);
	}
	gargv = argv;
	gargc = argc;
	tapefile = def_tapefile;
	cflag=dflag=eflag=iflag=nflag=sflag=tflag=vflag=wflag=xflag=0;
	while((c=getopt(argc,argv,"cdef:ins:tvwx")) != EOF)
		switch(c){
		case 'c':
			cflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'e':
			eflag++;
			break;
		case 'f':
			tapefile = optarg;
			break;
		case 'i':
			iflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 's':
			sflag++;
			sscanf(optarg,"%d",&selset);
			break;
		case 't':
			tflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			xflag++;
			break;
		case '?':
			usage(progname);
			exit(1);
			break;
		};
	if(!tflag && !xflag) {
		usage(progname);
		exit(1);
	}
	goptind = optind;

#ifdef	NEWD
	/* open debug file */
	lf = fopen("log", "w");
	if (lf == NULL) {
		perror("log");
		exit(1);
	}
#endif

	/* open the tape file */
	fd = open(tapefile, O_RDONLY);
	if (fd < 0) {
		perror(tapefile);
		exit(1);
	}

	/* rewind the tape */
	op.mt_op = MTREW;
	op.mt_count = 1;
	i = ioctl(fd, MTIOCTOP, &op);
	if (i < 0) {
		perror(tapefile);
		exit(1);
	}

	eoffl = rdhead();
	/* read the backup tape blocks until end of tape */ 
	while (!eoffl) {
		if(sflag && setnr != selset) {
			op.mt_op = MTFSF;
			op.mt_count = 1;
			i = ioctl(fd, MTIOCTOP, &op);
			if (i < 0) {
				perror(tapefile);
				exit(1);
			}
			i = 0;
		}
		else
			i = read(fd, block, blocksize);
		if(i == 0) {
			rdtail();
			eoffl=rdhead();
		}
		else if (i != blocksize) {
			fprintf(stderr, "bad block read i = %d\n", i);
			if (!iflag)
				exit(1);
			else if (f != NULL) {
				printf("File '%s' truncated - bad blocks\n",
					filename);
				fclose(f);
				f = NULL;
				file_count = 0;
				reclen = 0;
			}
		}
		else{
			eoffl = 0;
			process_block(block, blocksize);
		}
	}
	if(vflag || tflag) printf("End of tape\n");

	/* close the tape */
	close(fd);

#ifdef	NEWD
	/* close debug file */
	fclose(lf);
#endif	NEWD

	/* exit cleanly */
	exit(0);
}
