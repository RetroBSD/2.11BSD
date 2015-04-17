/* decompr16: decompress 16bit compressed files on a 16bit Intel processor
 *    running minix.
 * This kludge was hacked together by John N. White on 6/30/91 and
 *    is Public Domain.
 * Use chmem =500 decompr16.
 * decompr16 needs about 280k to run in pipe mode. (56k per copy)
 *
 * This program acts as a filter:
 *    decompr16 < compressed_file > decompressed_file
 *
 * The arguments -0 to -4 causes only the corresponding pass to be done. Thus:
 *    decompr16 -0 < compressed_file | decompr16 -1 | decompr16 -2 |
 *         decompr16 -3 | decompr16 -4 > decompressed_file
 * will also work.
 * If RAM is scarce these passes can be done sequentially using tmp files.
 *
 * Compress uses a modified LZW compression algorithm. A compressed file
 * is a set of indices into a dictionary of strings. The number of bits
 * used to store each index depends on the number of entries currently
 * in the dictionary. If there are between 257 and 512 entries, 9 bits
 * are used. With 513 entries, 10 bits are used, etc. The initial dictionary
 * consists of 0-255 (which are the corresponding chars) and 256 (which
 * is a special CLEAR code). As each index in the compressed file is read,
 * a new entry is added to the dictionary consisting of the current string
 * with the first char of the next string appended. When the dictionary
 * is full, no further entries are added. If a CLEAR code is received,
 * the dictionary will be completely reset. The first two bytes of the
 * compressed file are a magic number, and the third byte indicates the
 * maximum number of bits, and whether the CLEAR code is used (older versions
 * of compress didn't have CLEAR).
 *
 * This program works by forking four more copies of itself. The five
 * programs form a pipeline. Copy 0 reads from stdin and writes to
 * copy 1, which writes to copy 2, then to copy 3, then to copy 4 (the
 * original) which writes to stdout. If given a switch -#, where # is a
 * digit from 0 to 4 (example: -2), the program will run as that copy,
 * reading from stdin and writing to stdout. This allows decompressing
 * with very limited RAM because only one of the five passes is in
 * memory at a time.
 *
 * The compressed data is a series of string indices (and a header at
 * the beginning and an occasional CLEAR code). As these indices flow
 * through the pipes, each program decodes the ones it can. The result
 * of each decoding will be indices that the following programs can handle.
 *
 * Each of the 65536 strings in the dictionary is an earlier string with
 * some character added to the end (except for the the 256 predefined
 * single char strings). When new entries are made to the dictionary,
 * the string index part will just be the last index to pass through.
 * But the char part is the first char of the next string, which isn't
 * known yet. So the string can be stored as a pair of indices. When
 * this string is specified, it is converted to this pair of indices,
 * which are flagged so that the first will be decoded in full while
 * the second will be decoded to its first char. The dictionary takes
 * 256k to store (64k strings of 2 indices of 2 bytes each). This is
 * too big for a 64k data segment, so it is divided into 5 equal parts.
 * Copy 0 of the program maintains the high part and copy 4 holds the
 * low part.
 */

#define BUFSZ		256		/* size of i/o buffers */
#define BUFSZ_2		(BUFSZ/2)	/* # of unsigned shorts in i/o bufs */
#define DICTSZ		(unsigned)13056	/* # of local dictionary entries */
#define EOF_INDEX	(unsigned short)0xFFFF	/* EOF flag for pipeline */

int pipein=0, pipeout=1;	/* file nums for pipeline input and output */
int fildes[2];			/* for use with pipe() */
int ibufp, obufp, ibufe;	/* pointers to bufs, (inp, output, end) */
int ipbufp=BUFSZ_2, opbufp;	/* pointers to pipe bufs */
int pnum= -1;			/* ID of this copy */
unsigned short ipbuf[BUFSZ_2];	/* for buffering input */
unsigned short opbuf[BUFSZ_2];	/* for buffering output */
unsigned char *ibuf=(unsigned char*)ipbuf, *obuf=(unsigned char*)opbuf;

unsigned short	dindex[DICTSZ];	/* dictionary: index to substring */ 
unsigned short	dchar[DICTSZ];	/* dictionary: last char of string */
unsigned iindex, tindex, tindex2;/* holds index being processed */
unsigned base;			/* where in global dict local dict starts */
unsigned tbase;
unsigned locend;		/* where in global dict local dict ends */
unsigned curend=256;		/* current end of global dict */
unsigned maxend;		/* max end of global dict */
int dcharp;			/* ptr to dchar that needs next index entry */
int curbits;			/* number of bits for getbits() to read */
int maxbits;			/* limit on number of bits */
int clearflg;			/* if set, allow CLEAR */
int inmod;			/* mod 8 for getbits() */

main(argc, argv) char **argv; {
  int i;

  /* check for arguments */
  if(argc>=2){
	if(**++argv != '-' || (pnum = (*argv)[1]-'0') < 0 || pnum > 4)
	  die("bad args\n");
  }

  if(pnum<=0){				/* if this is pass 0 */
	/* check header of compressed file */
	if(mygetc() != 0x1F || mygetc() != 0x9D)/* check magic number */
	  die("not a compressed file\n");
	iindex=mygetc();			/* get compression style */
  } else getpipe();				/* get compression style */
  maxbits = iindex&0x1F;
  clearflg = (iindex&0x80) != 0;
  if(maxbits<9 || maxbits>16)			/* check for valid maxbits */
    die("can't decompress\n");
  if((pnum & ~3) == 0) putpipe(iindex, 0);	/* pass style to next copy */

  if(pnum<0){			/* if decompr should set up pipeline */
	/* fork copies and setup pipeline */
	for(pnum=0; pnum<4; pnum++){		/* do four forks */
		if(pipe(fildes)) die("pipe() error\n");
		if((i=fork()) == -1) die("fork() error\n");
		if(i){				/* if this is the copy */
			pipeout = fildes[1];
			break;
		}
		/* if this is the original */
		pipein = fildes[0];
		close(fildes[1]);		/* don't accumulate these */
	}
  }

  /* Preliminary inits. Note: end/maxend/curend are highest, not highest+1 */
  base = DICTSZ*(4-pnum) + 256, locend = base+DICTSZ-1;
  maxend = (1<<maxbits)-1;
  if(maxend>locend) maxend=locend;

  for(;;){
	curend = 255+clearflg;		/* init dictionary */
	dcharp = DICTSZ;		/* flag for none needed */
	curbits = 9;			/* init curbits (for copy 0) */
	for(;;){	/* for each index in input */
		if(pnum) getpipe();	/* get next index */
		else{			/* get index using getbits() */
			if(curbits<maxbits && (1<<curbits)<=curend){
				/* curbits needs to be increased */
				/* due to uglyness in compress, these indices
				 * in the compressed file are wasted */
				while(inmod) getbits();
				curbits++;
			}
			getbits();
		}
		if(iindex==256 && clearflg){
			if(pnum<4) putpipe(iindex, 0);
			/* due to uglyness in compress, these indices
			 * in the compressed file are wasted */
			while(inmod) getbits();
			break;
		}
		tindex=iindex;
		/* convert the index part, ignoring spawned chars */
		while(tindex>=base) tindex = dindex[tindex-base];
		putpipe(tindex, 0);	/* pass on the index */
		/* save the char of the last added entry, if any */
		if(dcharp<DICTSZ) dchar[dcharp++] = tindex;
		if(curend<maxend)	/* if still building dictionary */
		  if(++curend >= base)
		    dindex[dcharp=(curend-base)] = iindex;
		/* Do spawned chars. They are naturally produced in the wrong
		 * order. To get them in the right order without using memory,
		 * a series of passes, progressively less deep, are used */
		tbase = base;
		while((tindex=iindex) >= tbase){/* for each char to spawn */
			while((tindex2=dindex[tindex-base]) >= tbase)
			  tindex=tindex2;	/* scan to desired char */
			putpipe(dchar[tindex-base], 1);/* put it to the pipe */
			tbase=tindex+1;
		}
	}
  }
}

/* If s, write to stderr. Flush buffers if needed. Then exit. */
die(s) char *s; {
  static int recurseflag=0;
  if(recurseflag++) exit(1);
  if(s) while(*s) write(2, s++, 1);
  if(obufp) write(1, obuf, obufp);	   /* flush stdout buf if needed */
  do putpipe(EOF_INDEX, 0); while(opbufp); /* flush pipe if needed */
  exit(s ? 1 : 0);
}

/* Put a char to stdout. */
myputc(c){
  obuf[obufp++] = c;
  if(obufp >= BUFSZ){			/* if stdout buf full */
	write(1, obuf, BUFSZ);		/*   flush to stdout */
	obufp=0;
  }
}

/* Get a char from stdin. If EOF, then die() and exit. */
mygetc(){
  unsigned u;
  if(ibufp >= ibufe){		/* if stdin buf empty */
	if((ibufe = read(0, ibuf, BUFSZ)) <= 0)
	  die(0);		/* if EOF, do normal exit */
	ibufp=0;
  }
  return(ibuf[ibufp++]);
}

/* Put curbits bits into index from stdin. Note: only copy 0 uses this.
 * The bits within a byte are in the correct order. But when the bits
 * cross a byte boundry, the lowest bits will be in the higher part of
 * the current byte, and the higher bits will be in the lower part of
 * the next byte. */
getbits(){
  int have;
  static unsigned curbyte;	/* byte having bits extracted from it */
  static int left;		/* how many bits are left in curbyte */

  inmod = (inmod+1) & 7;	/* count input mod 8 */
  iindex=curbyte, have=left;
  if(curbits-have > 8) iindex |= (unsigned)mygetc() << have, have+=8;
  iindex |= ((curbyte=mygetc()) << have) & ~((unsigned)0xFFFF << curbits);
  curbyte >>= curbits-have, left = 8 - (curbits-have);
}

/* Get an index from the pipeline. If flagged firstonly, handle it here. */
getpipe(){
  static short flags;
  static int n=0;	/* number of flags in flags */

  for(;;){		/* while index with firstonly flag set */
	if(n<=0){
		if(ipbufp >= BUFSZ_2){	/* if pipe input buf empty */
			if(read(pipein, (char*)ipbuf, BUFSZ) != BUFSZ)
			  die("bad pipe read\n");
			ipbufp=0;
		}
		flags = ipbuf[ipbufp++];
		n = 15;
	}
	iindex = ipbuf[ipbufp++];
	if(iindex>curend)
	  die(iindex == EOF_INDEX ? 0 : "invalid data\n");
	flags<<=1, n--;
	/* assume flags<0 if highest remaining flag is set */
	if(flags>=0)		/* if firstonly flag for index is not set */
	  return;		/* return with valid non-firstonly index */
	/* handle firstonly case here */
	while(iindex>=base) iindex = dindex[iindex-base];
	putpipe(iindex, 1);
  }
}

/* put an index to the pipeline */
putpipe(u, flag) unsigned u; {
  static unsigned short flags, *flagp;
  static int n=0;	/* number of flags in flags */

  if(pnum==4){		/* if we should write to stdout */
	myputc(u);	/* index will BE the char value */
	return;
  }

  if(n==0)			/* if we need to reserve a flag entry */
    flags=0, flagp = opbuf + opbufp++;
  opbuf[opbufp++] = u;		/* add index to buffer */
  flags = (flags<<1) | flag;	/* add firstonly flag */
  if(++n >= 15){		/* if block of 15 indicies */
	n=0, *flagp=flags;	/*   insert flags entry */
	if(opbufp >= BUFSZ_2){	/* if pipe out buf full */
		opbufp=0;
		if(write(pipeout, (char*)opbuf, BUFSZ) != BUFSZ)
		  die("bad pipe write\n");
	}
  }
}
