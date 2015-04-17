#
# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"IIglobals.h"


/*
**  INTEGER OUTPUT CONVERSION
**
**	The integer `i' is converted to ascii and put
**	into the static buffer `buf'.  The address of `buf' is
**	returned.
*/

char *IIitos(i1)
int	i1;
{
	register char	*a;
	register int	i;
	static char	buf[7];

	i = i1;
	if (i < 0)
		i = -i;

	a = &buf[6];
	*a-- = '\0';
	do
	{
		*a-- = i % 10 + '0';
		i /= 10;
	} while (i);
	if (i1 < 0)
		*a-- = '-';

	a++;
	return (a);
}
