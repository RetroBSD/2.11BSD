# define	tTNFUNC		100
# define	tTNCUTOFF	70

int		tT[tTNFUNC];
char		*tTp, tTsav, tTany;
char		tTbuffer[100];

tTrace(argc, argv, flag)
int	*argc;
char	**argv;
char	flag;
{
	register int	fno;
	int		f;
	register char	**ps, **pd;
	int		cnt;

	ps = pd = argv;
	for (cnt = *argc; cnt > 0; cnt--)
	{
		if ((*ps)[0] != '-' || (*ps)[1] != flag)
		{
			*pd++ = *ps++;
			continue;
		}
		tTany++;
		tTp = tTbuffer;
		smove((*ps)+2, tTbuffer);
		(*argc)--;
		ps++;
		if (!*tTp)
		{
			for (fno = 0; fno < tTNCUTOFF; fno++)
				tTon(fno, -1);
			continue;
		}
		do
		{
			fno = tTnext();
			tTurn(fno);

			if (tTsav == '/')
			{
				f = fno + 1;
				fno = tTnext();
				while (f < fno)
					tTon(f++, -1);
				tTurn(fno);
			}
		}  while(tTsav);
	}
	*pd = 0;
}

tTnext()
{
	register char	*c;
	int		i;

	c = tTp;
	while (*tTp >= '0' && *tTp <= '9')
		tTp++;
	tTsav = *tTp;
	*tTp++ = '\0';
	atoi(c, &i);
	return (i);
}

tTurn(fno1)
int	fno1;
{
	register int	pt;
	register int	fno;

	fno = fno1;
	if (tTsav == '.')
	{
		while (tTsav == '.')
		{
			pt = tTnext();
			tTon(fno, pt);
		}
	}
	else
		tTon(fno, -1);
}


tTon(fun1, pt1)
int	fun1;
int	pt1;
{
	register int		i;
	register int		fun;
	register int		pt;

	fun = fun1;
	pt = pt1;
	if (pt >= 0)
		tT[fun] |= (1<<pt%16);
	else
		tT[fun] = 0177777;
}


/*
**  CHECK TRACE FLAG AND PRINT INFORMATION
**
**	This routine is equivalent to
**		if (tTf(m, n))
**			printf(a1, a2, a3, a4, a5, a6);
**
**	and can be called to reduce process space.  The return value
**	is the value of the flag.
*/

tTfp(m, n, a1, a2, a3, a4, a5, a6)
int	m;
int	n;
char	*a1, *a2, *a3, *a4, *a5, *a6;
{
	register int	rtval;

	if (rtval = tTf(m, n))
		printf(a1, a2, a3, a4, a5, a6);
	return (rtval);
}
