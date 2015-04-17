main()
{
	short	s;
	int	i;
	long	l;
	float	f;
	char	*p;

	s = "s = ...";		/* $L4 assigned to s		*/
	i = "i = ...";		/* $L5 assigned to i		*/

	l = "l = ...";		/* $L6 assigned to low word of l,
				 * upper word cleared
				 */

/*	f = "f = ...";		/* totally illegal		*/

	s = p;			/* p assigned to s		*/
	i = p;			/* p assigned to i		*/

	l = p;			/* p assigned to low word of l,
				 * upper word cleared.
				 */

/*	f = p;			/* totally illegal		*/

	p = 'a';
	p = 5;
	p = 15L;
	p = 100000L;		/* -74540(8) is assigned to p, with no
				 * truncation warning, but then the same
				 * thing happens with the next statement,
				 * so this is a global error
				 */

	i = 100000L;		/* -74540(8) assigned to i	*/

	p = s;			/* s assigned to p		*/
	p = i;			/* i assigned to p		*/
	p = l;			/* low word of l assigned to p	*/
/*	p = f;			/* totally illegal		*/

	l = i;			/* i assigned to low word of l,
				 * upper word sign extended
				 */
}
