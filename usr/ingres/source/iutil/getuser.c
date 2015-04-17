# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"

/*
**  GET LINE FROM USER FILE
**
**	Given a user code (a two byte string), this routine returns
**	the line from .../files/users into `buf'.  The users
**	file is automatically opened, and it is closed if getperm
**	is called with `code' == 0.
**
**	If `code' == -1 then getuser will reinitialize itself.
**	This will guarantee that getuser will reopen the file
**	if (for example) an interrupt occured during the previous
**	call.
*/

getuser(code, buf)
char	*code;
char	buf[];
{
	static FILE	*userf;
	register char	c;
	register char	*bp;
	
	if (code == 0)
	{
		if (userf != NULL)
			fclose(userf);
		userf = NULL;
		return (0);
	}
	if (code == -1)
	{
		userf = NULL;
		return (0);
	}
	if (userf == NULL)
	{
		userf = fopen(ztack(Pathname, "/files/users"), "r");
		if (userf == NULL)
			syserr("getuser: open err");
	}
	else
	{
		rewind(userf);
	}
	
	for (;;)
	{
		bp = buf;
		if (fgetline(bp, MAXLINE, userf) == NULL)
		{
			return (1);
		}
		while ((c = *bp++) != ':')
			if (c == '\0')
			{
				return (1);
			}
		if (bequal(bp, code, 2))
		{
			return (0);
		}
	}
}
