/*
 *	UNIX  2.9BSD  CRASH  ANALYZER  INCLUDE
 */

#define	END	0,0,0		/* Last structure entry */

/* Format codes */

# define OCT		1
# define TADDR		2	/* text address symbolic printing */
# define DEC		3	/* Decimal, unsigned */
# define DEV		4
# define ONEBYTE	5
# define CHARS		6
# define HALFDEC	7
# define DADDR		8	/* data address symbolic printing */
# define LONGDEC	9	/* long decimal printout */
# define LONGOCT	10	/* long octal printout */
# define UDEC		11	/* unsigned decimal */
# define HEXL		12
# define HEXW		13
# define HEX		13
# define HEXB		14
# define IGNORE		15	/* ignore this entry */

# define NEWPAGE	014		/* FF */
# define LINESPERPAGE	60		/* page size */

# define NSYM		0		/* undefined */
# define ISYM		2		/* if you want a text symbol */
# define DSYM		7		/* data, or bss symbol */

#define	NOTFOUND	0177777	/* value indicating symbol not found */

struct fetch {
	char	*symbol;		/* Symboltable entry name */
	char	*addr;			/* Addr to load to */
	int	f_size;			/* # of bytes to load */
};

struct display {
	char	*msg;			/* Message to preceed element */
	char	*place;			/* Base addr of data (offset added here) */
	int	fmt;			/* display format */
	int	(*routine)();		/* extra routine to call per field */
};


/*
 * Interrupt Tracing Strutures - The SYS Group
 */
struct	itrace	{
	unsigned	intps;		/* interrupted PS */
	unsigned	intpc;		/* interrupted PC */
	unsigned	r0;		/* r0 from trap vector */
	unsigned	savps;		/* saved PS (software traps) */
};



struct syment {
	char name[8];
	char flags;
	char ovno;
	unsigned value;
};

struct symsml {
	char sflags;
	char sovno;
	unsigned svalue;
};

unsigned find();
unsigned findv();
