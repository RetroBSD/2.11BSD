# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  WRITE OUT QUERY BUFFER TO UNIX FILE
**
**	The logical buffer is written to a UNIX file, the name of which
**	must follow the \w command.
**
**	Uses trace flag 18
*/

writeout()
{
	register int	i;
	register char	*file;
	register int	source;
	int		dest;
	char		buf[512];
	char		*getfilename();

	file = getfilename();
	if (file[0] == 0 || file[0] == '-')
	{
		printf("Bad file name \"%s\"\n", file);
		return;
	}

	if ((dest = creat(file, 0644)) < 0)
	{
		printf("Cannot create \"%s\"\n", file);
		return;
	}

	if (!Nautoclear)
		Autoclear = 1;

	if ((source = open(Qbname, 0)) < 0)
		syserr("writeout: open(%s)\n", Qbname);

	fflush(Qryiop);

	while ((i = read(source, buf, sizeof buf)) > 0)
		write(dest, buf, i);

	close(source);
	close(dest);
}
