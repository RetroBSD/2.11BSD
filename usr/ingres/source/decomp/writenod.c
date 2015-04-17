# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"decomp.h"




writenod(n)
struct querytree	*n;
{
	register int			t;
	register struct querytree	*q;
	register struct symbol		*s;
	char				*cp;
	int				l;

	q = n;
	s = &q->sym;
	t = s->type;
	l = s->len & I1MASK;

	printf("%l/ %l: %l: ", q, q->left, q->right);
	printf("%d, %d, ", t, l);

	switch (t)
	{
	  case VAR:
		printf("%d,%d,%d,%d,%l/", ((struct qt_var *)q)->varno, ((struct qt_var *)q)->attno, ((struct qt_var *)q)->frmt, ((struct qt_var *)q)->frml&0377, ((struct qt_var *)q)->valptr);
		if (((struct qt_var *)q)->varno < 0) 
			writenod(((struct qt_var *)q)->valptr);
		else
			printf("\n");
		return;

	  case AND:
	  case ROOT:
	  case AGHEAD:
		printf("%d,%d,%o,%o", ((struct qt_root *)q)->tvarc, ((struct qt_root *)q)->lvarc, ((struct qt_root *)q)->lvarm, ((struct qt_root *)q)->rvarm);
		if (t != AND)
			printf(",(%d)", ((struct qt_root *)q)->rootuser);
		break;

	  case AOP:
	  case RESDOM:
		printf("%d,%d,%d", ((struct qt_res *)q)->resno, ((struct qt_var *)q)->frmt, ((struct qt_var *)q)->frml & 0377);
		if (t == AOP)
			printf("(%d,%d)", ((struct qt_ag *)q)->agfrmt, ((struct qt_ag *)q)->agfrml & 0377);
		break;

	  case UOP:
	  case BOP:
	  case COP:
	  case INT:
		switch (l)
		{
		  case 1:
			printf("%d", i1deref(s->value));
			break;
		  case 2:
			printf("%d", i2deref(s->value));
			break;
		  case 4:
			printf("%s", locv(i4deref(s->value)));
		}
		break;

	  case FLOAT:
		switch (l)
		{
		  case 4:
			printf("%.10f", f4deref(s->value));
			break;
		  case 8:
			printf("%.10f", f8deref(s->value));
		}
		break;

	  case CHAR:
		cp = (char *) s->value;
		while (l--)
			putchar(*cp++);
		break;

	  case TREE:
	  case OR:
	  case QLEND:
	  case BYHEAD:
		break;

	  default:
		printf("Error in writenod");
	}
	printf("/\n");
}
