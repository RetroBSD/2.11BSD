# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
**	SYSTEM RELATION DESCRIPTOR CACHE DEFINITION
**
**	***  TOOLED FOR QUERY MODIFICATION PROCESS  ***
**
**	History:
**		2/14/79 -- version 6.2 released.
*/

struct descriptor	Reldes;
struct descriptor	Prodes;
struct descriptor	Intdes;
struct descriptor	Treedes;


struct desxx	Desxx[] =
{
	"relation",	&Reldes,	&Admin.adreld,
	"protect",	&Prodes,	0,
	"integrities",	&Intdes,	0,
	"tree",		&Treedes,	0,
	0
};
