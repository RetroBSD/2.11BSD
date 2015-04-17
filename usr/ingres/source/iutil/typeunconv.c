# include	"../symbol.h"

/*
**  CONVERT TYPES FROM INTERNAL TO EXTERNAL FORM
**
**	Type codes in internal form (CHAR, FLOAT, or INT) are
**	converted to external form ('c', 'f', or 'i') and returned.
**	The return value is -1 if the parameter is bad.
*/

typeunconv(type)
int	type;
{
	switch (type)
	{

	  case CHAR:
		return ('c');

	  case FLOAT:
		return ('f');

	  case INT:
		return ('i');

	}
	return (-1);
}
