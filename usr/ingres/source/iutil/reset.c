# include	"../unix.h"

/*
**  RESET -- non-local goto
**
**	In version seven of UNIX, this routine fakes the
**	version six "reset" call.
**
**	Parameters:
**		val -- the value to return.
**
**	Returns:
**		NON-LOCAL!
**
**	Side Effects:
**		Mucho stack deallocation, etc.
**
**	Requires:
**		longjmp()
**
**	History:
**		8/15/79 (eric) (6.2/7) -- created.
*/

# ifdef xV7_UNIX


jmp_buf	Sx_buf;

reset(val)
int	val;
{
	longjmp(Sx_buf, val);
}
# endif
