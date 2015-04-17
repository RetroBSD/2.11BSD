/*
 *
 *      UNIX debugger - common definitions
 *
 *	1998/4/21 - remove local redefinitions used with ptrace
 *
 *      Layout of a.out file (fsym):
 *
 *	This has changed over time - see a.out.h, sys/exec.h and nlist.h
 *	for the current a.out definition and format.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <sgtty.h>
#include <setjmp.h>
#include <unistd.h>
#include <a.out.h>
#include <sys/ptrace.h>

#define	MAXSYMLEN	32
#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	512

#define BEGIN	{
#define END	}

#define IF	if(
#define THEN	){
#define ELSE	} else {
#define ELIF	} else if (
#define FI	}

#define FOR	for(
#define WHILE	while(
#define DO	){
#define OD	}
#define REP	do{
#define PER	}while(
#define DONE	);
#define LOOP	for(;;){
#define POOL	}

#define ANDF	&&
#define ORF	||

#define LPRMODE "%Q"
#define OFFMODE "+%o"

/*
 * This is the memory resident portion of a symbol.  The only difference
 * between this and a 'nlist' entry is the size of the string offset   By
 * restricting the string table to be less than 64kb we save 2 bytes per
 * symbol - approximately 7.5kb of D space in the case of /unix's symbol
 * table.  In practice this restriction is unlikely to be reached since
 * even the kernel's symbol table of ~28kb only has a strings table of 21kb.
*/

	struct	SYMbol
		{
		u_short	value;
		char	type;
		char	ovno;
		u_short	soff;		/* low order of string table offset */
		};

#define SYMTYPE(symflag) (( symflag>=041 || (symflag>=02 && symflag<=04))\
				?  ((symflag&07)>=3 ? DSYM : (symflag&07))\
				: NSYM)

typedef	char		MSG[];
typedef	struct map	MAP;
typedef	MAP		*MAPPTR;
typedef	struct bkpt	BKPT;
typedef	BKPT		*BKPTR;
typedef	struct user	U;

/* file address maps */
struct map {
	long	b1;
	long	e1;
	long	f1;
	long	bo;
	long	eo;
	long	fo;
	long	b2;
	long	e2;
	long	f2;
	int	ufd;
};

struct bkpt {
	int	loc;
	int	ins;
	int	count;
	int	initcnt;
	char	flag;
	char	ovly;
	char	comm[MAXCOM];
	BKPT	*nxtbkpt;
};

typedef	struct reglist	REGLIST;
typedef	REGLIST		*REGPTR;
struct reglist {
	char	*rname;
	int	roffs;
};

/*
 * Internal variables ---
 *  They are addressed by name. (e.g. (`0'-`9', `a'-`b'))
 *  thus there can only be 36 of them.
 */

#define VARB    11
#define VARC    12      /* current overlay number */
#define VARD    13
#define VARE    14
#define VARM    22
#define VARO    24      /* overlay text segment addition */
#define VARS    28
#define VART    29

#define RD      0
#define WT      1
#define NSP     0
#define ISP     1
#define DSP     2
#define STAR    4
#define DSYM    7
#define ISYM    2
#define ASYM    1
#define NSYM    0
#define ESYM    0177
#define BKPTSET 1
#define BKPTEXEC 2

#define BPT     03

#define NOREG   32767           /* impossible return from getreg() */
#define NREG    9       /* 8 regs + PS from kernel stack */
/*
 * UAR0 is the value used for subprocesses when there is no core file.
 * If it doesn't correspond to reality, use pstat -u on a core file to
 * get uar0, subtract 0140000, and divide by 2 (sizeof int).
 */
#define UAR0    (&corhdr[ctob(USIZE)/sizeof(u_int) - 3]) /* default address of r0 (u.u_ar0) */

#define KR0     (0300/2)       /* location of r0 in kernel dump */
#define KR1     (KR0+1)
#define KR2     (KR0+2)
#define KR3     (KR0+3)
#define KR4     (KR0+4)
#define KR5     (KR0+5)
#define KSP     (KR0+6)
#define KA6     (KR0+7)       /* saved ka6 in kernel dump */

#define MAXOFF  255
#define MAXPOS  80
#define MAXLIN  128

#define TRUE	 (-1)
#define FALSE	0
#define LOBYTE	0377
#define HIBYTE	0177400
#define STRIP	0177

#define SP	' '
#define TB	'\t'
#define EOR     '\n'
#define QUOTE   0200
#define EVEN    (~1)

/* long to ints and back (puns) */
union {
	int     I[2];
	long   L;
} itolws;

#define leng(a)         ((long)((unsigned)(a)))
#define shorten(a)      ((int)(a))
#define itol(a,b)       (itolws.I[0]=(a), itolws.I[1]=(b), itolws.L)

/* result type declarations */
long    inkdot();
struct	SYMbol	*lookupsym();
struct	SYMbol	*symget();
u_int	get(), chkget();
char	*exform();
char	*cache_sym(), *no_cache_sym();
long    round();
BKPTR           scanbkpt();

struct sgttyb adbtty, usrtty;
jmp_buf erradb;
