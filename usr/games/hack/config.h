/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* config.h - version 1.0.3 */

#ifndef CONFIG	/* make sure the compiler doesnt see the typedefs twice */

#define	CONFIG
#define	UNIX		/* delete if no fork(), exec() available */
#define	CHDIR		/* delete if no chdir() available */

/*
 * Some include files are in a different place under SYSV
 * 	BSD		   SYSV
 * <strings.h>		<string.h>
 * <sys/wait.h>		<wait.h>
 * <sys/time.h>		<time.h>
 * <sgtty.h>		<termio.h>
 * Some routines are called differently
 * index		strchr
 * rindex		strrchr
 * Also, the code for suspend and various ioctls is only given for BSD4.2
 * (I do not have access to a SYSV system.)
 */
#define BSD		/* delete this line on System V */

/*
 * On V7 / 2.9BSD / Ultrix 2.0 and other V7 look-a-likes, a signal trap during
 * a read from the tty will cause the read to terminate prematurely with an
 * error (EOF) when the signal trap returns.  Note that on most version 7
 * type systems, defining `BSD' (above) in addtion to VERSION7 is appropriate.
 */

#define	VERSION7

/* #define STUPID */	/* avoid some complicated expressions if
			   your C compiler chokes on them */
/* #define PYRAMID_BUG */	/* avoid a bug on the Pyramid */
/* #define NOWAITINCLUDE */	/* neither <wait.h> nor <sys/wait.h> exists */

/* #define WIZARD  "casey"	/* the person allowed to use the -D option */
#define RECORD	"record"/* the file containing the list of topscorers */
#define	NEWS	"news"	/* the file containing the latest hack news */
#define	HELP	"help"	/* the file containing a description of the commands */
#define	SHELP	"hh"	/* abbreviated form of the same */
#define	RUMORFILE	"rumors"	/* a file with fortune cookies */
#define	DATAFILE	"data"	/* a file giving the meaning of symbols used */
#define	FMASK	0660	/* file creation mask */
#define	HLOCK	"perm"	/* an empty file used for locking purposes */
#define LLOCK	"safelock"	/* link to previous */

#ifdef UNIX
/*
 * Define DEF_PAGER as your default pager, e.g. "/bin/cat" or "/usr/ucb/more"
 * If defined, it can be overridden by the environment variable PAGER.
 * Hack will use its internal pager if DEF_PAGER is not defined.
 * (This might be preferable for security reasons.)
 * #define DEF_PAGER	".../mydir/mypager"
 */

/*
 * If you define MAIL, then the player will be notified of new mail
 * when it arrives. If you also define DEF_MAILREADER then this will
 * be the default mail reader, and can be overridden by the environment
 * variable MAILREADER; otherwise an internal pager will be used.
 * A stat system call is done on the mailbox every MAILCKFREQ moves.
 */
#define	MAIL
#define	DEF_MAILREADER	"/usr/ucb/mail"		/* or e.g. /bin/mail */
#define	MAILCKFREQ	100


#define SHELL		/* do not delete the '!' command */

#ifdef BSD
#define	SUSPEND		/* let ^Z suspend the game */
#endif BSD
#endif UNIX

#ifdef CHDIR
/*
 * If you define HACKDIR, then this will be the default playground;
 * otherwise it will be the current directory.
 */
#define HACKDIR	"/usr/games/lib/hackdir"

/*
 * Some system administrators are stupid enough to make Hack suid root
 * or suid daemon, where daemon has other powers besides that of reading or
 * writing Hack files. In such cases one should be careful with chdir's
 * since the user might create files in a directory of his choice.
 * Of course SECURE is meaningful only if HACKDIR is defined.
 */
#define SECURE			/* do setuid(getuid()) after chdir() */

/*
 * If it is desirable to limit the number of people that can play Hack
 * simultaneously, define HACKDIR, SECURE and MAX_NR_OF_PLAYERS.
 * #define MAX_NR_OF_PLAYERS	6
 */
#endif CHDIR

/* size of terminal screen is (at least) (ROWNO+2) by COLNO */
#define	COLNO	80
#define	ROWNO	22

/*
 * small signed integers (8 bits suffice)
 *	typedef	char	schar;
 * will do when you have signed characters; otherwise use
 *	typedef	short int schar;
 */
typedef	char	schar;

/*
 * small unsigned integers (8 bits suffice - but 7 bits do not)
 * - these are usually object types; be careful with inequalities! -
 *	typedef	unsigned char	uchar;
 * will be satisfactory if you have an "unsigned char" type; otherwise use
 *	typedef unsigned short int uchar;
 */
typedef	unsigned char	uchar;

/*
 * small integers in the range 0 - 127, usually coordinates
 * although they are nonnegative they must not be declared unsigned
 * since otherwise comparisons with signed quantities are done incorrectly
 */
typedef schar	xchar;
typedef	xchar	boolean;		/* 0 or 1 */
#define	TRUE	1
#define	FALSE	0

/*
 * Declaration of bitfields in various structs; if your C compiler
 * doesnt handle bitfields well, e.g., if it is unable to initialize
 * structs containing bitfields, then you might use
 *	#define Bitfield(x,n)	uchar x
 * since the bitfields used never have more than 7 bits. (Most have 1 bit.)
 *
 * Bitfields declared as `Bitfield' are subject to compile time initialization
 * Those declared as `EasyBitfield' are not (so if you compiler's only problem
 * with bit fields is that it doesn't like initializing them, leave
 * `EasyBitfield' as `unsigned x:n'
 */
#define	Bitfield(x,n)		uchar x
#define EasyBitfield(x,n)	unsigned x:n

#define	SIZE(x)	(int)(sizeof(x) / sizeof(x[0]))

#ifdef	VERSION7
#  define	STRLEN(quoted)	strlen(quoted)
#else
#  define	STRLEN(quoted)	(sizeof(quoted)-1)
#endif

/*
 * Local configuration choices:
 */
/*#define	QUEST			/* compile quest instead of hack */
/*#define	GOLD_ON_BOTL		/* current gold on bottom line */
#define	EXP_ON_BOTL		/* current experience points on bottom line */
/*#define	WAN_PROBING		/* wand of monster probing */
/*#define	NOWORM			/* no long worms */
/*#define	NEWSCORING		/* ... */
/*#define	NEWSCR			/* ... */

/*#define	NOSAVEONHANGUP		/* obvious? */
/*#define	PERS_IS_UID		/* well ... */
/*#define	PERMANENT_MAILBOX	/* ... */

/*#define	LINT			/* this is a lint compile */
/*#define	lint			/* ditto */
/*#define	nonsense		/* enable weird code? */
/*#define	NOT_YET_IMPLEMENTED	/* new code */

/*
 * KLUDGE:  C names are too short ...
 */

#define  standoutbeg      SOBEG
#define  standoutend      SOEND
#define  done_intr        DONEINTR
#define  lastwarntime     LWTIME
#define  lastwarnlev      LWLEV
#define  fakecorridor     FAKECORR
#define  corrode_armor    CRRDRMR
#define  corrode_weapon   CRRDWPN
#define  monster_nearby   MONNRBY
#define  monstersym       MONSYM
#define  stoned_texts     STNDTXT
#define  stoned_dialogue  STNDDLG

#endif CONFIG
