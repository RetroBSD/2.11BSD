# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"../batch.h"
# include	"ovqp.h"

/*
**	This file contains all the routines needed
**	for communicating with the equel process.
**	They are called only if the flag Equel = TRUE.
*/

/* global equel pipebuffer */
struct pipfrmt	Eoutpipe;


equelatt(ss)
struct symbol	*ss;

/*
**	equelatt writes one symbol pointed to
**	by ss up the data pipe to the equel
**	process.
**
**	if a symbol is a character then *ss->value
**	contains a pointer to the character string.
**	otherwise the value is stored in successive
**	words starting in ss->value.
*/

{
#	ifdef xOTR1
	if (tTf(20, 0))
		prstack(ss);
#	endif
	pwritesym(&Eoutpipe, W_front, ss);
}


equeleol(code)
int	code;

/*
**	equeleol is called at the end of the interpretation of
**	a tuple. Its purpose is to write an end-of-tuple
**	symbol to the equel process and flush the pipe.
**
**	It is also called at the end of a query to write
**	an exit symbol to equel.
**
**	History:
**		1/22/79 -- (marc) modified so won't flush pipe
**			buffer after each tuple
*/

{
	register int	mode;
	struct symbol	symb;

	if (code == EXIT)
		mode = P_END;
	else
		mode = P_FLUSH;

	symb.type = code;
	symb.len = 0;

#	ifdef  xOTR1
	if (tTf(20, 3))
		printf("equeleol:writing %d to equel\n", code);
#	endif

	wrpipe(P_NORM, &Eoutpipe, W_front, &symb, 2);
	if (mode != P_FLUSH || Equel != 2)
		wrpipe(mode, &Eoutpipe, W_front);
}


pwritesym(pipedesc, filedesc, ss)
struct pipfrmt	*pipedesc;
int		filedesc;
struct stacksym	*ss;

/*
**	pwritesym write the stacksymbol
**	pointed to by "ss" to the pipe
**	indicated by filedesc.
**
**	The destination will either be equel
**	or decomp
**
**	Since a CHAR isn't stored immediately following
**	the type and len of the symbol, A small bit
**	of manipulation must be done.
*/

{
	register struct stacksym	*s;
	register char			*p;
	register int			length;

	s = ss;
	length = s->len & 0377;

	if (s->type  == CHAR)
	{
		wrpipe(P_NORM, pipedesc, filedesc, s, 2);	/* write the type and length */
		p = cpderef(ss->value);		/* p points to the string */
	}
	else
	{
		p = (char *) ss;
		length += 2;	/* include two bytes for type and length */
	}
	wrpipe(P_NORM, pipedesc, filedesc, p, length);
}
