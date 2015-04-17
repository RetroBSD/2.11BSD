# include	"../ingres.h"
# include	"../access.h"
# include	"../aux.h"
# include	"../lock.h"




/*
**  PRINT RELATION
**
**	Parmv[0] through Parmv [parmc -1] contain names
**	of relations to be printed on the standard output.
**	Note that this calls printatt, so all the output formatting 
**	features associated with printatt are available here.
**
**	Parmv[parmc] is -1 for a "normal" print, -2 for headers on
**	every page, and -3 for all headers and footers suppressed.
**	err_array is set to 0 for a good relation, 1 for no
**	relation by that name, and -1 for a view.
**
**	Uses trace flag 8
**
**	Returns:
**		0 if no errors else the last error number
**
**	Diagnostics:
**		5001 -- non-existant relation
**		5002 -- tried to print a view
**		5003 -- protection violation
**
**	History:
**		12/18/78 (rse) -- added protection check, modified the way
**				err_array was handled.
**		9/12/78 -- (marc) err_array[] added, and openr()
**			   call split in two to allow error messages
**			   when trying to print views. error #s
**			   5002 & 3 added.
**		9/25/78 -- (marc) openr's merged after openr of view error
*/

print(parmc, parmv)
int	parmc;
char	**parmv;
{
	struct descriptor		d;
	extern struct descriptor	Attdes;
	struct attribute		att, katt;
	char				tuple[MAXTUP];
	struct tup_id			tid, limtid;
	struct tup_id			stid;
	register int			i;
	register int			ern;
	register char			*name;
	extern struct out_arg		Out_arg;
	int				mode;
	int				lineno;
	int				pc;
	int				err_array[MAXPARMS];

#	ifdef xZTR1
	if (tTf(8, -1) || tTf(0, 8))
		printf("entering print\n");
#	endif
	mode = (int) parmv[parmc] + 2;
	opencatalog("attribute", 0);

	for (pc = 0; pc < parmc; pc++)
	{
		name = parmv[pc];

		ern = openr(&d, 0, name);
		if (ern == AMOPNVIEW_ERR)
		{
			err_array[pc] = 5002;
			continue;
		}
		if (ern > 0)
		{	
			err_array[pc] = 5001;
			continue;
		}
		if (ern < 0)
			syserr("printr:openr target %s, %d",
			name, ern);
		if ((d.relstat & S_PROTALL) && (d.relstat & S_PROTRET) &&
			!bequal(Usercode, d.relowner, 2))
		{
			err_array[pc] = 5003;
			continue;
		}


		/* a printable relation */
		err_array[pc] = 0;
#		ifdef xZTR2
		if (tTf(8, 1))
			printdesc(&d);
#		endif
		lineno = Out_arg.linesperpage - 6;
		if (mode >= 0)
		{
			if (mode == 0)
				putchar('\014');	/* form feed */
			printf("\n%s relation\n", name);
			lineno -= 2;
		}
	
		find(&d, NOKEY, &tid, &limtid);
	
		if (Lockrel)
			setrll(A_SLP, d.reltid, M_SHARE);	/* set shared lock on relation*/
		for (;;)
		{
			if (mode >= 0)
			{
				beginhdr();
				seq_init(&Attdes, &d);
				for (i = 1; seq_attributes(&Attdes, &d, &att); i++)
				{
					printhdr(d.relfrmt[i], d.relfrml[i], att.attname);
				}
				printeol();
				printeh();
				lineno -= 3;
			}
	
#			ifdef xZTM
			if(tTf(76, 1))
				timtrace(29, 0);
#			endif
			while ((ern = get(&d, &tid, &limtid, tuple, TRUE)) == 0)
			{
				printup(&d, tuple);
				if (mode == 0 && --lineno <= 0)
				{
					printf("\n\n\n\n\n\n");
					lineno = Out_arg.linesperpage - 6;
					break;
				}
			}
#			ifdef xZTM
			if(tTf(76, 1))
				timtrace(30, 0);
#			endif
			if (ern > 0)
				break;
	
			if (ern < 0)
				syserr("print: get %d", ern);
		}
	
		if (mode >= 0)
			printeh();
		if (Lockrel)
			unlrl(d.reltid);	/* release relation lock */
	
		closer(&d);
	}
	/* check for any error messages that should be printed */
	ern = 0;
	for (pc = 0; pc < parmc; pc++)
	{
		if (i = err_array[pc])
		{
			ern = error(i, parmv[pc], 0);
		}
	}
	return (ern);
}
