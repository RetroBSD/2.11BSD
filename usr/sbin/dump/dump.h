/*
 * "@(#)dump.h	1.2 (2.11BSD GTE) 12/6/94"
 */
#define	NI	4	/* number of blocks of inodes per read */

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <grp.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <utmp.h>
#include <protocols/dumprestor.h>
#include <fstab.h>

#define	MWORD(m,i)	(m[(unsigned)(i-1)/MLEN])
#define	MBIT(i)		(1<<((unsigned)(i-1)%MLEN))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

short	clrmap[MSIZ];
short	dirmap[MSIZ];
short	nodmap[MSIZ];

/*
 *	All calculations done in 0.1" units!
 */

char	*disk;		/* name of the disk file */
char	*tape;		/* name of the tape file */
char	*increm;	/* name of the file containing incremental information*/
char	incno;		/* increment number */
int	uflag;		/* update flag */
int	fi;		/* disk file descriptor */
int	to;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
ino_t	ino;		/* current inumber; used globally */
int	lastlevel;
int	nonodump;
int	nsubdir;
int	newtape;	/* new tape flag */
int	nadded;		/* number of added sub directories */
int	dadded;		/* directory added flag */
u_short	density;	/* density in 0.1" units */
long	tsize;		/* tape size in 0.1" units */
long	esize;		/* estimated tape size, blocks */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */

int	notify;		/* notify operator flag */
long	blockswritten;	/* number of blocks written on current tape */
int	tapeno;		/* current tape number */
time_t	tstart_writing;	/* when started writing the first tape block */

time_t	time();
off_t	lseek();
char	*ctime();
char	*prdate();
long	atol();
int	mark();
int	add();
int	dump();
int	tapsrec();
int	dmpspc();
int	dsrch();
int	nullf();
char	*rawname();

int	interrupt();		/* in case operator bangs on console */

#define	HOUR	(60L*60L)
#define	DAY	(24L*HOUR)
#define	YEAR	(365L*DAY)

/*
 *	Exit status codes
 */
#define	X_FINOK		1	/* normal exit */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort all of dump; don't attempt checkpointing*/

#ifdef DEBUG
#define	OINCREM	"./ddate"		/*old format incremental info*/
#define	NINCREM	"./dumpdates"		/*new format incremental info*/
#else not DEBUG
#define	OINCREM	"/etc/ddate"		/*old format incremental info*/
#define	NINCREM	"/etc/dumpdates"	/*new format incremental info*/
#endif

#define	TAPE	"/dev/rmt8"		/* default tape device */
#define	DISK	"/dev/rxp0a"		/* default disk */
#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

/*
 *	The contents of the file NINCREM is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
struct	itime{
	struct	idates	it_value;
	struct	itime	*it_next;
};
struct	itime	*ithead;	/* head of the list version */
int	nidates;		/* number of records (might be zero) */
int	idates_in;		/* we have read the increment file */
struct	idates	**idatev;	/* the arrayfied version */
#define	ITITERATE(i, ip) for (i = 0,ip = idatev[0]; i < nidates; i++, ip = idatev[i])

/*
 *	We catch these interrupts
 */
int	sighup();
int	sigquit();
int	sigill();
int	sigtrap();
int	sigfpe();
int	sigkill();
int	sigbus();
int	sigsegv();
int	sigsys();
int	sigalrm();
int	sigterm();
