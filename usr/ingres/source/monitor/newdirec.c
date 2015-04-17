# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  CHANGE WORKING DIRECTORY
*/

newdirec()
{
	register char	*direc;
	char		*getfilename();

	direc = getfilename();
	if (chdir(direc))
		printf("Cannot access directory \"%s\"\n", direc);
}
