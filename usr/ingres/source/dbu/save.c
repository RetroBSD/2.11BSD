# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"

/*
**  SAVE RELATION UNTIL DATE
**
**	This function arranges to save a named relation until a
**	specified date.
**
**	Parameters:
**		0 -- relation name
**		1 -- month (1 -> 12 or "jan" -> "dec" or a variety
**			of other codes)
**		2 -- day (1 -> 31)
**		3 -- year (1970 -> ?)
**
**	Returns:
**		zero on success
**		nonzero otherwise.
**
**	Side Effects:
**		Change expiration date for some relation in
**		relation relation.
**
**	Trace Flags:
**		4
**
**	Diagnostics:
**		Uses error messages 53xx
**
**	History:
**		8/15/79 (eric) (6.2/7) -- defined dmsize locally
**			for version seven.
*/

# ifdef xV7_UNIX
int	Dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
# include	"sys/timeb.h"
# endif
# ifndef xV7_UNIX
# define	Dmsize		dmsize
extern int	Dmsize[12];	/* defined in ctime(3) */
# endif


save(parmc, parmv)
int	parmc;
char	**parmv;
{
	long				date;
	register int			i;
	extern struct descriptor	Reldes;
	struct tup_id			tid;
	extern char			*Usercode;
	struct relation			relk, relt;
	int				day, month, year;
#	ifndef xV7_UNIX
	extern int			timezone;	/* defined in ctime(3) */
#	endif
#	ifdef xV7_UNIX
	struct timeb			timeb;
#	endif
	extern int			dysize();	/* defined in ctime(3) */

	/* validate parameters */
	if (atoi(parmv[3], &year) || year < 1970)
		return (error(5603, parmv[3], 0));	/* bad year */
	if (monthcheck(parmv[1], &month))
		return (error(5601, parmv[1], 0));	/* bad month */
	if (atoi(parmv[2], &day) || day < 1 || day > monthsize(--month, year))
		return (error(5602, parmv[2], 0));	/* bad day */

	/* convert date */
	/* "date" will be # of days from 1970 for a while */
	date = 0;

	/* do year conversion */
	for (i = 1970; i < year; i++)
	{
		date += dysize(i);
	}

	/* do month conversion */
	for (i = 0; i < month; i++)
		date += Dmsize[i];
	/* once again, allow for leapyears */
	if (month >= 2 && year % 4 == 0 && year % 100 != 0)
		date += 1;

	/* do day conversion */
	date += day - 1;

	/* we now convert date to be the # of hours since 1970 */
	date *= 24;

	/* do daylight savings computations */
	/*  <<<<< none now >>>>> */

	/* convert to seconds */
	date *= 60 * 60;

	/* adjust to local time */
#	ifdef xV7_UNIX
	ftime(&timeb);
	date += ((long) timeb.timezone) * 60;
#	endif
#	ifndef xV7_UNIX
	date += timezone;
#	endif

#	ifdef xZTR1
	if (tTf(4, 1))
		printf("%s", ctime(&date));
#	endif

	/* let's check and see if the relation exists */
	opencatalog("relation", 2);
	clearkeys(&Reldes);
	setkey(&Reldes, &relk, parmv[0], RELID);
	setkey(&Reldes, &relk, Usercode, RELOWNER);
	if (getequal(&Reldes, &relk, &relt, &tid))
	{
		return (error(5604, parmv[0], 0));	/* relation not found */
	}

	/* check that it is not a system catalog */
	if (relt.relstat & S_CATALOG)
		return (error(5600, parmv[0], 0));	/* cannot save sys rel */
	/* got it; lets change the date */
	relt.relsave = date;

#	ifdef xZTR2
	if (tTf(4, 2))
	{
		printup(&Reldes, &relt);
	}
#	endif

	if ((i = replace(&Reldes, &tid, &relt, 0)) < 0)
		syserr("SAVE: replace %d", i);

	/* that's all folks.... */
	return (0);
}


struct monthtab
{
	char	*code;
	int	month;
};

struct monthtab	Monthtab[] =
{
	"jan",		1,
	"feb",		2,
	"mar",		3,
	"apr",		4,
	"may",		5,
	"jun",		6,
	"jul",		7,
	"aug",		8,
	"sep",		9,
	"oct",		10,
	"nov",		11,
	"dec",		12,
	"january",	1,
	"february",	2,
	"march",	3,
	"april",	4,
	"june",		6,
	"july",		7,
	"august",	8,
	"september",	9,
	"october",	10,
	"november",	11,
	"december",	12,
	0
};

monthcheck(input, output)
char	*input;
int	*output;
{
	register struct monthtab	*p;
	int				month;

	/* month can be an integer, or an alphanumeric code */
	if (atoi(input, &month) == 0)
	{
		*output = month;
		return (month < 1 || month > 12);
	}
	for (p = Monthtab; p->code; p++)
	{
		if (sequal(input, p->code))
		{
			*output = p->month;
			return (0);
		}
	}
	return (1);
}



/*
**  MONTHSIZE -- determine size of a particular month
*/

monthsize(month, year)
int	month;
int	year;
{
	register int	size;
	extern int	Dmsize[12];
	extern int	dysize();	/* defined in ctime */

	size = Dmsize[month];
	if (month == 1 && dysize(year) == 366)
		/* This is February of a leap year */
		size++;

	return (size);
}
