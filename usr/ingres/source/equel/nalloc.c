/*
**  NALLOC.C -- Dynamic core allocation routines
**
**	Defines:
**		nalloc()
**		xfree()
**		salloc()
**
**	History:
**		77 -- (marc) written
**		(xfree is modified from a routine
**		stolen from bill joy)
*/

/*
**	NALLOC --
**		Dynamic allocation routine which
** 		merely calls alloc(III), returning 
**		0 if no core and a pointer otherwise.
**
*/

char *nalloc(s)
int	s;
{
	extern char	*malloc();
	
	return (malloc(s));
}
/*
**	SALLOC -- allocate
**		place for string and initialize it,
**		return string or 0 if no core.
**
*/

char *salloc(s)
char		*s;
{
	register int		i;
	register char		*string;
	extern char		*malloc();

	string = s;
	i = (int) malloc(length(string) + 1);
	if (i == 0)
		return (0);

	smove(string, i);
	return ((char *)i);
}

/*
**	XFREE -- Free possibly dynamic storage
**		checking if its in the heap first.
**
**		0 - freed
**		1 - not in heap
**
*/

xfree(cp)
char		*cp;
{
	extern 			end;	/* break (II) */
	register char		*lcp, *lend, *lacp;

	lcp = cp;
	lacp = (char *) &cp;
	lend = (char *) &end;
	if (lcp >= lend && lcp < lacp)	/* make sure its in heap */
	{
		free(lcp);
		return (0);
	}
	return (1);
}
