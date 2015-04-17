# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"

/*
**  INGRES ERROR MESSAGE GENERATOR
**
**	Error message `num' is sent up towards process 1 with param-
**	eters `msg'.  This routine may have any number of parameters,
**	but the last one must be zero.
**
**	In process one, the appropriate error file is scanned for the
**	actual message.  The parameters are then substituted into that
**	message.  If the error message doesn't exist, then the first
**	parameter is printed as is.
**
**	History:
**		1/8/79 (eric) -- Removed temporary code to check for
**			old error numbers; removed 'Err_base'.
**		8/17/78 (rse) -- Removed error offset. Doesn't write
**			if W_err is negative, returns error code instead
**			of -1.
**		9/22/78 (rse) -- Added EXEC_ERROR for exec_id.
*/

error(num, msg)
int	num;
char	*msg;
{
	struct pipfrmt		s;
	register struct pipfrmt	*ss;
	register char		**x;
	extern int		W_err;

	x = &msg;
	ss = &s;
	wrpipe(P_PRIME, ss, EXEC_ERROR, 0, 0);
	ss->err_id = num;

	if (W_err >= 0)
	{
		while (*x)
		{
			wrpipe(P_NORM, ss, W_err, *x++, 0);
		}
		wrpipe(P_END, ss, W_err);
	}

	return (num);
}
