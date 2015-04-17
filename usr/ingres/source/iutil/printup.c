# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"
# include	"../symbol.h"

# define	PAGELGTH	56

int	Printlgth;
extern struct out_arg	Out_arg;

printup(d, tuple)
struct descriptor	*d;
char			*tuple;
{
	register char		*ftype, *flen;
	register int		*foff;
	int			i, type;

	ftype = &d->relfrmt[1];
	flen = &d->relfrml[1];
	foff = &d->reloff[1];
	i = d->relatts;

	/* If relation is S_BINARY then print char fields escaped */
	if (d->relstat & S_BINARY)
	{
		while (i--)
			printatt((type = *ftype++) == CHAR ? BINARY : type, *flen++, &tuple[*foff++]);
	}
	else
	{
		while (i--)
			printatt(*ftype++, *flen++, &tuple[*foff++]);
	}
	printeol();
}

printatt(type, length1, value1)
char	type;
int	length1;
char	*value1;
{
	register int		length;
	register char		*value;
	char			buf[MAXFIELD];
	extern char		*locv();
	int			valbuf[4];

	putc(Out_arg.coldelim, stdout);
	length = length1;
	switch (type)
	{

	  case INT:
		value = (char *) valbuf;
		bmove(value1, value, length);
		switch (length)
		{

		  case 1:
			printfatt(Out_arg.i1width, iocv(i1deref(value)));
			break;

		  case 2:
			printfatt(Out_arg.i2width, iocv(i2deref(value)));
			break;

		  case 4:
			printfatt(Out_arg.i4width, locv(i4deref(value)));
			break;

		  default:
			syserr("printatt: i%d", length);

		}
		return (0);

	  case FLOAT:
		value = (char *) valbuf;
		bmove(value1, value, length);
		switch (length)
		{

		  case 4:
			ftoa(f4deref(value), buf, Out_arg.f4width, Out_arg.f4prec, Out_arg.f4style);
			printfatt(Out_arg.f4width, buf);
			break;

		  case 8:
			ftoa(f8deref(value), buf, Out_arg.f8width, Out_arg.f8prec, Out_arg.f8style);
			printfatt(Out_arg.f8width, buf);
			break;

		  default:
			syserr("printatt: f%d", length);

		}
		return (0);

	  case CHAR:
		length &= 0377;
		fwrite(value1, 1, length, stdout);
		if ((length = Out_arg.c0width - length) > 0)
			while (length--)
				putc(' ', stdout);
		return (0);

	  case BINARY:
		length &= 0377;
		value = value1;
		while (length--)
			xputchar(*value++);
		return (0);

	  default:
		syserr("printatt type %d", type);
	}
}


/*
**  FORMATTED ATTRIBUTE PRINT
**
**	Attribute 'value' is printed.  It is type 'type' and has a
**	field width of 'width'.
*/

printfatt(width, value)
int	width;
int	value;
{
	register char	*p;
	register int	w;
	register int	v;

	w = width;
	p = (char *) value;
	v = length(p);

	if (v > w)
	{
		/* field overflow */
		while (w--)
			putc('*', stdout);
		return;
	}

	/* output the field */
	for (w -= v; w > 0; w--)
		putc(' ', stdout);
	fwrite(p, 1, v, stdout);
}


printeol()
{
	putc(Out_arg.coldelim, stdout);
	putc('\n', stdout);
}

printeh()
{
	register int		i;

	putc(Out_arg.coldelim, stdout);
	for (i = 1; i < Printlgth; i++)
		putc('-', stdout);
	printeol();
}

printhdr(type, length1, value1)
char	type;
int	length1;
char	*value1;
{
	register int		length, i;
	register char		*value;
	char			c;

	value = value1;
	length = length1 & 0377;

	switch (type)
	{
	  case INT:
		switch (length)
		{

		  case 1:
			length = Out_arg.i1width;
			break;

		  case 2:
			length = Out_arg.i2width;
			break;

		  case 4:
			length = Out_arg.i4width;
			break;

		  default:
			syserr("printhdr: i%d", length);

		}
		break;

	  case FLOAT:
		switch (length)
		{

		  case 4:
			length = Out_arg.f4width;
			break;

		  case 8:
			length = Out_arg.f8width;
			break;

		  default:
			syserr("printhdr: f%d", length);

		}
		break;

	  case CHAR:
		if (length < Out_arg.c0width)
			length = Out_arg.c0width;
		break;

	  default:
		syserr("printhdr: type 0%o", type);
	}

	putc(Out_arg.coldelim, stdout);
	for (i = 0; i < length && i < MAXNAME; i++)
		if (c = *value++)
			putc(c, stdout);
		else
			break;

	for ( ; i < length; i++)
		putc(' ', stdout);

	Printlgth += length + 1;
}

beginhdr()
{
	Printlgth = 0;
	putchar('\n');
}
