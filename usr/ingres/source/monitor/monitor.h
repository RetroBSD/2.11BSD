#

/*
**  MONITOR.H -- globals for the interactive terminal monitor
**
**	History:
**		6/8/79 (eric) -- changed Version to Versn, since
**			Version now has the full name (with mod #).
**		3/2/79 (eric) -- Error_id and {querytrap} stuff
**			added.  Also, the RC_* stuff.
*/

/* various global names and strings */
char		Qbname[30];	/* pathname of query buffer */
char		Versn[12];	/* version number (w/o mod #) */
extern char	Version[];	/* version number (with mod #) */
extern char	Rel_date[];	/* release date */
extern char	Mod_date[];	/* release date of this mod */
extern char	*Fileset;	/* unique string */
extern char	*Pathname;	/* pathname of INGRES root */

/* flags */
char		Nodayfile;	/* suppress dayfile/prompts */
				/* 0 - print dayfile and prompts
				** 1 - suppress dayfile but not prompts
				** -1 - supress dayfile and prompts
				*/
char		Userdflag;	/* same: user flag */
				/*  the Nodayfile flag gets reset by include();
				**  this is the flag that the user actually
				**  specified (and what s/he gets when in
				**  interactive mode.			*/
char		Autoclear;	/* clear query buffer automatically if set */
char		Notnull;	/* set if the query is not null */
char		Prompt;		/* set if a prompt is needed */
char		Nautoclear;	/* if set, disables the autoclear option */
char		Phase;		/* set if in processing phase */

/* query buffer stuff */
FILE		*Qryiop;	/* the query buffer */
char		Newline;	/* set if last character was a newline */

/* other stuff */
int		Xwaitpid;	/* pid to wait on - zero means none */
int		Error_id;	/* the error number of the last err */

/* \include support stuff */
FILE		*Input;		/* current input file */
int		Idepth;		/* include depth */
char		Oneline;	/* deliver EOF after one line input */
char		Peekch;		/* lookahead character for getch */

/* commands to monitor */
# define	C_APPEND	1
# define	C_BRANCH	2
# define	C_CHDIR		3
# define	C_EDIT		4
# define	C_GO		5
# define	C_INCLUDE	6
# define	C_MARK		7
# define	C_LIST		8
# define	C_PRINT		9
# define	C_QUIT		10
# define	C_RESET		11
# define	C_TIME		12
# define	C_EVAL		13
# define	C_WRITE		14
# define	C_SHELL		15

/* stuff for querytrap facility */
extern FILE	*Trapfile;

/* internal retcode codes */
# define	RC_OK		0
# define	RC_BAD		1
