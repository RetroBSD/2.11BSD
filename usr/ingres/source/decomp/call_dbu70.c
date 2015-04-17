# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"decomp.h"

int Synconly, Error_flag;

char	D_dbu70 = 1;	/* used for loading only. causes call_dbu70 to be loaded
			** instead of call_dbu.
			*/

call_dbu(code, errflag)
int	code;
int	errflag;

/*
**	Call the appropriate dbu with the arguments
**	given in the globals Pc and Pv. Code is a
**	number identifing which dbu to call. Errflag
**	indicates whether an error return from the dbu
**	is possibly expected.
**
**	If errflag < 0 then call_dbu will not wait for sync.
**	If errflag = 0 then call_dbu will syserr on an error
**	If errflag > 0 then call_dbu will return error value
*/

{
	struct pipfrmt 	pipe;
	register int 	i, j;
	register int	(*func)();
	extern char	*Pv[];
	extern int	Pc;
	extern int	create(), destroy(), resetrel();

#	ifdef xDTR1
	if (tTf(3, 0))
		printf("Calling DBU %d\n", code);
	if (tTf(3, 1))
		prargs(Pc, Pv);
#	endif

	switch (code)
	{

	  case mdCREATE:
		func = &create;
		break;

	  case mdDESTROY:
		func = &destroy;
		break;
		
	  case mdRESETREL:
		func = &resetrel;
		break;

	  default:
		func = NULL;
	}

	if (func != NULL)
	{
		Pv[Pc] = (char *) -1;
		Error_flag = (*func)(Pc, Pv);
		closecatalog(FALSE);
	}
	else
	{
		wrpipe(P_PRIME, &pipe, EXEC_DBU, 0, code);
		i = 0;
		j = Pc;
		while (j--)
			wrpipe(P_NORM, &pipe, W_dbu, Pv[i++], 0);
		wrpipe(P_END, &pipe, W_dbu);
	
		Error_flag = 0;
		if (errflag >= 0)
		{
			Synconly++;
			rdpipe(P_PRIME, &pipe);
			rdpipe(P_SYNC, &pipe, R_dbu);
			Synconly = 0;
		}
	}


	if (Error_flag && !errflag)
	{
		prargs(Pc, Pv);
		syserr("call_dbu:%d,ret %d", code, Error_flag);
	}
	return (Error_flag);

}


proc_error(s1, rpipnum)
struct pipfrmt	*s1;
int		rpipnum;

/*
**	Proc_error is called from rdpipe if an error
**	block is encountered. If Synconly is true then
**	the dbu request was initiated by decomp.
**	Otherwise the error block(s) are passed on up.
*/

{
	extern int		Synconly, Error_flag;
	register struct pipfrmt	*s;
	register int		fd;
	struct pipfrmt		t;

	fd = rpipnum;
	s = s1;

	if (Synconly)
	{
		Synconly = 0;
		Error_flag = s->err_id;
		rdpipe(P_SYNC, s, fd);		/*  throw away the error message  */
	}
	else
	{
		wrpipe(P_PRIME, &t, s->exec_id, 0, s->func_id);
		t.err_id = s->err_id;

		copypipes(s, fd, &t, W_up);
		rdpipe(P_PRIME, s);
	}
}

/*
**	SYSTEM RELATION DESCRIPTOR CACHE DEFINITION
**
**	This is defined here to prevent the ovqp definition (if any)
**	from overriding these definitions. This file is a direct
**	steal from ".../source/utility/catalog_desc.c"
*/

struct descriptor	Reldes;
struct descriptor	Attdes;
struct descriptor	Inddes;


struct desxx	Desxx[] =
{
	"relation",	&Reldes,	&Admin.adreld,
	"attribute",	&Attdes,	&Admin.adattd,
	"indexes",	&Inddes,	0,
	0
};


/*
**	override definition of userdestroy() for destroy() function.
**	Since destroy is only used by decomp to destroy _SYS relations,
**	userdestroy is not needed.
*/
userdestroy()
{
	return;
}
