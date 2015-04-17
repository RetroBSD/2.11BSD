#
# include	<stdio.h>

# include	"constants.h"
# include	"globals.h"
/*
**  GETKEY -- Get the optab entry for a keyword
**
**	Performs a binary search through Kwrdtab
**	for a given keyword.
**
**	Parameters:
**		key -- char * to the keywords character 
**			representation.
**
**	Returns:
**		a pointer to the optab struct node for that 
**		keyword, or 0 if not found.
**
**	Requires:
**		Kwrdtab -- to search through
**		Kwrdnum -- the number of entries in the
**			key word table [tokens.y].
**
**	Called By:
**		name() [name.c]
**
**	History:
**		6/1/78 -- (marc) written
**
*/



struct optab
*getkey(key)
char		*key;
{
	register struct optab	*op;
	int			top, bot;
	register int		k;
	extern int		Kwrdnum;

	op = Kwrdtab;
	bot = 0;
	top = Kwrdnum - 1;
	do 
	{
		k = (top + bot) / 2;
		switch (scompare(key, 0, op [k].op_term, 0))
		{

		  case 1 :
			bot = k + 1;
			break;
		
		  case 0 :
			return (&op [k]);

		  case -1 :
			top = k - 1;
			break;
		}
	}  while (bot <= top);
	return (0);
}
