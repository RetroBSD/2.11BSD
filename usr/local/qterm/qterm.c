#ifndef lint
static char *RCSid = "$Header: /src/common/usc/bin/qterm/RCS/qterm.c,v 5.4 1991/03/21 02:09:40 mcooper Exp $";
#endif

/*
 * Copyright (c) 1990 Michael A. Cooper.
 * This software may be freely distributed provided it is not sold for 
 * profit and the author is credited appropriately.
 */

/*
 *------------------------------------------------------------------
 *
 * $Source: /src/common/usc/bin/qterm/RCS/qterm.c,v $
 * $Revision: 5.4 $
 * $Date: 1991/03/21 02:09:40 $
 * $State: Exp $
 * $Author: mcooper $
 * $Locker:  $
 *
 *------------------------------------------------------------------
 *
 * Michael A. Cooper
 * Research and Development Group
 * University Computing Services 
 * University of Southern California
 * (mcooper@usc.edu)
 *
 *------------------------------------------------------------------
 *
 * $Log: qterm.c,v $
 * Revision 5.4  1991/03/21  02:09:40  mcooper
 * Fix memory buffer problem with some C
 * compilers.  (tp@vtold.vtol.fi)
 *
 * Revision 5.3  1991/03/16  05:36:30  mcooper
 * Fix casting of (char) NULL problem.
 *
 * Revision 5.2  1991/03/12  00:46:24  mcooper
 * Change CMASK to CHAR_CMASK to avoid conflict
 * under AIX 3.1.
 *
 * Revision 5.1  1991/02/20  02:23:33  mcooper
 * Cleanup #ifdef USG5 as part of port
 * to UTS 2.1 (System V.3).
 *
 * Revision 5.0  1990/12/15  18:30:41  mcooper
 * Version 5.
 *
 * Revision 4.13  90/12/15  18:14:23  mcooper
 * Add copywrite.
 * 
 * Revision 4.12  90/11/13  16:00:03  mcooper
 * Convert OptInt's to OptBool's where needed.
 * 
 * Revision 4.11  90/11/13  15:38:28  mcooper
 * Make OLD_SepArg include both
 * SepArg and StickyArg.
 * 
 * Revision 4.10  90/11/08  15:41:08  mcooper
 * Make sure qt_fullname is not 0 length.
 * 
 * Revision 4.9  90/11/08  13:02:06  mcooper
 * Fix bug that closes the tty when an error
 * occurs during command line parsing.
 * 
 * Revision 4.8  90/11/06  13:19:40  mcooper
 * Changed command line options to new 
 * longer names.
 * 
 * Revision 4.7  90/11/05  17:09:30  mcooper
 * Update option help messages and option names
 * to be more mnemonic.
 * 
 * Revision 4.6  90/11/05  16:44:35  mcooper
 * - Converted to use new ParseOptions() for
 *   command line parsing.
 * - Major de-linting.
 * - Convert dprintf() to use varargs (if
 *   HAS_VARARGS is defined).
 * - Lots of misc. cleanup.
 * 
 * Revision 4.5  89/10/20  22:50:49  mcooper
 * Changed code indention to current local
 * standard of 4.  (This should also mess up
 * everybody trying to do diff's from older versions!)
 * 
 * Revision 4.4  89/10/20  14:03:48  mcooper
 * Fixed command line parsing of "-f -q".
 * 
 * Revision 4.3  88/06/16  19:43:46  mcooper
 * - Added -T flag to wait until timeout when
 *   listening for response string.  This solves
 *   problem when the first entry in a table
 *   doesn't have a response string with a
 *   common ending character to look for.
 * - Added -I flag for "intense" query mode.
 * - Cleaned up debugging a bit.
 * 
 * Revision 4.2  88/06/08  15:30:53  mcooper
 * Cleanup pass including removing
 * extraneous debugging messages.
 * 
 * Revision 4.1  88/04/25  13:24:38  mcooper
 * Added -S option to print send and recieve
 * strings as they are sent and recieved as
 * suggested by David W. Sanderson
 * (dws@attunix.att.com).
 * 
 * Revision 4.0  88/03/08  19:30:59  mcooper
 * Version 4.
 * 
 * Revision 3.7  88/03/08  19:28:32  mcooper
 * Major rewrite.
 * 
 * Revision 3.6  88/03/08  15:31:35  mcooper
 * General cleanup time.
 * 
 * Revision 3.5  88/03/08  13:59:39  mcooper
 * - Catch signals and fix terminal modes.
 * - Don't allow alarm times of 0.
 * - Support for HP-UX machines and cleaner
 *   listen() code from Zenon Fortuna, 
 *   HP-UX Support, Hewlett-Packard Vienna.
 * 
 * Revision 3.4  87/10/07  15:16:17  mcooper
 * - Beautify code a bit.
 * - Add -w <N> option to set the wait time.
 * 
 * Revision 3.3  87/08/24  19:25:32  mcooper
 * The following based on code from Frank Crawford 
 * <frank@teti.qhtours.OZ>:
 * - Use $TERM as output string when the terminal
 *   type is not known instead of "dumb".
 * - Regular Expressions are now supported.  RE are
 *   started with a leading `\'.
 * - Octal values may now be used in send/recieve strings.
 * 
 * Revision 3.1  87/08/03  15:21:07  mcooper
 * As pointed out by Scott H. Robinson <shr@cetus.ece.cmu.edu>,
 * the -F switch does work.  Problem was that it never read
 * in the ~/.qterm file.
 * 
 * Revision 3.0  87/06/30  19:07:59  mcooper
 * Release of version 3.
 * 
 * Revision 2.4  87/04/29  19:28:35  mcooper
 * In readtabfile() we now do special
 * things when opening "file" fails
 * depending on the bequiet flag.
 * 
 * Revision 2.3  87/04/29  13:11:37  mcooper
 * - No more "internal" table.  The master
 *   table is read from a file (TABFILE).
 *   This makes ~/.qterm stuff much cleaner.
 * - Error handling for qtermtab files is
 *   much more informative now.
 * - More things I can't remember.
 * 
 * Revision 2.2  87/03/05  21:01:28  mcooper
 * Fixed system V compiler problem.
 * 
 * Revision 2.1  87/03/01  19:43:22  mcooper
 * Be more intelligent about the size of 
 * the default terminal table.
 * 
 * Revision 2.0  87/03/01  19:20:00  mcooper
 * General cleanup.
 * 
 *------------------------------------------------------------------
 */


/*
 * qterm - Query Terminal
 *
 * qterm is used to query a terminal to determine the name of the terminal.
 * This is done by sending a fairly universal string "\33Z" to the terminal,
 * reading in a response, and comparing it against a master table of responses
 * and names.  The "name" printed to standard output should be one found in
 * the termcap(5) database.
 *
 * Putting a line in your ".login" file such as:
 *
 *	setenv TERM `qterm`
 *
 * or the following lines in your ".profile" file:
 *
 *	TERM=`qterm`
 *	export TERM
 *
 * will set your terminal type automagically.
 * 
 * If you add a terminal to the master table, please also send me a copy
 * so that I may put it into my version.
 *
 * Michael Cooper
 * Internet: 	mcooper@usc.edu
 * UUCP: 	...!rutgers!usc!mcooper
 * BITNET:	mcooper@gamera
 */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#ifdef USG5
# include <termio.h>
#else /*USG5*/
# include <sys/file.h>
# include <sgtty.h>
#endif /*USG5*/
#include "qterm.h"
#include "options.h"
#ifdef HAS_VARARGS
#include <varargs.h>
#endif /*HAS_VARARGS*/

#ifdef USG5
struct termio _ntty, _otty;
#else
struct sgttyb _tty;
#endif
int _tty_ch = 2;
char recvbuf[SIZE];
char *progname;
char *termfile = NULL;

int debug = FALSE;		/* Debug mode */
int use_alt_str = FALSE;	/* Alternate string */
int towait = FALSE;		/* Time out wait flag */
int always_send = FALSE;	/* Intense query mode */
int longname = FALSE;		/* Print long terminal name */
int sent_chars = FALSE;		/* Print strings sent from the terminal */
int watch_chars = FALSE;	/* Watch strings as they are sent and recv. */
int quiet = FALSE;		/* Quiet mode */
int do_usrtabfile = FALSE;	/* Use user's own .qtermtab file */
int do_systabfile = TRUE;	/* Use the system's qterm tab file */
int almwait = WAIT;		/* Wait (timeout) interval */

/*
 * Old options should not be visable in help and usage messages.
 */
#ifdef OPT_COMPAT
#define OLD_NoArg	NoArg|ArgHidden
#define OLD_SepArg	SepArg|StickyArg|ArgHidden
#define fFLAG 		"-f"
#define FFLAG		"-F"
#endif

/*
 * Command line options table.
 */
OptionDescRec opts[] = {
#ifdef OPT_COMPAT
    {"-a", 	OLD_NoArg,	OptInt,	(caddr_t) &use_alt_str,		"1",
	 (char *)NULL,	"Use alternate query string"},
    {"-s",	OLD_NoArg,	OptInt,	(caddr_t) &sent_chars,		"1",
	 (char *)NULL,	"Display the characters the terminal sent"},
    {"-t", ArgHidden|OLD_NoArg,	OptInt,	(caddr_t) &sent_chars,		"1",
	 (char *)NULL,	"Display the characters the terminal sent"},
    {"-I",	OLD_NoArg,	OptInt,	(caddr_t) &always_send,		"1",
	 (char *)NULL,	"Always send the terminal query string"},
    {"-T",	OLD_NoArg,	OptInt,	(caddr_t) &towait,		"1",
	 (char *)NULL,	"Enable time out wait"},
    {"-S",	OLD_NoArg,	OptInt,	(caddr_t) &watch_chars,		"1",
	 (char *)NULL,	"Print strings as they are sent and received"},
    {"-q",	OLD_NoArg,	OptInt,	(caddr_t) &quiet,		"1",
	 (char *)NULL,	"Enable quite mode"},
    {"-f",	OLD_SepArg,	OptStr,	(caddr_t) &termfile,  fFLAG,
	 "<tabfile>",  "Try <tabfile>, then ~/.qtermtab, then system tabfile"},
    {"-F",	OLD_SepArg,	OptStr,	(caddr_t) &termfile,  FFLAG,
	 "<tabfile>",	"Try <tabfile>, then ~/.qtermtab"},
    {"-l",	OLD_NoArg,	OptInt,	(caddr_t) &longname,		"1",
	 (char *)NULL,	"Output only the long (verbose) terminal name"},
    {"-d", 	OLD_NoArg,	OptInt,	(caddr_t) &debug,		"1",
	 (char *)NULL,	"Enable debug mode"},
    {"-w",	OLD_SepArg,	OptInt,	(caddr_t) &almwait,	    __ NULL,
	 "<interval>",	"Wait (timeout) period (in seconds)"},
#endif /*OPT_COMPAT*/
    {"+alt", 	NoArg,		OptBool, (caddr_t) &use_alt_str,	"1",
	 (char *)NULL,	"Use alternate query string"},
    {"-alt", 	NoArg,		OptBool, (caddr_t) &use_alt_str,	"0",
	 (char *)NULL,	"Don't use alternate query string"},
    {"+always",	NoArg,		OptBool, (caddr_t) &always_send,	"1",
	 (char *)NULL,	"Always send the terminal query string"},
    {"-always",	NoArg,		OptBool, (caddr_t) &always_send,	"0",
	 (char *)NULL,	"Don't always send the terminal query string"},
    {"-file",	SepArg,		OptStr,	(caddr_t) &termfile,  	    __ NULL,
	 "<tabfile>",   "Use <tabfile> to query terminal"},
    {"+longname",NoArg,		OptBool, (caddr_t) &longname,		"1",
	 (char *)NULL,	"Output only the long (verbose) terminal name"},
    {"-longname",NoArg,		OptBool, (caddr_t) &longname,		"0",
	 (char *)NULL,	"Don't output the long (verbose) terminal name"},
    {"+quiet",	NoArg,		OptBool, (caddr_t) &quiet,		"1",
	 (char *)NULL,	"Enable quiet mode"},
    {"-quiet",	NoArg,		OptBool, (caddr_t) &quiet,		"0",
	 (char *)NULL,	"Disable quiet mode"},
    {"+sent",	NoArg,		OptBool, (caddr_t) &sent_chars,		"1",
	 (char *)NULL,	"Display the characters the terminal sent"},
    {"-sent",	NoArg,		OptBool, (caddr_t) &sent_chars,		"0",
	 (char *)NULL,	"Don't display the characters the terminal sent"},
    {"+timeout",NoArg,		OptBool, (caddr_t) &towait,		"1",
	 (char *)NULL,	"Enable time out wait"},
    {"-timeout",NoArg,		OptBool, (caddr_t) &towait,		"0",
	 (char *)NULL,	"Disable time out wait"},
    {"+usrtab",	NoArg,		OptBool, (caddr_t) &do_usrtabfile,	"1",
	 (char *)NULL,	"Enable using ~/.qtermtab"},
    {"-usrtab",	NoArg,		OptBool, (caddr_t) &do_usrtabfile,	"0",
	 (char *)NULL,	"Disable using ~/.qtermtab"},
    {"-wait",	SepArg,		OptInt,	(caddr_t) &almwait,	    __ NULL,
	 "<interval>",	"Wait (timeout) period (in seconds)"},
    {"+watch",	NoArg,		OptBool, (caddr_t) &watch_chars,	"1",
	 (char *)NULL,	"Watch the characters sent and recieved"},
    {"-watch",	NoArg,		OptBool, (caddr_t) &watch_chars,	"0",
	 (char *)NULL,	"Don't watch the characters sent and recieved"},
    {"+systab",	NoArg,		OptBool, (caddr_t) &do_usrtabfile,	"1",
	 (char *)NULL,	"Enable using system qtermtab file"},
    {"-systab",	NoArg,		OptBool, (caddr_t) &do_systabfile,	"0",
	 (char *)NULL,	"Disable using system qtermtab file"},
    {"-debug", ArgHidden|NoArg,	OptInt,	(caddr_t) &debug,		"1",
	 (char *)NULL,	"Enable debug mode"},
};

FILE *fopen();
char *decode();
char *getenv();
char *malloc();
char *re_comp();
char *strcat();
char *xmalloc();
int alarm();
int found = FALSE;
int modes_set = FALSE;
jmp_buf env;
struct termtable *compare();
struct passwd *getpwuid();
void catch();
void done();
void dprintf();
void exit();
void myperror();
void mktable();
void notrecognized();
void proctab();
void wakeup();
#ifdef USG5
char *regcmp();
#endif /* USG5 */

main(argc, argv)
     int argc;
     char **argv;
{
    config(argc, argv);
    setmodes();
    mktable();
    proctab((struct termtable *)NULL);
    resetmodes();
    
    if (!found) {
	notrecognized();
    }

    exit(0);
}

/*
 * Config() - Perform configuration operations.
 */
config(argc, argv)
     int argc;
     char **argv;
{
    progname = argv[0];

    /*
     * Parse command line args
     */
    if (ParseOptions(opts, Num_Opts(opts), argc, argv) < 0) {
	done(1);
	/*NOTREACHED*/
    }

    /*
     * Check results of command line parsing and perform any
     * needed post processing.
     */

    if (longname)
	quiet = TRUE;

    if (almwait == 0) {
	(void) fprintf(stderr, 
		      "%s: Alarm (wait) time must be greater than 0.\n",
		       progname);
	done(1);
	/*NOTREACHED*/
    }

#ifdef OPT_COMPAT
    /*
     * Kludgy stuff to be backwards compatable for command line options.
     */
    if (termfile) {
	if (strcmp(termfile, fFLAG) == 0) {
	    do_usrtabfile = TRUE;
	    do_systabfile = TRUE;
	    termfile = NULL;
	} else if (strcmp(termfile, FFLAG) == 0) {
	    do_usrtabfile = TRUE;
	    do_systabfile = FALSE;
	    termfile = NULL;
	}
    }
#endif /*OPT_COMPAT*/

    dprintf("[ %s debug mode enabled ]\n\n", progname);
}

/*
 * Set signal catches and terminal modes
 */
setmodes()
{
    if (!isatty(0)) {
	(void) fprintf(stderr, "%s: Not a tty.\n", progname);
	done(0);
	/*NOTREACHED*/
    }
    
    /*
     * Set output buffers
     */
    setbuf(stdout, (char *)0);
    if (debug)
	setbuf(stderr, (char *)0);
    
    /*
     * Cleanup terminal modes & such if we are killed
     */
    (void) signal(SIGINT, catch);
    (void) signal(SIGHUP, catch);
    (void) signal(SIGTERM, catch);
    
    /*
     * Set terminal modes
     */
#ifdef USG5
    if (ioctl(_tty_ch, TCGETA, &_otty) < 0)
#else
    if (ioctl(_tty_ch, TIOCGETP, &_tty) < 0)
#endif /* USG5 */
    {
	myperror("gtty");
	done(1);
	/*NOTREACHED*/
    }
#ifdef USG5
    _ntty = _otty;
#endif /* USG5 */

    if (crmode() < 0) {
	myperror("crmode");
	done(1);
	/*NOTREACHED*/
    }
    
    if (noecho() < 0) {
	myperror("noecho");
	done(1);
	/*NOTREACHED*/
    }
    modes_set = TRUE;
}

/*
 * Reset terminal modes
 */
resetmodes()
{
    if (modes_set) {
	(void) nocrmode();
	(void) echo();
    }
}

/*
 * Print info about terminal structure t.
 */
prinfo(t, what)
     struct termtable *t;
     int what;
{
    int len = 0;
    int st = FALSE;
    
    if (t && t->qt_termname && (recvbuf[0] != (char) NULL)) {
	if (debug || sent_chars) {
	    len = strlen(recvbuf);
	    (void) fprintf(stderr, "%s received %d character%s:", 
			   progname, len, (len == 1) ? "" : "s");
	    (void) fprintf(stderr, " %s\n", decode(recvbuf));
	}
	
	if (!quiet) {
	    (void) fprintf(stderr, "Terminal recognized as %s", 
			   t->qt_termname);
	    if (t->qt_fullname && t->qt_fullname[0])
		(void) fprintf(stderr, " (%s)\n", t->qt_fullname);
	    else
		(void) fprintf(stderr, "\n");
	}
	
	if (longname) {
	    if (t->qt_fullname && t->qt_fullname[0])
		(void) printf("%s\n", t->qt_fullname);
	    else
		(void) fprintf(stderr, "%s: No full terminal name for %s.\n",
			       progname, t->qt_termname);
	} else {
	    (void) printf("%s\n", t->qt_termname);
	}
	
	found = TRUE;
	done(0);
	/*NOTREACHED*/
    } else {
	found = FALSE;
	
	if (what) {
	    notrecognized();
	    done(1);
	    /*NOTREACHED*/
	}
    }
    
    return(st);
}

/*
 * compare - actually compare what we received against the table.
 */
struct termtable *compare(str)
     char *str;
{
#ifdef USG5
    register char *reexp;
#endif /* USG5 */
    register struct termtable *t;
    char buf[BUFSIZ];

    dprintf("compare %s\n", (str && str[0]) ? decode(str) : "nothing");
    (void) alarm((unsigned)0);
    
    if (strlen(str) == 0)
	return(NULL);
    
    for (t = termtab; t != NULL; t = t->nxt) {
	dprintf("  with %s ", decode(t->qt_recvstr));
	(void) sprintf(buf, "^%s$", t->qt_recvstr);
	
#ifdef USG5
	if ((reexp = regcmp(buf, NULL)) == NULL) {
#else
	if (re_comp((char *)buf) != NULL) {
#endif /* USG5 */
	    (void) fprintf(stderr, "%s: bad regular expression: \"%s\"\n", 
			   progname, t->qt_recvstr);
	    done(1);
	    /*NOTREACHED*/
	}

#ifdef USG5
	if (regex(reexp, str) != NULL) {
#else
	if (re_exec(str) == 1) {
#endif /* USG5 */
	    found = TRUE;
	    dprintf("\tOK\n");
	    return(t);
	}

	dprintf("\tNOPE\n");
#ifdef USG5
	(void) free(reexp);
#endif /* USG5 */
    }
    found = FALSE;

    return(NULL);
}

/*
 * getch - read in a character at a time.
 */
getch()
{
    char c;
    
    (void) read(0, &c, 1);
    
    return(c & CHAR_MASK);
}

/*
 * decode - print str in a readable fashion
 */
char *decode(str)
     char *str;
{
    register int len;
    static char buf[BUFSIZ];
    char tmp[10];
    
    if (!str)
      return("(null)");
    
    (void) strcpy(buf, "");
    while (*str) {
	if (*str == ESC) {
	    (void) strcat(buf, "<esc> ");
	} else if ((*str <= 33) || (*str >= 127)) {
	    (void) sprintf(tmp,"\\%#o ", (unsigned) *str);
	    (void) strcat(buf, tmp);
	} else {
	    (void) sprintf(tmp,"%c ", *str);
	    (void) strcat(buf, tmp);
	}
	++str;
    }
    
    len = strlen(buf);
    if (len && buf[len - 1] == ' ') {
	buf[len - 1] = (char) NULL;
    }
    
    return(buf);
}

/*
 * Make a termtab table
 */
void mktable()
{
    char file[BUFSIZ];
    struct passwd *pwd;
    char *home;

    dprintf("[ initilizing term table... ]\n");
    
    if (termfile != NULL) {
	(void) readtabfile(termfile, FALSE);
    }

    if (do_usrtabfile) {
	/*
	 * Try to read the user's own table
	 */
	if ((home = getenv("HOME")) == NULL) {
	    if ((pwd = getpwuid(getuid())) == NULL) {
		(void) fprintf(stderr, 
			       "%s: Cannot find user info for uid %d.\n",
			       progname, getuid());
		done(1);
		/*NOTREACHED*/
	    }
	    home = pwd->pw_dir;
	}

	(void) sprintf(file, "%s/%s", home, USRFILE);
	if (readtabfile(file, TRUE) < 0) {
	    (void) sprintf(file, "%s/%s", home, OLDUSRFILE);
	    (void) readtabfile(file, TRUE);
	}
    }
    
    if (do_systabfile)
	(void) readtabfile(TABFILE, FALSE);
    
    dprintf("[ mktable done ]\n");
}

int readtabfile(file, bequiet)
     char *file;
     int bequiet;
{
    static int line = 0;
    char lbuf[4][BUFSIZ];
    char buf[BUFSIZ];
    FILE *fd;
    char *p, *fixctl();
    char *errmsg = NULL;
    struct termtable *t;
    
    if ((fd = fopen(file, "r")) == NULL) {
	if (bequiet) {
	    dprintf("[ tab file '%s' can not read ]\n", file);
	    return(-1);
	}
	myperror(file);
	done(1);
	/*NOTREACHED*/
    }

    dprintf("[ Read tab file '%s' ]\n", file);

    line = 0;
    while (fgets(buf, sizeof(buf), fd)) {
	++line;
	
	if (buf[0] == '#' || buf[0] == '\n')
	    continue;
	
	lbuf[0][0] = lbuf[1][0] = lbuf[2][0] = lbuf[3][0] = (char) NULL;
	
	(void) sscanf(buf, "%s%s%s\t%[^\n]", 
		      lbuf[0], lbuf[1], lbuf[2], lbuf[3]);
	
	if (lbuf[0][0] == (char) NULL)
	    continue;
	
	if (lbuf[1][0] == (char) NULL)
	    errmsg = "receive string";
	
	if (lbuf[2][0] == (char) NULL)
	    errmsg = "terminal name";
	
	if (errmsg) {
	    (void) fprintf(stderr, "%s: Line %d of %s: Error parsing %s.\n", 
			   progname, line, file, errmsg);
	    done(1);
	    /*NOTREACHED*/
	}
	
	t = (struct termtable *) xmalloc(sizeof(struct termtable));
	
	if (use_alt_str)
	    p = fixctl(ALTSEND, 0);
	else
	    p = fixctl(lbuf[0], 0);
	
	t->qt_sendstr = (char *) xmalloc(strlen(p)+1);
	(void) strcpy(t->qt_sendstr, p);
	
	p = fixctl(lbuf[1], 1);
	t->qt_recvstr = (char *) xmalloc(strlen(p)+1);
	(void) strcpy(t->qt_recvstr, p);
	
	t->qt_termname = (char *) xmalloc(strlen(lbuf[2])+1);
	(void) strcpy(t->qt_termname, lbuf[2]);
	
	t->qt_fullname = (char *) xmalloc(strlen(lbuf[3])+1);
	(void) strcpy(t->qt_fullname, lbuf[3]);
	
	dprintf("\n  Send String = %s\n", decode(t->qt_sendstr));
	dprintf("Expect String = %s\n", decode(t->qt_recvstr));
	dprintf("     Terminal = '%s'\n", t->qt_termname);
	dprintf("    Full Name = '%s'\n", t->qt_fullname);
	
	(void) addterm(t);
    }
    
    return(0);
}

/*
 * Add termtab (n) entry to main termtab.
 */
int addterm(n)
     struct termtable *n;
{
    register struct termtable *t;
    
    if (!n)
      return(-1);
    
    n->nxt = NULL;
    
    if (termtab == NULL) {
	termtab = n;
    } else {
	t = termtab;
	while(t && t->nxt)
	  t = t->nxt;
	t->nxt = n;
    }
    
    return(0);
}

/*
 * Listen for a response.
 */
void qterm_listen(q)
     struct termtable *q;
{
    static int i, len;
    register char c;
    char end;
    
    (void) alarm((unsigned)0);
    (void) strcpy(recvbuf, "");
    i = 0;
    
    len = strlen(q->qt_recvstr);
    
    if (len) {
	end = q->qt_recvstr[len - 1];
    } else {
	end = 'c'; /* Fairly standard ANSI default */
    }
    
    dprintf("\nlisten for %s\t [ len = %d, end = `%c' ]\n", 
	    decode(q->qt_recvstr), len, end);
    
    /*
     * If we don't get an initial character, bounce out
     * of here and finish with done(0).
     */
    if (setjmp(env)) {
	if (found) {
	    done(0);
	    /*NOTREACHED*/
	}
	(void) fflush(stdin);
	proctab(q->nxt);
    } else {
	(void) signal(SIGALRM, wakeup);
	(void) alarm((unsigned)almwait);
	recvbuf[0] = getch();
	(void) alarm((unsigned)0);
    }
    
    /*
     * Read in remaining response.  Loop until ending character
     * is received or until alarm goes off.  If towait is set,
     * then let alarm go off.
     */
    for (i = 1, c = -1; (!towait && (c != end)) || towait; ) {
	if (setjmp(env))  {
	    recvbuf[i] = (char) NULL;
	    return;
	} else {
	    (void) signal(SIGALRM, wakeup);
	    (void) alarm((unsigned)almwait);
	    c = getch();
	    (void) alarm((unsigned)0);
	}
	recvbuf[i++] = c;
    }
    recvbuf[i] = (char) NULL;
    
    dprintf("listen done.  read %d chars.\n\n", i);
}

/*
 * Print a message since we didn't recognize this terminal.
 */
void notrecognized()
{
    char *envterm;
    
    if ((envterm = getenv("TERM")) == NULL)
	envterm = "dumb";
    
    if (!quiet)
	(void) fprintf(stderr, 
		       "Terminal NOT recognized - defaults to \"%s\".\n",
		       envterm);
    
    puts(envterm);
}

/*
 * Process entries in the termtable.
 */
void proctab(t)
     struct termtable *t;
{
    int st = FALSE;
    static int firsttime = TRUE;
    static struct termtable *lastt;
    
    dprintf("\n[ Processing entries ] \n");
    
    if (firsttime) {
	t = termtab;
	lastt = NULL;
    }
    
    while ((!found || do_usrtabfile) && t && t->qt_sendstr && !st) {
	/*
	 * If this is our first time or the sendstr is the same as
	 * last time, don't send it again.
	 */
	if (always_send || firsttime || lastt == NULL || 
	    strcmp(t->qt_sendstr, lastt->qt_sendstr) != 0) {
	    
	    if (firsttime)
		firsttime = FALSE;
	    
	    if (watch_chars)
		(void) printf("Send: %s\n", decode(t->qt_sendstr));

	    (void) fflush(stdin);
	    (void) fprintf(stderr, "%s", t->qt_sendstr);
	    (void) fflush(stderr);
	    
	    lastt = t;
	    (void) qterm_listen(t);
	    
	    if (watch_chars)
		(void) printf("\tRead: %s\n", decode(recvbuf));
	}
	
	st = prinfo(compare(recvbuf), FALSE);
	
	lastt = t;
	t = t->nxt;
    }
    
    if (!found)
	notrecognized();
    
    done(0);
    /*NOTREACHED*/
}

char *fixctl(str, rex)
     char *str;
     int rex;
{
    register int i;
    static char buf[BUFSIZ];
    
    for (i = 0; str && *str; ) {
	switch (*str) {
	    
	  case '\\':
	    if (isdigit(*++str)) {
		buf[i] = 0;
		while (isdigit(*str))
		    buf[i] = (char) (((int)buf[i] * 8) + 
				     (int)*str++ - (int) '0');
		i++;
	    } else
		buf[i++] = *str++;
	    continue;
	    
	  case '^':
	    switch (*++str) {
	      case '?':
		buf[i++] = '\177';
		break;
	      default:
		buf[i++] = *str & 037;
		break;
	    }
	    break;
	    
	    /* Special R.E. symbols */
	  case '[':
	  case '*':
	  case '.':
	  case '$':
	  case '{':
	  case '(':
	    if (rex)
	      buf[i++] = '\\';
	    
	  default:
	    buf[i++] = *str;
	}
	*++str;
    }
    
    buf[i] = (char) NULL;
    
    return(buf);
}

/*
 * xmalloc - Do a malloc with error checking.
 */
char *xmalloc(size)
     int size;
{
    char *p;
    
    if ((p = malloc((unsigned) size)) == NULL) {
	myperror("malloc");
	done(1);
	/*NOTREACHED*/
    }
    
    return(p);
}

#ifdef HAS_VARARGS
void dprintf(va_alist)
     va_dcl
{
    va_list args;
    char *fmt;

    if (!debug)
	return;

    va_start(args);
    fmt = (char *) va_arg(args, char *);
    (void) vprintf(fmt, args);
    va_end(args());
    (void) fflush(stdout);
}

#else /*HAS_VARARGS*/

void dprintf(fmt, a1, a2, a3, a4, a5, a6)
     char *fmt;
{
    if (!debug)
	return;

    (void) printf(fmt, a1, a2, a3, a4, a5, a6);
    (void) fflush(stdout);
}
#endif /*HAS_VARARGS*/

/*
 * Catch kill signals and cleanup.
 */
void catch()
{
    done(2);
    /*NOTREACHED*/
}

void wakeup()
{
    dprintf("wakeup called\n");
    longjmp(env, 1);
}

void myperror(msg)
     char *msg;
{
    (void) fprintf(stderr, "%s: ", progname);
    perror(msg);
}

/*
 * Reset terminal and exit with status s.
 */
void done(s)
     int s;
{
    resetmodes();
    exit(s);
}
