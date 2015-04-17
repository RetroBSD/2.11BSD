# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
**	SYSTEM RELATION DESCRIPTOR CACHE DEFINITION
**
*/

struct descriptor	Reldes;
struct descriptor	Attdes;
struct descriptor	Inddes;
struct descriptor	Treedes;
struct descriptor	Prodes;
struct descriptor	Intdes;


struct desxx	Desxx[] =
{
	"relation",	&Reldes,	&Admin.adreld,
	"attribute",	&Attdes,	&Admin.adattd,
	"indexes",	&Inddes,	0,
	"tree",		&Treedes,	0,
	"protect",	&Prodes,	0,
	"integrities",	&Intdes,	0,
	0
};
