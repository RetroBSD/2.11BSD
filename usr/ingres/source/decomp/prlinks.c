# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"


prlinks(label,linkmap)
char	*label;
char	linkmap[];
{

	register char	*lm;
	register int	i;

	printf("\n%s: ", label);
	lm = linkmap;
	for (i = 0; i < MAXDOM; i++)
		if (*lm++)
			printf("dom %d,", i);
	putchar('\n');
}
