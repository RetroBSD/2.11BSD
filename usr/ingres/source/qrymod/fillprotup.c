# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../catalog.h"
# include	"../pipes.h"
# include	"qrymod.h"

/*
**  FILLPROTUP -- fill the protection tuple from pipe.
**
**	This routine is broken off from d_prot.c so that we
**	don't get symbol table overflows.
**
**	Parameters:
**		protup -- pointer to the protection tuple to fill.
**
**	Returns:
**		A bool telling whether we have PERMIT ALL TO ALL
**		permission.
**
**	Side Effects:
**		The R_up pipe is flushed.
**
**	History:
**		4/16/80 (eric) -- broken off from d_prot for 6.2/10.
*/


extern struct pipfrmt		Pipe;

fillprotup(protup)
struct protect	*protup;
{
	register struct protect	*pt;
	register int		i;
	register char		*p;
	char			buf[30];
	int			all_pro;
	auto int		ix;
	char			ubuf[MAXLINE + 1];

	/*
	**  Fill in the protection tuple with the information
	**	from the parser, validating as we go.
	**
	**	Also, determine if we have a PERMIT xx to ALL
	**	with no further qualification case.  The variable
	**	'all_pro' is set to reflect this.
	*/

	all_pro = TRUE;
	pt = protup;

	/* read operation set */
	if (rdpipe(P_NORM, &Pipe, R_up, buf, 0) <= 0)
		syserr("fillprotup: rdp#1");
	if (atoi(buf, &ix))
		syserr("fillprotup: atoi#1 %s", buf);
	pt->proopset = ix;
	if ((pt->proopset & PRO_RETR) != 0)
		pt->proopset |= PRO_TEST | PRO_AGGR;

	/* read relation name */
	if (rdpipe(P_NORM, &Pipe, R_up, buf, 0) > MAXNAME + 1)
		syserr("fillprotup: rdp#2");
	pmove(buf, pt->prorelid, MAXNAME, ' ');

	/* read relation owner */
	if (rdpipe(P_NORM, &Pipe, R_up, buf, 0) != 3)
		syserr("fillprotup: rdp#3");
	bmove(buf, pt->prorelown, 2);

	/* read user name */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	if (sequal(buf, "all"))
		bmove("  ", pt->prouser, 2);
	else
	{
		/* look up user in 'users' file */
		if (getnuser(buf, ubuf))
			ferror(3591, -1, Resultvar, buf, 0);
		for (p = ubuf; *p != ':' && *p != 0; p++)
			continue;
		bmove(++p, pt->prouser, 2);
		if (p[0] == ':' || p[1] == ':' || p[2] != ':')
			syserr("fillprotup: users %s", ubuf);
		all_pro = FALSE;
	}

	/* read terminal id */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	if (sequal(buf, "all"))
	{
#		ifndef xV7_UNIX
		pt->proterm = ' ';
#		endif
#		ifdef xV7_UNIX
		bmove("  ", pt->proterm, 2);
#		endif
	}
	else
	{
#		ifndef xV7_UNIX
		pt->proterm = buf[3];
		buf[3] = 'x';
		if (!sequal(buf, "ttyx"))
			ferror(3590, -1, Resultvar, buf, 0);
#		endif
#		ifdef xV7_UNIX
		if (bequal(buf, "/dev/tty", 8))
			pmove(&buf[8], pt->proterm, 2, ' ');
		else if (bequal(buf, "/dev/", 5))
			pmove(&buf[5], pt->proterm, 2, ' ');
		else if (bequal(buf, "tty", 3))
			pmove(&buf[3], pt->proterm, 2, ' ');
		else
			pmove(buf, pt->proterm, 2, ' ');
#		endif
		all_pro = FALSE;
	}

	/* read starting time of day */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	if (atoi(buf, &ix))
		syserr("fillprotup: atoi#5 %s", buf);
	pt->protodbgn = ix;
	if (ix > 0)
		all_pro = FALSE;
	
	/* read ending time of day */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	if (atoi(buf, &ix))
		syserr("fillprotup: atoi#6 %s", buf);
	pt->protodend = ix;
	if (ix < 24 * 60 - 1)
		all_pro = FALSE;

	/* read beginning day of week */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	i = cvt_dow(buf);
	if (i < 0)
		ferror(3594, -1, Resultvar, buf, 0);	/* bad dow */
	pt->prodowbgn = i;
	if (i > 0)
		all_pro = FALSE;

	/* read ending day of week */
	rdpipe(P_NORM, &Pipe, R_up, buf, 0);
	i = cvt_dow(buf);
	if (i < 0)
		ferror(3594, -1, Resultvar, buf, 0);	/* bad dow */
	pt->prodowend = i;
	if (i < 6)
		all_pro = FALSE;
	
	/* finished with pipe... */
	rdpipe(P_SYNC, &Pipe, R_up);

	return (all_pro);
}
