#
/*
**  IIGLOBALS.H -- Equel run-time library globals
**
**	In this file are defined the global variables,
**	and manifest constants used in the Equel run-time
**	routines. All globals must start with "II".
**
**	Requires:
**		The "pipfrmt" structure is the ingres interprocess
**		pipe buffer structure defined in ".../pipes.h".
**		MAXDOM is defined in .../ingres.h.
**
**	Required By:
**		.../source/equel/II*.c
**
**	History:
**		Worked upon at various times by :
**		jim ford
**		eric allman
**		mike ubell - v5 to v6
**		4/78 -	rewritten [marc]
*/

# define	opINT		1
# define	opFLOAT		2
# define	opSTRING	3
# define	opDOUBLE	4
# define	opCHAR		5
# define	opLONG		6

# define	EQUEL		'&'
# define	ERRDELIM	'~'

char		*IIproc_name;		/* file name */
int		IIline_no;		/* line no */
int		IIdebug;		/* debug flag */
int		IIingpid;		/* process id of ingres */
char		*IIoptn [9];		/* ingres options */
int		IIin_retrieve;		/* set if inside a retrieve */
int		IIndomains;		/* number of doamins in this retrieve */
int		IIdomains;
int		IInxtdomain;		/* index into source field of buffer */
long		IItupcnt;		/* # tuples which satified last 
					 * update query 
					 */
int		IIw_down, IIr_down;	/* pipe descriptors for 
					 * parser communcation 
					 */
int		IIr_front;		/* pipe descriptor for 
					 * ovqp ommunication 
					 */
int		IIerrflag;		/* error flag. 
					 * Set in IIerror, cleared in IIsync 
					 */
extern char	IIPathname[];		/* initialized by 
					 * IIgetpath() [IIingres.c] 
					 */
int		(*IIprint_err)();	/* wether or not error messgs should
					 * be printed
					 */
int		IIret_err();		/* returns its integer arg for 
					 * (*IIprint_err)()
					 */
int		IIno_err();		/* returns 0 for (*IIprint_err)() */
int		(*IIo_print)();		/* a one value stack for 
					 * temporarily turned off printing of
					 * errors. Done in [IIw_left.c & 
					 * IIw_right.c].
					 */
char		*IImainpr;		/* "/usr/bin/ingres.c" usually, 
					 * [ingres.c]
					 */

/* buffer structure of get(III) */
struct iob
{
	int		fildes;
	int		nleft;
	char		*nextp;
	char		buff[512];
};

struct retsym
{
	char		type;
	char		len;
	char		*addr;
};
struct retsym	IIretsym[MAXDOM];	/* retrieve table */
struct retsym	IIr_sym;		/* type, length fields used
					 * by new equel in IIn_ret and IIn_get
					 */

struct pipfrmt	IIeinpipe;	/* the data pipe structure (IIR_front) */
struct pipfrmt	IIinpipe;	/* the control pipe struct (IIR_down)  */
struct pipfrmt	IIoutpipe;	/* the pipe for writing qrys (IIW_down)*/
