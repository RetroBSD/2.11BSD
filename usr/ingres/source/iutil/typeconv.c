# include	"../symbol.h"

/*
**  CONVERT TYPES FROM EXTERNAL TO INTERNAL FORM
**
**	Type codes in external form ('c', 'f', or 'i') are converted
**	to internal form (CHAR, FLOAT, or INT) and returned.  The
**	return value is -1 if the parameter is bad.
*/

typeconv(code)
char	code;
{
	switch (code)
	{

	  case 'c':
		return (CHAR);

	  case 'f':
		return (FLOAT);

	  case 'i':
		return (INT);

	}
	return (-1);
}
