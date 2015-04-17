/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)tset.c	5.8.2 (2.11BSD GTE) 1997/3/28";
#endif

/*
**  TSET -- set terminal modes
**
**	This program does sophisticated terminal initialization.
**	I recommend that you include it in your .profile or .login
**	file to initialize whatever terminal you are on.
**
**	There are several features:
**
**	A special file or sequence (as controlled by the termcap file)
**	is sent to the terminal.
**
**	Mode bits are set on a per-terminal_type basis (much better
**	than UNIX itself).  This allows special delays, automatic
**	tabs, etc.
**
**	Erase and Kill characters can be set to whatever you want.
**	Default is to change erase to control-H on a terminal which
**	can overstrike, and leave it alone on anything else.  Kill
**	is always left alone unless specifically requested.  These
**	characters can be represented as "^X" meaning control-X;
**	X is any character.
**
**	Terminals which are dialups or plugboard types can be aliased
**	to whatever type you may have in your home or office.  Thus,
**	if you know that when you dial up you will always be on a
**	TI 733, you can specify that fact to tset.  You can represent
**	a type as "?type".  This will ask you what type you want it
**	to be -- if you reply with just a newline, it will default
**	to the type given.
**
**	The current terminal type can be queried.
**
**	Usage:
**		tset [-] [-eC] [-kC] [-iC] [-s] [-r]
**			[-m [ident] [test baudrate] :type]
**			[-Q] [-I] [-S] [type]
**
**		In systems with environments, use:
**			eval `tset -s ...`
**		Actually, this doesn't work in old csh's.
**		Instead, use:
**			tset -s ... > tset.tmp
**			source tset.tmp
**			rm tset.tmp
**		or:
**			set noglob
**			set term=(`tset -S ....`)
**			setenv TERM $term[1]
**			setenv TERMCAP "$term[2]"
**			unset term
**			unset noglob
**
**	Positional Parameters:
**		type -- the terminal type to force.  If this is
**			specified, initialization is for this
**			terminal type.
**
**	Flags:
**		- -- report terminal type.  Whatever type is
**			decided on is reported.  If no other flags
**			are stated, the only affect is to write
**			the terminal type on the standard output.
**		-r -- report to user in addition to other flags.
**		-eC -- set the erase character to C on all terminals.
**			C defaults to control-H.  If not specified,
**			the erase character is untouched; however, if
**			not specified and the erase character is NULL
**			(zero byte), the erase character  is set to delete.
**		-kC -- set the kill character to C on all terminals.
**			Default for C is control-X.  If not specified,
**			the kill character is untouched; however, if
**			not specified and the kill character is NULL
**			(zero byte), the kill character is set to control-U.
**		-iC -- set the interrupt character to C on all terminals.
**			Default for C is control-C.  If not specified, the
**			interrupt character is untouched; however, if
**			not specified and the interrupt character is NULL
**			(zero byte), the interrupt character is set to
**			control-C.
**		-qC -- reserved for setable quit character.
**		-m -- map the system identified type to some user
**			specified type. The mapping can be baud rate
**			dependent.
**			Syntax:	-m identifier [test baudrate] :type
**			where: ``identifier'' is terminal type found in
**			/etc/ttys for this port, (abscence of an identifier
**			matches any identifier); ``test'' may be any combination
**			of  >  =  <  !  @; ``baudrate'' is as with stty(1);
**			``type'' is the actual terminal type to use if the
**			mapping condition is met. Multiple maps are scanned
**			in order and the first match prevails.
**		-n -- If the new tty driver from UCB is available, this flag
**			will activate the new options for erase and kill
**			processing. This will be different for printers
**			and crt's. For crts, if the baud rate is < 1200 then
**			erase and kill don't remove characters from the screen.
**		-s -- output setenv commands for TERM.  This can be
**			used with
**				`tset -s ...`
**			and is to be prefered to:
**				setenv TERM `tset - ...`
**			because -s sets the TERMCAP variable also.
**		-S -- Similar to -s but outputs 2 strings suitable for
**			use in csh .login files as follows:
**				set noglob
**				set term=(`tset -S .....`)
**				setenv TERM $term[1]
**				setenv TERMCAP "$term[2]"
**				unset term
**				unset noglob
**		-Q -- be quiet.  don't output 'Erase set to' etc.
**		-I -- don't do terminal initialization (is & if
**			strings).
**
**	Files:
**		/etc/ttys
**			contains a terminal id -> terminal type
**			mapping; used when any user mapping is specified,
**			or the environment doesn't have TERM set.
**		/etc/termcap
**			a terminal_type -> terminal_capabilities
**			mapping.
**
**	Return Codes:
**		-1 -- couldn't open ttycap.
**		1 -- bad terminal type, or standard output not tty.
**		0 -- ok.
**
**	Defined Constants:
**		BACKSPACE -- control-H, the default for -e.
**		CNTL('X') -- control-X, the default for -k.
**		OLDERASE -- the system default erase character.
**		OLDKILL -- the system default kill character.
**		FILEDES -- the file descriptor to do the operation
**			on, nominally 1 or 2.
**
**	Requires:
**		Routines to handle htmp, ttys, and ttycap.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		Bad flag
**			An incorrect option was specified.
**		Too few args
**			more command line arguments are required.
**		Unexpected arg
**			wrong type of argument was encountered.
**		Cannot open ...
**			The specified file could not be openned.
**		Type ... unknown
**			An unknown terminal type was specified.
**		Cannot update htmp
**			Cannot update htmp file when the standard
**			output is not a terminal.
**		Erase set to ...
**			Telling that the erase character has been
**			set to the specified character.
**		Kill set to ...
**			Ditto for kill
**		Erase is ...    Kill is ...
**			Tells that the erase/kill characters were
**			wierd before, but they are being left as-is.
**		Not a terminal
**			Set if FILEDES is not a terminal.
**
**	Author:
**		Eric Allman
**		Electronics Research Labs
**		U.C. Berkeley
**
**	History:
**		1997/3/28 -- major cleanup.
**		1/81 -- Added alias checking for mapping identifiers.
**		9/80 -- Added UCB_NTTY mods to setup the new tty driver.
**			Added the 'reset ...' invocation.
**		7/80 -- '-S' added. '-m' mapping added. TERMCAP string
**			cleaned up.
**		3/80 -- Changed to use tputs.  Prc & flush added.
**		10/79 -- '-s' option extended to handle TERMCAP
**			variable, set noglob, quote the entry,
**			and know about the Bourne shell.  Terminal
**			initialization moved to before any information
**			output so screen clears would not screw you.
**			'-Q' option added.
**		8/79 -- '-' option alone changed to only output
**			type.  '-s' option added.  'VERSION7'
**			changed to 'V6' for compatibility.
**		12/78 -- modified for eventual migration to VAX/UNIX,
**			so the '-' option is changed to output only
**			the terminal type to STDOUT instead of
**			FILEDES.
**		9/78 -- '-' and '-p' options added (now fully
**			compatible with ttytype!), and spaces are
**			permitted between the -d and the type.
**		8/78 -- The sense of -h and -u were reversed, and the
**			-f flag is dropped -- same effect is available
**			by just stating the terminal type.
**		10/77 -- Written.
*/

#define curerase mode.sg_erase
#define curkill mode.sg_kill
#define curintr tchar.t_intrc
#define olderase oldmode.sg_erase
#define oldkill oldmode.sg_kill
#define oldintr oldtchar.t_intrc

#include	<ttyent.h>
#include	<sgtty.h>
#include	<stdio.h>
#undef	putchar
#include	<errno.h>
#include	<signal.h>
#include	<ctype.h>
#include	<strings.h>
#include	<stdlib.h>
#include	<unistd.h>

#define	YES		1
#define	NO		0
#undef CNTL
#define	CNTL(c)		((c)&037)
#define	BACKSPACE	(CNTL('H'))
#define	CHK(val, dft)	(val<=0 ? dft : val)
#define	OLDERASE	'#'
#define	OLDKILL		'@'
#define	OLDINTR		'\177'	/* del */

/* default special characters */
#ifndef CERASE
#define	CERASE	'\177'
#endif
#ifndef CKILL
#define	CKILL	CNTL('U')
#endif
#ifndef CINTR
#define	CINTR	CNTL('C')
#endif
#ifndef CDSUSP
#define	CQUIT	034		/* FS, ^\ */
#define	CSTART	CNTL('Q')
#define	CSTOP	CNTL('S')
#define	CEOF	CNTL('D')
#define	CEOT	CEOF
#define	CBRK	0377
#define	CSUSP	CNTL('Z')
#define	CDSUSP	CNTL('Y')
#define	CRPRNT	CNTL('R')
#define	CFLUSH	CNTL('O')
#define	CWERASE	CNTL('W')
#define	CLNEXT	CNTL('V')
#endif

#define	FILEDES		2	/* do gtty/stty on this descriptor */

#define	USAGE	"usage: tset [-] [-nrsIQS] [-eC] [-kC] [-iC] [-m [ident][test speed]:type] [type]\n"

#define	DIALUP		"dialup"
#define	PLUGBOARD	"plugboard"
#define	ARPANET		"arpanet"

/*
 * Baud Rate Conditionals
 */
#define	ANY		0
#define	GT		1
#define	EQ		2
#define	LT		4
#define	GE		(GT|EQ)
#define	LE		(LT|EQ)
#define	NE		(GT|LT)
#define	ALL		(GT|EQ|LT)

# define	NMAP		10

struct	map {
	char *Ident;
	char Test;
	char Speed;
	char *Type;
} map[NMAP];

struct map *Map = map;

/* This should be available in an include file */
struct
{
	char	*string;
	int	speed;
	int	baudrate;
} speeds[] = {
	"0",	B0,	0,
	"50",	B50,	50,
	"75",	B75,	75,
	"110",	B110,	110,
	"134",	B134,	134,
	"134.5",B134,	134,
	"150",	B150,	150,
	"200",	B200,	200,
	"300",	B300,	300,
	"600",	B600,	600,
	"1200",	B1200,	1200,
	"1800",	B1800,	1800,
	"2400",	B2400,	2400,
	"4800",	B4800,	4800,
	"9600",	B9600,	9600,
	"19200",EXTA,	19200,
	"exta",	EXTA,	19200,
	"extb",	EXTB,	38400,
	0,
};

char	Erase_char;		/* new erase character */
char	Kill_char;		/* new kill character */
char	Intr_char;		/* new interrupt character */

char	*TtyType;		/* type of terminal */
char	*DefType;		/* default type if none other computed */
char	*NewType;		/* mapping identifier based on old flags */
int	DoSetenv;		/* output setenv commands */
int	BeQuiet;		/* be quiet */
int	NoInit;			/* don't output initialization string */
int	IsReset;		/* invoked as reset */
int	Report;			/* report current type */
int	Ureport;		/* report to user */
int	RepOnly;		/* report only */
int	CmndLine;		/* output full command lines (-s option) */
int	Ask;			/* ask user for termtype */
int	PadBaud;		/* Min rate of padding needed */
int	lines, columns;
short	ospeed;

#define CAPBUFSIZ	1024
char	Capbuf[CAPBUFSIZ];	/* line from /etc/termcap for this TtyType */
char	*Ttycap;		/* termcap line from termcap or environ */
char	*askuser();

struct sgttyb	mode;
struct sgttyb	oldmode;
struct tchars	tchar;
struct tchars	oldtchar;
int	prc();
void	wrtermcap();

extern char	*tgetstr();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		buf[CAPBUFSIZ];
	char		termbuf[32];
	char		*bufp, *ttypath;
	register char	*p;
	char		*command;
	register int	i;
	int		Break;
	int		Not;
	char		*nextarg();
	char		*mapped();
	struct winsize	win;
	char		bs_char;
	int		csh;
	int		settle;
	int		setmode();
	extern char	PC;
	int		lmode;
	int		ldisc;
	struct	ttyent	*t;

	(void) ioctl(FILEDES, TIOCLGET, (char *)&lmode);
	(void) ioctl(FILEDES, TIOCGETD, (char *)&ldisc);

	if (gtty(FILEDES, &mode) < 0)
	{
		prs("Not a terminal\n");
		exit(1);
	}
	bcopy((char *)&mode, (char *)&oldmode, sizeof mode);
	(void) ioctl(FILEDES, TIOCGETC, (char *)&tchar);
	bcopy((char *)&tchar, (char *)&oldtchar, sizeof tchar);
	ospeed = mode.sg_ospeed & 017;
	(void) signal(SIGINT, setmode);
	(void) signal(SIGQUIT, setmode);
	(void) signal(SIGTERM, setmode);

	if (command = rindex(argv[0], '/'))
		command++;
	else
		command = argv[0];
	if (!strcmp(command, "reset") )
	{
	/*
	 * reset the teletype mode bits to a sensible state.
	 * Copied from the program by Kurt Shoens & Mark Horton.
	 * Very useful after crapping out in raw.
	 */
		struct ltchars ltc;

		if (ldisc == NTTYDISC)
		{
			(void) ioctl(FILEDES, TIOCGLTC, (char *)&ltc);
			ltc.t_suspc = CHK(ltc.t_suspc, CSUSP);
			ltc.t_dsuspc = CHK(ltc.t_dsuspc, CDSUSP);
			ltc.t_rprntc = CHK(ltc.t_rprntc, CRPRNT);
			ltc.t_flushc = CHK(ltc.t_flushc, CFLUSH);
			ltc.t_werasc = CHK(ltc.t_werasc, CWERASE);
			ltc.t_lnextc = CHK(ltc.t_lnextc, CLNEXT);
			(void) ioctl(FILEDES, TIOCSLTC, (char *)&ltc);
		}
		tchar.t_intrc = CHK(tchar.t_intrc, CINTR);
		tchar.t_quitc = CHK(tchar.t_quitc, CQUIT);
		tchar.t_startc = CHK(tchar.t_startc, CSTART);
		tchar.t_stopc = CHK(tchar.t_stopc, CSTOP);
		tchar.t_eofc = CHK(tchar.t_eofc, CEOF);
		/* brkc is left alone */
		(void) ioctl(FILEDES, TIOCSETC, (char *)&tchar);
		mode.sg_flags &= ~(RAW | CBREAK);
		mode.sg_flags |= XTABS|ECHO|CRMOD|ANYP;
		curerase = CHK(curerase, CERASE);
		curkill = CHK(curkill, CKILL);
		curintr = CHK(curintr, CINTR);
		BeQuiet = YES;
		IsReset = YES;
	}
	else if (argc == 2 && !strcmp(argv[1], "-"))
		RepOnly = YES;

	argc--;

	/* scan argument list and collect flags */
	while (--argc >= 0)
	{
		p = *++argv;
		if (*p == '-')
		{
			if (*++p == NULL)
				Report = YES; /* report current terminal type */
			else while (*p) switch (*p++)
			{
			  case 'n':
				ldisc = NTTYDISC;
				if (ioctl(FILEDES, TIOCSETD, (char *)&ldisc)<0)
					fatal("ioctl ", "new");
				continue;
			  case 'r':	/* report to user */
				Ureport = YES;
				continue;
			  case 'e':	/* erase character */
				if (*p == NULL)
					Erase_char = -1;
				else
				{
					if (*p == '^' && p[1] != NULL)
						if (*++p == '?')
							Erase_char = '\177';
						else
							Erase_char = CNTL(*p);
					else
						Erase_char = *p;
					p++;
				}
				continue;
			  case 'i':	/* interrupt character */
				if (*p == NULL)
					Intr_char = CNTL('C');
				else
				{
					if (*p == '^' && p[1] != NULL)
						if (*++p == '?')
							Intr_char = '\177';
						else
							Intr_char = CNTL(*p);
					else
						Intr_char = *p;
					p++;
				}
				continue;
			  case 'k':	/* kill character */
				if (*p == NULL)
					Kill_char = CNTL('X');
				else
				{
					if (*p == '^' && p[1] != NULL)
						if (*++p == '?')
							Kill_char = '\177';
						else
							Kill_char = CNTL(*p);
					else
						Kill_char = *p;
					p++;
				}
				continue;
			  case 'd':	/* dialup type */
				NewType = DIALUP;
				goto mapold;
			  case 'p':	/* plugboard type */
				NewType = PLUGBOARD;
				goto mapold;
			  case 'a':	/* arpanet type */
				NewType = ARPANET;
mapold:				Map->Ident = NewType;
				Map->Test = ALL;
				if (*p == NULL)
					p = nextarg(argc--, argv++);
				Map->Type = p;
				Map++;
				p = "";
				continue;
			  case 'm':	/* map identifier to type */
				/* This code is very loose. Almost no
				** syntax checking is done!! However,
				** illegal syntax will only produce
				** weird results.
				*/
				if (*p == NULL)
				{
					p = nextarg(argc--, argv++);
				}
				if (isalnum(*p))
				{
					Map->Ident = p;	/* identifier */
					while (isalnum(*p)) p++;
				}
				else
					Map->Ident = "";
				Break = NO;
				Not = NO;
				while (!Break) switch (*p)
				{
					case NULL:
						p = nextarg(argc--, argv++);
						continue;

					case ':':	/* mapped type */
						*p++ = NULL;
						Break = YES;
						continue;

					case '>':	/* conditional */
						Map->Test |= GT;
						*p++ = NULL;
						continue;

					case '<':	/* conditional */
						Map->Test |= LT;
						*p++ = NULL;
						continue;

					case '=':	/* conditional */
					case '@':
						Map->Test |= EQ;
						*p++ = NULL;
						continue;
					
					case '!':	/* invert conditions */
						Not = ~Not;
						*p++ = NULL;
						continue;

					case 'B':	/* Baud rate */
						p++;
						/* intentional fallthru */
					default:
						if (isdigit(*p) || *p == 'e')
						{
							Map->Speed = baudrate(p);
							while (isalnum(*p) || *p == '.')
								p++;
						}
						else
							Break = YES;
						continue;
				}
				if (Not)	/* invert sense of test */
				{
					Map->Test = (~(Map->Test))&ALL;
				}
				if (*p == NULL)
				{
					p = nextarg(argc--, argv++);
				}
				Map->Type = p;
				p = "";
				Map++;
				continue;
			  case 's':	/* output setenv commands */
				DoSetenv = YES;
				CmndLine = YES;
				continue;
			  case 'S':	/* output setenv strings */
				DoSetenv = YES;
				CmndLine = NO;
				continue;
			  case 'Q':	/* be quiet */
				BeQuiet = YES;
				continue;
			  case 'I':	/* no initialization */
				NoInit = YES;
				continue;
			  default:
				*p-- = NULL;
				fatal("Bad flag -", p);
			}
		}
		else
			DefType = p; 	/* terminal type */
	}

	if (DefType)
	{
		Map->Ident = "";	/* means "map any type" */
		Map->Test = ALL;	/* at all baud rates */
		Map->Type = DefType;	/* to the default type */
	}

	/*
	 * Get rid of $TERMCAP, if it's there, so we get a real
	 * entry from /etc/termcap.  This prevents us from being
	 * fooled by out of date stuff in the environment.
	 */
	unsetenv("TERMCAP");
	/* get current idea of terminal type from environment */
	if	(TtyType)
		goto found;
	if	(TtyType = getenv("TERM"))
		goto map;

	/* Try ttyname(3) if type is current unknown */
	if	(TtyType == 0)
		{
		if	(ttypath = ttyname(FILEDES))
			{
			if	(p = rindex(ttypath, '/'))
				++p;
													else
				p = ttypath;
			if	((t = getttynam(p)))
				TtyType = t->ty_type;
			}
		}
	/* If still undefined, use "unknown" */
	if	(TtyType == 0)
		TtyType = "unknown";

	/* check for dialup or other mapping */
map:
	TtyType = mapped(TtyType);

	/* TtyType now contains a pointer to the type of the terminal */
	/* If the first character is '?', ask the user */
found:
	if	(TtyType[0] == '?')
		{
		if	(TtyType[1] != '\0')
			TtyType = askuser(TtyType + 1);
		else
			TtyType = askuser(NULL);
		}

	/* Find the termcap entry.  If it doesn't exist, ask the user. */
	while	((i = tgetent(Capbuf, TtyType)) == 0)
		{
		(void)fprintf(stderr, "tset: terminal type %s is unknown\n", 
				TtyType);
		TtyType = askuser(NULL);
		}
	if	(i == -1)
		fatal("termcap", strerror(errno ? errno : ENOENT));

	Ttycap = Capbuf;

	if (!RepOnly)
	{
		/* determine erase and kill characters */
		bufp = buf;
		p = tgetstr("kb", &bufp);
		if (p == NULL || p[1] != '\0')
			p = tgetstr("bc", &bufp);
		if (p != NULL && p[1] == '\0')
			bs_char = p[0];
		else if (tgetflag("bs"))
			bs_char = BACKSPACE;
		else
			bs_char = 0;
		if (Erase_char == 0 && !tgetflag("os") && curerase == OLDERASE)
		{
			if (tgetflag("bs") || bs_char != 0)
				Erase_char = -1;
		}
		if (Erase_char < 0)
			Erase_char = (bs_char != 0) ? bs_char : BACKSPACE;

		if (curerase == 0)
			curerase = CERASE;
		if (Erase_char != 0)
			curerase = Erase_char;

		if (curintr == 0)
			curintr = CINTR;
		if (Intr_char != 0)
			curintr = Intr_char;

		if (curkill == 0)
			curkill = CKILL;
		if (Kill_char != 0)
			curkill = Kill_char;

		/* set modes */
		PadBaud = tgetnum("pb");	/* OK if fails */
		for (i=0; speeds[i].string; i++)
			if (speeds[i].baudrate == PadBaud) {
				PadBaud = speeds[i].speed;
				break;
			}
		mode.sg_flags &= ~(EVENP | ODDP | RAW | CBREAK);
		if (tgetflag("EP"))
			mode.sg_flags |= EVENP;
		if (tgetflag("OP"))
			mode.sg_flags |= ODDP;
		if ((mode.sg_flags & (EVENP | ODDP)) == 0)
			mode.sg_flags |= EVENP | ODDP;
		mode.sg_flags |= CRMOD | ECHO | XTABS;
		if (tgetflag("NL"))	/* new line, not line feed */
			mode.sg_flags &= ~CRMOD;
		if (tgetflag("HD"))	/* half duplex */
			mode.sg_flags &= ~ECHO;
		if (tgetflag("pt"))	/* print tabs */
			mode.sg_flags &= ~XTABS;
		if (ldisc == NTTYDISC)
		{
			lmode |= LCTLECH;	/* display ctrl chars */
			if (tgetflag("hc"))
			{	/** set printer modes **/
				lmode &= ~(LCRTBS|LCRTERA|LCRTKIL);
				lmode |= LPRTERA;
			}
			else
			{	/** set crt modes **/
				if (!tgetflag("os"))
				{
					lmode &= ~LPRTERA;
					lmode |= LCRTBS;
					if (mode.sg_ospeed >= B1200)
						lmode |= LCRTERA|LCRTKIL;
				}
			}
		}
		if (IsReset)
			lmode &= ~(LMDMBUF|LLITOUT|LPASS8);
		(void) ioctl(FILEDES, TIOCLSET, (char *)&lmode);

		/* get pad character */
		bufp = buf;
		if (tgetstr("pc", &bufp) != 0)
			PC = buf[0];

		columns = tgetnum("co");
		lines = tgetnum("li");

		/* Set window size */
		(void) ioctl(FILEDES, TIOCGWINSZ, (char *)&win);
		if (win.ws_row == 0 && win.ws_col == 0 &&
		    lines > 0 && columns > 0) {
			win.ws_row = lines;
			win.ws_col = columns;
			(void) ioctl(FILEDES, TIOCSWINSZ, (char *)&win);
		}
		/* output startup string */
		if (!NoInit)
		{
			if (oldmode.sg_flags&(XTABS|CRMOD))
			{
				oldmode.sg_flags &= ~(XTABS|CRMOD);
				setmode(-1);
			}
			if (settabs()) {
				settle = YES;
				flush();
			}
			bufp = buf;
			if (IsReset && tgetstr("rs", &bufp) != 0 || 
			    tgetstr("is", &bufp) != 0)
			{
				tputs(buf, 0, prc);
				settle = YES;
				flush();
			}
			bufp = buf;
			if (IsReset && tgetstr("rf", &bufp) != 0 ||
			    tgetstr("if", &bufp) != 0)
			{
				cat(buf);
				settle = YES;
			}
			if (settle)
			{
				prc('\r');
				flush();
				sleep(1);	/* let terminal settle down */
			}
		}

		setmode(0);	/* set new modes, if they've changed */

		/* set up environment for the shell we are using */
		/* (this code is rather heuristic, checking for $SHELL */
		/* ending in the 3 characters "csh") */
		csh = NO;
		if (DoSetenv)
		{
			char *sh;

			if ((sh = getenv("SHELL")) && (i = strlen(sh)) >= 3)
			{
			    if ((csh = !strcmp(&sh[i-3], "csh")) && CmndLine)
				(void) puts("set noglob;");
			}
			if (!csh)
			    /* running Bourne shell */
			    (void) puts("export TERMCAP TERM;");
		}
	}

	/* report type if appropriate */
	if (DoSetenv || Report || Ureport)
	{
		/*
		 * GET THE TERMINAL TYPE (second name in termcap entry)
		 * See line 182 of 4.4's tset/tset.c
		 */
		if (DoSetenv)
		{
			if (csh)
			{
				if (CmndLine)
				    (void) printf("%s", "setenv TERM ");
				(void) printf("%s ", TtyType);
				if (CmndLine)
				    (void) puts(";");
			}
			else
				(void) printf("TERM=%s;\n", TtyType);
		}
		else if (Report)
			(void) puts(TtyType);
		if (Ureport)
		{
			prs("Terminal type is ");
			prs(TtyType);
			prs("\n");
			flush();
		}

		if (DoSetenv)
		{
			if (csh)
			{
			    if (CmndLine)
				(void) printf("setenv TERMCAP '");
			}
			else
			    (void) printf("TERMCAP='");
			wrtermcap(Ttycap);
			if (csh)
			{
				if (CmndLine)
				{
				    (void) puts("';");
				    (void) puts("unset noglob;");
				}
			}
			else
				(void) puts("';");
		}
	}

	if (RepOnly)
		exit(0);

	/* tell about changing erase, kill and interrupt characters */
	reportek("Erase", curerase, olderase, OLDERASE);
	reportek("Kill", curkill, oldkill, OLDKILL);
	reportek("Interrupt", curintr, oldintr, OLDINTR);
	exit(0);
}

/*
 * Set the hardware tabs on the terminal, using the ct (clear all tabs),
 * st (set one tab) and ch (horizontal cursor addressing) capabilities.
 * This is done before if and is, so they can patch in case we blow this.
 */
settabs()
{
	char caps[100];
	char *capsp = caps;
	char *clear_tabs, *set_tab, *set_column, *set_pos;
	char *tg_out, *tgoto();
	int c;

	clear_tabs = tgetstr("ct", &capsp);
	set_tab = tgetstr("st", &capsp);
	set_column = tgetstr("ch", &capsp);
	if (set_column == 0)
		set_pos = tgetstr("cm", &capsp);

	if (clear_tabs && set_tab) {
		prc('\r');	/* force to be at left margin */
		tputs(clear_tabs, 0, prc);
	}
	if (set_tab) {
		for (c=8; c<columns; c += 8) {
			/* get to that column. */
			tg_out = "OOPS";	/* also returned by tgoto */
			if (set_column)
				tg_out = tgoto(set_column, 0, c);
			if (*tg_out == 'O' && set_pos)
				tg_out = tgoto(set_pos, c, lines-1);
			if (*tg_out != 'O')
				tputs(tg_out, 1, prc);
			else
				prs("        ");
			/* set the tab */
			tputs(set_tab, 0, prc);
		}
		prc('\r');
		return 1;
	}
	return 0;
}

setmode(flag)
int	flag;
/* flag serves several purposes:
 *	if called as the result of a signal, flag will be > 0.
 *	if called from terminal init, flag == -1 means reset "oldmode".
 *	called with flag == 0 at end of normal mode processing.
 */
{
	struct sgttyb *ttymode;
	struct tchars *ttytchars;

	if (flag < 0) { /* unconditionally reset oldmode (called from init) */
		ttymode = &oldmode;
		ttytchars = &oldtchar;
	} else if (bcmp((char *)&mode, (char *)&oldmode, sizeof mode)) {
		ttymode = &mode;
		ttytchars = &tchar;
	} else	{	/* don't need it */
		ttymode = (struct sgttyb *)0;
		ttytchars = (struct tchars *)0;
	}
	
	if (ttymode)
		(void) ioctl(FILEDES, TIOCSETN, (char *)ttymode);
	if (ttytchars)
		(void) ioctl(FILEDES, TIOCSETC, (char *)ttytchars);
	if (flag > 0)	/* trapped signal */
		exit(1);
}

reportek(name, new, old, def)
char	*name;
char	old;
char	new;
char	def;
{
	register char	o;
	register char	n;
	register char	*p;
	char		buf[32];
	char		*bufp;

	if (BeQuiet)
		return;
	o = old;
	n = new;

	if (o == n && n == def)
		return;
	prs(name);
	if (o == n)
		prs(" is ");
	else
		prs(" set to ");
	bufp = buf;
	if (tgetstr("kb", &bufp) > 0 && n == buf[0] && buf[1] == NULL)
		prs("Backspace\n");
	else if (n == 0177)
		prs("Delete\n");
	else
	{
		if (n < 040)
		{
			prs("Ctrl-");
			n ^= 0100;
		}
		p = "x\n";
		p[0] = n;
		prs(p);
	}
	flush();
}

prs(s)
register char	*s;
	{
	while (*s != '\0')
		prc(*s++);
	}


prc(c)
char	c;
	{
	putc(c, stderr);
	}

flush()
{
	fflush(stderr);
}


cat(file)
char	*file;
{
	register int	fd;
	register int	i;
	char		buf[BUFSIZ];

	fd = open(file, 0);
	if (fd < 0)
	{
		prs("Cannot open ");
		prs(file);
		prs("\n");
		flush();
		return;
	}

	while ((i = read(fd, buf, BUFSIZ)) > 0)
		(void) write(FILEDES, buf, i);

	(void) close(fd);
}

/*
 * routine to output the string for the environment TERMCAP variable
 */

void
wrtermcap(bp)
	char *bp;
{
	register int ch;
	register char *p;
	char *t, *sep;

	/* Find the end of the terminal names. */
	if ((t = index(bp, ':')) == NULL)
		fatal("termcap names not colon terminated", "");
	*t++ = '\0';

	/* Output terminal names that don't have whitespace. */
	sep = "";
	while ((p = strsep(&bp, "|")) != NULL)
		if (*p != '\0' && strpbrk(p, " \t") == NULL) {
			(void)printf("%s%s", sep, p);
			sep = "|";
		}
	(void)putchar(':');

	/*
	 * Output fields, transforming any dangerous characters.  Skip
	 * empty fields or fields containing only whitespace.
	 */
	while ((p = strsep(&t, ":")) != NULL) {
		while ((ch = *p) != '\0' && isspace(ch))
			++p;
		if (ch == '\0')
			continue;
		while ((ch = *p++) != '\0')
			switch(ch) {
			case '\033':
				(void)printf("\\E");
			case  ' ':		/* No spaces. */
				(void)printf("\\040");
				break;
			case '!':		/* No csh history chars. */
				(void)printf("\\041");
				break;
			case ',':		/* No csh history chars. */
				(void)printf("\\054");
				break;
			case '"':		/* No quotes. */
				(void)printf("\\042");
				break;
			case '\'':		/* No quotes. */
				(void)printf("\\047");
				break;
			case '`':		/* No quotes. */
				(void)printf("\\140");
				break;
			case '\\':		/* Anything following is OK. */
			case '^':
				(void)putchar(ch);
				if ((ch = *p++) == '\0')
					break;
				/* FALLTHROUGH */
			default:
				(void)putchar(ch);
		}
		(void)putchar(':');
	}
}

baudrate(p)
char	*p;
{
	char buf[8];
	int i = 0;

	while (i < 7 && (isalnum(*p) || *p == '.'))
		buf[i++] = *p++;
	buf[i] = NULL;
	for (i=0; speeds[i].string; i++)
		if (!strcmp(speeds[i].string, buf))
			return (speeds[i].speed);
	return (-1);
}

char *
mapped(type)
char	*type;
{
	int	match;

# ifdef DEB
	printf ("spd:%d\n", ospeed);
	prmap();
# endif
	Map = map;
	while (Map->Ident)
	{
		if (*(Map->Ident) == NULL || !strcmp(Map->Ident, type))
		{
			match = NO;
			switch (Map->Test)
			{
				case ANY:	/* no test specified */
				case ALL:
					match = YES;
					break;
				
				case GT:
					match = (ospeed > Map->Speed);
					break;

				case GE:
					match = (ospeed >= Map->Speed);
					break;

				case EQ:
					match = (ospeed == Map->Speed);
					break;

				case LE:
					match = (ospeed <= Map->Speed);
					break;

				case LT:
					match = (ospeed < Map->Speed);
					break;

				case NE:
					match = (ospeed != Map->Speed);
					break;
			}
			if (match)
				return (Map->Type);
		}
		Map++;
	}
	/* no match found; return given type */
	return (type);
}

# ifdef DEB
prmap()
{
	Map = map;
	while (Map->Ident)
	{
	printf ("%s t:%d s:%d %s\n",
		Map->Ident, Map->Test, Map->Speed, Map->Type);
	Map++;
	}
}
# endif

char *
nextarg(argc, argv)
int	argc;
char	*argv[];
{
	if (argc <= 0)
		fatal ("Too few args: ", *argv);
	if (*(*++argv) == '-')
		fatal ("Unexpected arg: ", *argv);
	return (*argv);
}

fatal (mesg, obj)
	char	*mesg, *obj;
	{

	fprintf(stderr, "%s%s\n%s", mesg, obj, USAGE);
	exit(1);
	}

/* Prompt the user for a terminal type. */
char *
askuser(dflt)
	char *dflt;
{
	static char answer[64];
	char *p;

	/* We can get recalled; if so, don't continue uselessly. */
	if (feof(stdin) || ferror(stdin)) {
		(void)fprintf(stderr, "\n");
		exit(1);
	}
	for (;;) {
		if (dflt)
			(void)fprintf(stderr, "Terminal type? [%s] ", dflt);
		else
			(void)fprintf(stderr, "Terminal type? ");
		(void)fflush(stderr);

		if (fgets(answer, sizeof(answer), stdin) == NULL) {
			if (dflt == NULL) {
				(void)fprintf(stderr, "\n");
				exit(1);
			}
			return (dflt);
		}

		if (p = index(answer, '\n'))
			*p = '\0';
		if (answer[0])
			return (answer);
		if (dflt != NULL)
			return (dflt);
	}
}
