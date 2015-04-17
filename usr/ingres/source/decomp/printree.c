# include	"../ingres.h"
# include  	"../tree.h"
# include	"decomp.h"


printree(p, string)
struct querytree	*p;
char			*string;
{

	printf("%s: %l\n", string, p);
	prtree1(p);
}

prtree1(p)
struct querytree *p;
{
	register struct querytree	*q;

	if ((q = p) == NULL)
		return;
	prtree1(q->left);
	prtree1(q->right);
	writenod(q);
}
