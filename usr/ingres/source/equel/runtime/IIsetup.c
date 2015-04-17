# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"IIglobals.h"

/*
**	IIsetup is called to mark the start of a retrieve.
*/

IIsetup()
{
#	ifdef xETR1
	if (IIdebug)
		printf("IIsetup\n");
#	endif
	IIin_retrieve = 1;
	IIr_sym.len = IIr_sym.type = 0;
	IIdomains = 0;
}
