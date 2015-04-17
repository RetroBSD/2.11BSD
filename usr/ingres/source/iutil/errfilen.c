# include	"../ingres.h"
# include	"../aux.h"

/*
** Errfilen() -- Returns the pathname where the error file can be found
**
**	It is assumed that the error digit cannot be more than 999
*/

char *errfilen(digit)
int	digit;
{
	register char	*cp;
	char		temp[100], ver[12];
	char		*ztack(), *concat(), *iocv();
	extern char	Version[];

	/* first form the string for version A.B_X */
	/* only the first 7 chars of VERSION are accepted */
	pmove(Version, ver, sizeof (ver) - 5, '\0');

	/* make sure any mod number is removed */
	for (cp = ver; *cp; cp++)
		if (*cp == '/')
			break;

	/* now insert the "_X" */
	*cp++ = '_';
	smove(iocv(digit), cp);

	/* now put "/files/error" into temp */
	concat("/files/error", ver, temp);

	return (ztack(Pathname, temp));
}
