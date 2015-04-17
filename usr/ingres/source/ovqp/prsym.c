# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"

prsym(ss)
struct symbol	*ss;
{
	char		*p;
	register	i,j;
	register struct symbol	*s;
	char			temp[4];

	s = ss;
	i = s->type;
	j = s->len & 0377;
	p = (char *) s->value;
	if (i == S_VAR)
	{
		i = VAR;	/* execute var portion */
		printf("s_");	/* first part of "s_var" message */
	}
	if (i == VAR)
	{
		/* beware : do S_VAR's have attno's? */
		printf("var:att#=%d:", *p);
		i = ((struct qt_v *)s)->vtype;
		j = ((struct qt_v *)s)->vlen & 0377;
		p = (char *) ((struct qt_v *)s)->vpoint;
		if (i != CHAR)
		{
			/* move INT & FLOAT to word boundary */
			bmove(p, temp, 4);
			p = temp;
		}
	}

	printf("%d:%d:value='",i,j);
	switch (i)
	{
	  case AND:
	  case AOP:
	  case BOP:
	  case OR:
	  case RESDOM:
	  case UOP:
		printf("%d (operator)", *p);
		break;

	  case INT:
		switch(j)
		{
		  case 1:
			printf("%d", i1deref(p));
			break;
		  case 2:
			printf("%d", i2deref(p));
			break;
		  case 4:
			printf("%s", locv(i4deref(p)));
		}
		break;

	  case FLOAT:
		switch(j)
		{
		  case 4:
			printf("%10.3f", f4deref(p));
			break;
		  case 8:
			printf("%10.3f", f8deref(p));
		}
		break;

	  case RESULTID:
	  case SOURCEID:
	  case CHAR:
		prstr(p, j);
		break;

	  case AGHEAD:
	  case BYHEAD:
	  case QLEND:
	  case ROOT:
	  case TREE:
		printf(" (delim)");
		break;

	  case CHANGESTRAT:
	  case REOPENRES:
	  case EXIT:
	  case QMODE:
	  case RETVAL:
	  case USERQRY:
		if (j)
			printf("%d", *p);
		printf(" (status)");
		break;

	  default:
		printf("\nError in prsym: bad type= %d\n",i);
	}
	printf("'\n");
}


prstack(ss)
struct symbol	*ss;
{
	register struct symbol	*s;
	register char		*p;

	s = ss;

	if (s->type == CHAR)
	{
		printf("%d:%d:value='", s->type, s->len);
		prstr(cpderef(s->value), s->len & 0377);
		printf("'\n");
	}
	else
		prsym(s);
}


prstr(pt, len)
char	*pt;
int	len;
{
	register char	*p;
	register int	l;

	l = len + 1;
	p = pt;
	while (--l)
		putchar(*p++);
}
