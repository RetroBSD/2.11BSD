/*
 *	U N I X   2 . 9 B S D   C R A S H   A N A L Y Z E R   S U B S
 *
 * More proc struct flag changes.  No progress (no effort either ;)) on
 * getting the program to compile - 1999/9/14
 *
 * Another proc struct flag went away.  Program still doesn't run or
 * compile ;-(  1999/8/11
 *
 * The proc structure flags cleaned up.  This program still doesn't run
 * (or compile) under the current system.  1997/9/2
 *
 * All the tty delay bits went away.  1997/3/28
 *
 * 'LCASE' and 'LTILDE' went away.  Some day this program should be
 * rewritten to reflect the current system.  12/9/94
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/inode.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <pwd.h>
#include <grp.h>
#include "crash.h"

/*
 * structure to access a word as bytes
 */
struct byteof {
	char	lobyte;
	char	hibyte;
};

/* These globals are used in crash.c */
char	*subhead;			/* pntr to sub-heading */
int	line;				/* current line number */

/*
 *			S T R O C T
 *
 *	stroct		Mark Kampe	7/2/75
 *	returns:	int
 *	takes:		*char
 *	it does:	If string represents an octal integer,
 *			the value of that integer is returned.
 *			If not, a zero is returned.
 *			The string should be null terminated and
 *			contain no non-octal-numerics.
 */
int stroct(s1)
	char *s1;
{
	register char *p;
	register char thisn;
	register int value;

	p = s1;
	value = 0;
	while (thisn = *p++)
		if ((thisn >= '0') && (thisn <= '7')) {
			value <<= 3;
			value += thisn - '0';
		}
		else return(0);

	return(value);
}

/*
 *			O C T O U T
 *
 *
 *	octout		Mark Kampe	7/2/75
 *	returns:	nothing
 *	takes:		int
 *	it does:	print the integer as a six digit
 *			octal number with leading zeroes
 *			as required.  I wrote this because
 *			I found the octal output produced
 *			by printf to be very hard to read.
 *			maybe I'm a pervert, but I like the leading
 *			zeroes.  If you dont, replace this
 *			routine with "printf("%6o",arg);"
 *
 */
octout(value)
register unsigned value;
{
	char outbuf[7];
	register char *c;
	register int i;

	c = &outbuf[5];

	for(i = 0; i<6; i++) {
		*c-- = (value & 07) + '0';
		value >>= 3;
	}

	outbuf[6] = 0;		/* Null terminate string */
	printf("%s", outbuf );
	return;
}
/*
 *			L O C T O U T
 *
 *
 *	loctout		John Stewart 3 Mar 83
 *	returns:	nothing
 *	takes:		long int
 *	it does:	print the integer as an eleven digit
 *			octal number with leading zeroes
 *			as required.  (See octout, above.)
 *
 */
loctout(value)
long value;
{
	char outbuf[12];
	register char *c;
	register int i;

	c = &outbuf[10];

	for(i = 0; i<10; i++) {
		*c-- = (value & 07) + '0';
		value >>= 3;
	}
	/* no unsigned long on this machine */
	*outbuf = (value & 03) + '0';

	outbuf[11] = 0;		/* Null terminate string */
	printf("%s", outbuf );
	return;
}

hexout(value)
unsigned value;
{
	hexdump((long)value, 4);
}

hexdump(value, digs)
long value;
register int digs;
{
	char outbuf[12];
	register char *c;
	register int i;

	c = &outbuf[digs];
	*c-- = 0;		/* null terminate */
	for(; digs--;) {
		*c-- = "0123456789ABCDEF"[value & 0x0f];
		value >>= 4;
	}

	printf("%s", outbuf );
	return;
}

/*
 *			B A R F
 *
 *
 *	Print a diagnostic, flush buffers, and exit
 */
barf( c1 )
register char *c1;
{
	printf("%s\n", c1);
	exit(10);
}

/*
 *			N E W P A G E
 *
 *
 *	New page processor
 */
newpage() {
	static int	page = 0;			/* current page number */

	page++;
	line = 0;
	putchar( NEWPAGE );

	printf("\n\t\t* * * UNIX Crash Dump Analyzer * * *\t\tPage %d\n\n",
		page);
	printf("\t\t\t%s\n\n", subhead);
}


/*
 *			S H O W
 *
 *	This routine takes an address and a format type, and arranges
 * to have the value at the given address printed in the appropriate
 * format.
 *	Mike Muuss, 6/28/77.
 */

show( addr, fmt )
register unsigned *addr;
register fmt;
{
	register char *byte;
	int	i,j;

	switch( fmt ) {

	/* Special code to just return */
	case IGNORE:
		return;

	/* Octal.  Use Mark Kampe's nice routine */
	case OCT:
		octout( *addr );
		return;

	/* Long Octal.  Use John Stewart's nice routine */
	case LONGOCT:
		loctout(*(long *)addr);
		return;

	case HEXL:
		hexdump(*(long *)addr, 8);
		return;
	case HEXW:
		hexdump((long)*(int *)addr, 4);
		return;
	case HEXB:
		hexdump((long)*(char *)addr, 2);
		return;
	
	/* Interupt Address Symbolicaly */
	case TADDR:
		symbol( *addr, ISYM, 0 );
		return;
	case DADDR:
		symbol( *addr, DSYM, 0 );
		return;


	/* Decimal.  Append a dot for ease of reading */
	case DEC:
		printf("%6d.", *addr);
		return;

	/* Unsigned Decimal */
	case UDEC:
		printf("%6u.", *addr);
		return;

	/* Show both halves of the word */
	case DEV:
		printf("%2d./%2d.", ((struct byteof *)addr)->hibyte,
			(((struct byteof *)addr)->lobyte)&0377);
		return;

	/* Show the byte */
	case ONEBYTE:
		byte = (char *) addr;		/* better safe than sorry */
		printf("%6o", (*byte) & 0377 );
		return;

	/* Show printable characters */
	case CHARS:
		byte = (char *) addr;
		for( i=0; i < CBSIZE; i++ )  {
			j = *byte++ & 0377;
			if( (j<' ') || (j>'~') )
				printf("0%o", j);
			else
				putchar( j );
			putchar(' ');
		}
		return;

	/* Show the byte in decimal */
	case HALFDEC:
		byte = (char *) addr;
		j = *byte & 0377;
		printf("%d.", j);
		return;

	/* Show the long in decimal */
	case LONGDEC:
		printf("%ld.", *((long *)addr));
		return;

	/* Just in case */
	default:
		printf("I can't show that!");
		return;

	}
}

/*
 *			P U T B I T S
 *
 *	This routine accepts a table of 16 strings representing each
 * bit in a word.  For every bit set in the given word, the
 * associated message is printed.
 *	Mike Muuss, 6/28/77.
 */

putbits( array, word )
register char *array[];
register int word;
{
	register int i;			/* current bit # */

	for( i=0; i<16; i++)
		if( (word >> i) & 01 )  printf("%s", array[i] );
}

/*
 *			C O L
 *
 *	Print a region in nice columns
 *
 *	Each line is preceeded with 'indent' spaces,
 * and 8 octal values are printed on each line.  Output
 * comences at 'base' and proceeds for 'offset' words.
 */

col( indent, base, offset )
register unsigned *base;
register offset;
{
	register i;			/* Current offset */
	int	here;			/* Current line pointer */
	int	j;			/* indent counter */

	here = 50;			/* force end of line */

	for( i=0; i < offset; i++ ) {
		if( ++here >= 8 ) {
			/* End of Line */
			here = 0;
			putchar( '\n' );
			j = indent;
			while( j-- )  putchar( ' ' );
		}
		else putchar ('\t');
		octout( *( base + i ) );
	}
}


/*
 *			E Q U A L
 *
 *
 *	Determine if the first 8 characters (or up to NULL if
 * shorter than 8 chars) are equal.  True return if yes.
 */

equal( a, b )
register char *a, *b;
{
	register i;
	register wrong;

	wrong = 0;
	for( i=0; i < 8; i++ ) {
		if( !*a && !*b ) break;
		if( *a++ != *b++ )  wrong++;
	}
	if( wrong ) return( 0 );			/* mismatch */
	return( 1 );					/* match */
}


/*
 *			P R I N T S
 *
 *   This function converts the 'number' argument into decimal
 * and outprintf it at location 'pointer'.  Leading zeros are
 * suppressed.
 *	Mike Muuss, 7/8/77.
 */

char *
prints( pointer, number )
register char *pointer;
register int  number;
{
	register left;			/* whats left */

	if( left = number/10 )  pointer = prints( pointer, left );
	*pointer++ = ( number%10 ) + '0';
	*pointer = 0;			/* string terminator */
	return( pointer );
}

/*
 *			D I S P L A Y
 *
 *	This routine takes a structure of 'display' format,
 * and generates output for each element until an END element
 * is encountered.  The offset field is added to the base
 * address of each structure element, in case this routine
 * is being repeatedly called to display various structure
 * elements from the core dump.
 *	Mike Muuss, 6/27/77.
 */

display( table, offset )
register struct display *table;
register offset;
{

	while( table -> fmt ) {
		/* Display Prefix */
		printf("%s:\t", table -> msg );

		/*
		 *  Format item
		 *
		 * offset is taken to be a byte pointer
		 * place is now defined as a byte pointer
		 */
		show((unsigned *)((table->place)+offset), table->fmt);
		if (table->routine)
			(*table->routine)((table->place)+offset);
		table++;
	}
}

/*
 *	P R I N T D E V
 *
 * This routine takes a pointer to a mount structure and spins through
 * the /dev/directory look for a matching file with the same major/minor
 * numbers. It also prints out the plain file name from the super-block.
 *
 */

#define	DEVDIR "/dev"

printdev(mp)
struct mount *mp;
{
	struct stat stb;
	register struct direct *dirent;
	struct DIR *dirfd;
	char filename[20];

	dirfd = opendir(DEVDIR);
	while ((dirent = readdir(dirfd)) != NULL) {
		if (dirent->d_ino) {
			sprintf(filename, "%s/%.*s", DEVDIR,
				dirent->d_namlen, dirent->d_name);
			if (stat(filename, &stb) == 0 &&
			    (stb.st_mode & S_IFMT) == S_IFBLK &&
			    stb.st_rdev == mp->m_dev) {
				printf("\t%s\t%s\n", filename,
					mp->m_filsys.fs_fsmnt);
				break;
			}
		}
	}
	closedir(dirfd);
}

/*
 * Print a value a la the %b format of the kernel's printf
 */
printb(v, bits)
	u_long v;
	register char *bits;
{
	register int i, any = 0;
	register char c;

	if (v && bits)
		if (*bits == 8)
			printf("0%lo=", v);
		else
			if (*bits == 16)
				printf("0x%lx=", v);
			else
				putchar(' ');
	bits++;
	if (v && bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

/*
 * Print out information about the PDR passed as argument
 */

char *acffld[] = {
	"non-resident",
	"read-only",
	"read-only",
	"unused",
	"read/write",
	"read/write",
	"read/write",
	"unused"
};

pdrprint(pdr)
unsigned *pdr;
{
	char plf, a, w, ed, acf;

	plf = (*pdr & 077400) >> 8;
	a = (*pdr & 0200) >> 7;
	w = (*pdr & 0100) >> 6;
	ed = (*pdr & 010) >> 3;
	acf = (*pdr & 07);
	printf(" plf: %d.%s%s%s acf: 0%o %s", plf, (a ? " A" : ""),
	      (w ? " W" : ""), (ed ? " ED" : ""), acf, acffld[acf]);
}

/*
 * Print the uid passed
 */

printuid(uid)
unsigned *uid;
{
	struct passwd *pwd, *getpwuid();

	pwd = getpwuid(*uid);
	if (pwd)
		printf("\t%s", pwd->pw_name);
}

/*
 * Print the gid passed
 */

printgid(gid)
unsigned *gid;
{
	struct group *gwd, *getgrgid();

	gwd = getgrgid(*gid);
	if (gwd)
		printf("\t%s", gwd->gr_name);
}

/*
 * Print out sinals by name.
 */

prtsig(sigm)
long *sigm;
{
#define	SIGM_FLAGS "\0\1HUP\2INT\3QUIT\4ILL\5TRAP\6IOT\7EMT\10FPE\11KILL\12BUS\13SEGV\14SYS\15PIPE\16ALRM\17TERM\20URG\21STOP\22TSTP\23CONT\24CHLD\25TTIN\26TTOU\27IO\30XCPU\31XFSZ\32VTALRM\33PROF\34WINCH\36USR1\37USR2"
	printb((u_long) *sigm, SIGM_FLAGS);
}

/*
 * Print out flags in the proc structure
 */

procflg(flgs)
int *flgs;
{
#define	PROC_FLAGS "\0\1SLOAD\2SSYS\3SLOCK\4SSWAP\5P_TRACED\6P_WAITED\7SULOCK\8P_SINTR\11SVFORK\12SVFPRNT\13SVFDONE\15P_TIMEOUT\16P_NOCLDSTOP\17P_SELECT"
	printb((u_long) *flgs, PROC_FLAGS);
}

/*
 * Print out the flags in the tty structure
 */

ttyflg(flgs)
unsigned *flgs;
{
#define	TTY_FLAGS "\0\1TANDEM\2CBREAK\4ECHO\5CRMOD\6RAW\7ODDP\10EVENP\21CRTBS\22PRTERA\23CRTERA\25MDMBUF\26LITOUT\27TOSTOP\30FLUSHO\31NOHANG\32RTSCTS\33CRTKILL\34PASS8\35CTLECH\36PENDIN\DECCTQ"
	printb((u_long) *flgs, TTY_FLAGS);
}

ttystat(flgs)
unsigned *flgs;
{
#define	TTY_STATE "\0\1TIMEOUT\2WOPEN\3ISOPEN\4FLUSH\5CARR_ON\6BUSY\7ASLEEP\10XCLUDE\11TTSTOP\12HUPCLS\13TBLOCK\14RCOLL\15WCOLL\16NBIO\17ASYNC\21BKSL\22QUOT\23ERASE\24LNCH\25TYPEN\26CNTTB"
	printb((u_long) *flgs, TTY_STATE);
}

/*
 * Ths routine prints out the flags in the buf structure
 */

bufflg(flgs)
unsigned *flgs;
{
#define	BUF_FLAGS "\0\1READ\2DONE\3ERROR\4BUSY\5PHYS\6MAP\7WANTED\10AGE\11ASYNC\12DELWRI\13TAPE\14INVAL\15bad\16RH70\17UBAREMAP\18RAMREMAP"
	printb((u_long) *flgs, BUF_FLAGS);
}

/*
 * This routine prints out the flags in the inode structure
 */

inoflg(flgs)
unsigned *flgs;
{
#define	INO_FLAGS "\0\1ILOCKED\2IUPD\3IACC\4IMOUNT\5IWANT\6ITEXT\7ICHG\10SHLOCK\11IEXLOCK\12ILWAIT\13IMOD\14IRENAME\15IPIPE\20IXMOD"
	printb((u_long) *flgs, INO_FLAGS);
}

/*
 * This routine prints out the file mode from the inode structure
 */

inomod(ip)
struct inode *ip;
{
	char special = 0;
	unsigned dev;

	show(&ip->i_mode, OCT);
	switch (ip->i_mode & IFMT) {
		case IFDIR:
				printf(" (Directory)");
				break;
		case IFCHR:
				special++;
				printf(" (Character Device)");
				break;
		case IFBLK:
				special++;
				printf(" (Block Device)");
				break;
		case IFREG:
				printf(" (Regular)");
				break;
		case IFLNK:
				printf(" (Symbolic Link)");
				break;
		case IFSOCK:
				printf(" (Socket)");
				break;
		case 0:	
				printf(" (Free-inode)");
				break;
		default:
				printf(" (Garbage!)");
				break;
	}
	if (special) {
		printf("\tdev:\t");
		dev = (unsigned) ip->i_rdev;
		show(&dev, DEV);
		putchar('\n');
	} else {
		printf("\tsize:\t");
		show(&ip->i_size, LONGDEC);
		printf("\n\t\tlastr:\t");
		show(&ip->i_lastr, LONGDEC);
		printf("\taddr[0]:\t");
		show(&ip->i_addr[0], LONGDEC);
	}
	putchar('\n');
}

/*
 * Print out the processor status in a symbolic format
 */

char *mode[] = {
	"kernel",
	"supervisor",
	"illegal",
	"user"
};

printps(ps)
unsigned ps;
{
	char curmode, prevmode, cis, prio, t, n, z, v, c;

	curmode = (ps >> 14) & 03;
	prevmode = (ps >> 12) & 03;
	cis = (ps >> 8) & 01;
	prio = (ps >> 5) & 07;
	t = (ps >> 4) & 01;
	n = (ps >> 3) & 01;
	z = (ps >> 2) & 01;
	v = (ps >> 1) & 01;
	c = ps & 01;

	printf(" <%s %s%s spl: %d.%s%s%s%s%s>", mode[curmode], mode[prevmode],
		(cis ? " CIS" : ""), prio, (t ? " T" : ""), (n ? " N" : ""),
		(z ? " Z" : ""), (v ? " V" : ""), (c ? " C" : ""));
}
