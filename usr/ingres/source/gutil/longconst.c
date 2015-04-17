/*
**  PASS LONG CONSTANT
**
**	This dipshit little routine just allows you to specify
**	a long constant.  If called as:
**
**		l = longconst(4, 3);
**
**	the long variable l will be assigned a value which is
**	4 * 2 ** 16 + 3.
**
**	Note that longconst MUST be typed as
**
**		extern long	longconst();
*/

long longconst(l)
long	l;
{
	return (l);
}
