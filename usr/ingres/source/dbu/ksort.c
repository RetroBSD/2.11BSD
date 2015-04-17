# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../access.h"

# define	N	7
# define	MEM	(32768 - 2)
# define	BUCKETSIZE	4
# define	ENDKEY	MAXDOM + 1

/*
**	Trace info comes from trace flag Z23 passed as the
**	second parameter. The bits used are:
**
**		0001	main trace info
**		0002	secondary trace info
**		0004	terciary trace info
**		0010	don't truncate temps
**		0020	don't unlink temps
**		0040	print am page refs
**		0100	print am tuple gets
*/

char			*Infile;
char			*Outfile;
struct descriptor	Desc;
char			Descsort[MAXDOM+1];
FILE			*Oiop;
int			Trace;
int			Tupsize;
int			Bucket;
char			File[15];
char			*Fileset;
char			*Filep;
int			Nfiles = 1;
int			Nlines;
long			Ccount;
char			**Lspace;
char			*Tspace;
extern			term();
extern int		cmpa();
long			Tupsout;

main(argc, argv)
char **argv;
{
	extern char	*Proc_name;
	extern		(*Exitfn)();
	register int	i;
	register int	j;
	register char	*mem;
	char		*start;
	int		maxkey, rev;
	char		*sbrk();

	Proc_name = "KSORT";
	Exitfn = &term;

	Fileset = *++argv;
	atoi(*++argv, &Trace);
	if ((i = open(*++argv, 0)) < 0)
		cant(*argv);
	if (read(i, &Desc, sizeof Desc) < sizeof Desc)
		syserr("read(Desc)");
	close(i);

	/* set up Descsort to indicate the sort order for tuple */
	/* if domain zero is given prepare to generate "hash bucket"
	** value for tuple */

	maxkey = 0;
	for (i = 0; i <= Desc.relatts; i++)
		if (j = Desc.relgiven[i])
		{
			if ((rev = j) < 0)
				j = -j;
			if (maxkey < j)
				maxkey = j;
			Descsort[--j] = rev < 0 ? -i : i;
		}

	Descsort[maxkey] = ENDKEY;	/* mark end of list */

	Tupsize = Desc.relwid;

	if (Bucket = (Descsort[0] == 0))
	{
		/* we will be generating hash bucket */
		Tupsize += BUCKETSIZE;
		Desc.relfrml[0] = BUCKETSIZE;
		Desc.relfrmt[0] = INT;
		Desc.reloff[0] = Desc.relwid;
	}

	if (Trace & 01)
	{
		printf("Bucket is %d,Sort is:\n", Bucket);
		for (i = 0; (j = Descsort[i]) != ENDKEY; i++)
			printf("Descsort[%d]=%d\n", i, j);
	}
	if (i = (maxkey - Bucket - Desc.relatts))
		syserr("%d domains missing\n", -i);
	Infile = *++argv;
	Outfile = *++argv;

	/* get up to 2**15 - 1 bytes of memory for buffers */
	/* note that mem must end up positive so that Nlines computation is right */
	start = sbrk(0); Lspace = (char **) start;
	mem = start + MEM;	/* take at most 2**15 - 1 bytes */
	while (brk(mem) == -1)
		mem -= 512;
	mem -= start;

	/* compute pointers and sizes into buffer memory */
	Nlines = (unsigned int) mem / (Tupsize + 2);
	Tspace = (char *) (Lspace + Nlines);
	if (Trace & 01)
		printf("Tspace=%l,Lspace=%l,Nlines=%l,mem=%l,start=%l\n",
		Tspace, Lspace, Nlines, mem, start);

	/* set up temp files */
	concat(ztack("_SYSS", Fileset), "Xaa", File);
	Filep = File;
	while (*Filep != 'X')
		Filep++;
	Filep++;
	if ((signal(2, 1) & 01) == 0)
		signal(2, term);

	/* sort stage -- create a bunch of temporaries */
	Ccount = 0;
	if (Trace & 01)
		printf("sorting\n");
	sort();
	close(0);
	if (Trace & 01)
	{
		printf("done sorting\n%s tuples written to %d files\n", locv(Tupsout), Nfiles - 1);
		printf("sort required %s compares\n", locv(Ccount));
	}

	/* merge stage -- merge up to N temps into a new temp */
	Ccount = 0;
	for (i = 1; i + N < Nfiles; i += N) 
	{
		newfile();
		merge(i, i + N);
	}

	/* merge last set of temps into target file */
	if (i != Nfiles) 
	{
		oldfile();
		merge(i, Nfiles);
	}
	if (Trace & 01)
	{
		printf("%s tuples in out file\n", locv(Tupsout));
		printf("merge required %s compares\n", locv(Ccount));
	}
	term(0);
}

sort()
{
	register char	*cp;
	register char	**lp;
	register int	i;
	int		done;
	long		ntups;
	struct tup_id	tid, ltid;
	char		*xp;
	long		pageid;
	long		rhash();

	done = 0;
	ntups = 0;
	Tupsout = 0;
	if ((Desc.relfp = open(Infile, 0)) < 0)
		cant(Infile);
	Desc.relopn = (Desc.relfp + 1) * 5;

	/* initialize tids for full scan */
	pageid = 0;
	tid.line_id = -1;
	stuff_page(&tid, &pageid);
	pageid = -1;
	ltid.line_id = -1;
	stuff_page(&ltid, &pageid);

	do 
	{
		cp = Tspace;
		lp = Lspace;
		while (lp < Lspace + Nlines)
		{
			if ((i = get(&Desc, &tid, &ltid, cp, TRUE)) != 0)
			{
				if (i < 0)
					syserr("get %d", i);
				close(Desc.relfp);
				Desc.relopn = 0;
				done++;
				break;
			}
			if (Bucket)
			{
				/* compute hash bucket and insert at end */
				pageid = rhash(&Desc, cp);
				bmove(&pageid, cp + Desc.relwid, BUCKETSIZE);
			}
			*lp++ = cp;
			cp += Tupsize;
			ntups++;
		}
		qsort(Lspace, lp - Lspace, 2, &cmpa);
		if (done == 0 || Nfiles != 1)
			newfile();
		else
			oldfile();
		while (lp > Lspace) 
		{
			cp = *--lp;
			xp = cp;
			if ((lp == Lspace) || (cmpa(&xp, &lp[-1]) != 0))
			{
				if ((i = fwrite(cp, 1, Tupsize, Oiop)) != Tupsize)
					syserr("cant write outfile %d (%d)", i, Nfiles);
				Tupsout++;
			}
		}
		fclose(Oiop);
	} while (done == 0);
	if (Trace & 01)
		printf("%s tuples in\n", locv(ntups));
}

struct merg
{
	char		tup[MAXTUP+BUCKETSIZE];
	int		file_num;
	FILE		*fiop;
};

merge(a, b)
int	a;
int	b;
{
	register struct merg	*merg;
	register int		i, j;
	char			*f, *yesno;
	struct merg		*mbuf[N + 1];
	char			*setfil();

	if (Trace & 02)
		printf("merge %d to %d\n", a, b);
	merg = (struct merg *) Lspace;
	j = 0;
	for (i = a; i < b; i++) 
	{
		f = setfil(i);
		mbuf[j] = merg;
		merg->file_num = i;
		if ((merg->fiop = fopen(f, "r")) == NULL)
			cant(f);
		if (!rline(merg))
			j++;
		merg++;
	}

	i = j - 1;
	if (Trace & 04)
		printf("start merg with %d\n", i);
	while (i >= 0) 
	{
		if (Trace & 04)
			printf("mintup %d\n", i);
		if (mintup(mbuf, i, &cmpa))
		{
			if (fwrite(mbuf[i]->tup, 1, Tupsize, Oiop) != Tupsize)
				syserr("cant write merge output");
			Tupsout++;
		}
		merg = mbuf[i];
		if (rline(merg))
		{
			yesno = "not ";
			if (!(Trace & 010))
			{
				/* truncate temporary files to zero length */
				yesno = "";
				close(creat(setfil(merg->file_num), 0600));
			}
			if (Trace & 02 || Trace & 010)
				printf("dropping and %struncating %s\n", yesno, setfil(merg->file_num));
			i--;
		}
	}

	fclose(Oiop);
}


mintup(mbuf, cnt, cmpfunc)
struct merg	*mbuf[];
int		cnt;
int		(*cmpfunc)();

/*
**	Mintup puts the smallest tuple in mbuf[cnt-1].
**	If the tuple is a duplicate of another then
**	mintup returns 0, else 1.
**
**	Cnt is the number of compares to make; i.e.
**	mbuf[cnt] is the last element.
*/

{
	register struct merg	**next, **last;
	struct merg		*temp;
	register int		nodup;
	int			j;

	nodup = TRUE;
	next = mbuf;
	last = &next[cnt];

	while (cnt--)
	{
		if (j = (*cmpfunc)(last, next))
		{
			/* tuples not equal. keep smallest */
			if (j < 0)
			{
				/* exchange */
				temp = *last;
				*last = *next;
				*next = temp;
				nodup = TRUE;
			}
		}
		else
			nodup = FALSE;

		next++;
	}
	return (nodup);
}


rline(mp)
struct merg	*mp;
{
	register struct merg	*merg;
	register int		i;

	merg = mp;
	if ((i = fread(merg->tup, 1, Tupsize, merg->fiop)) != Tupsize)
	{
		if (i == 0)
		{
			fclose(merg->fiop);
			return (1);
		}
		syserr("rd err %d on %s", i, setfil(merg->file_num));
	}
	return (0);
}

newfile()
{
	char	*setfil();

	makfile(setfil(Nfiles));
	Nfiles++;
}

char *setfil(i)
int	i;

/*
**	Convert the number i to a char
**	sequence aa, ab, ..., az, ba, etc.
*/

{
	register int	j;

	j = i;
	j--;
	Filep[0] = j/26 + 'a';
	Filep[1] = j%26 + 'a';
	return (File);
}

oldfile()
{
	makfile(Outfile);
	Tupsout = 0;
}

makfile(name)
char	*name;

/*
**	Create a file by the name "name"
**	and place its fio pointer in Oiop
*/

{
	if ((Oiop = fopen(name, "w")) == NULL)
		cant(name);
}

cant(f)
char	*f;
{
	syserr("open %s", f);
}

term(error)
int	error;
{
	register int	i;

	if (Nfiles == 1)
		Nfiles++;
	if (Trace & 020)
		printf("temp files not removed\n");
	else
		for (i = 1; i < Nfiles; i++) 
		{
			unlink(setfil(i));
		}
	exit(error);
}

cmpa(a, b)
char	**a;
char	**b;
{
	int			af[4];
	int			bf[4];
	char			*pa, *pb;
	register char		*tupa, *tupb;
	int			dom;
	register int		frml;
	int			frmt;
	int			off;
	int			temp;
	int			rt;
	char			*dp;

	pa = *a;
	pb = *b;
	Ccount++;
	dp = Descsort;
	while ((temp = *dp++) != ENDKEY)
	{
		if ((dom = temp) < 0)
			dom = -temp;
		frml = Desc.relfrml[dom];
		frmt = Desc.relfrmt[dom];
		off = Desc.reloff[dom];
		tupa = &pa[off];
		tupb = &pb[off];
		if (temp < 0)
		{
			tupb = tupa;
			tupa = &pb[off];
		}
		if (frmt == CHAR)
		{
			frml &= 0377;
			if (rt = scompare(tupb, frml, tupa, frml))
				return (rt);
			continue;
		}

		/* domain is a numeric type */
		if (bequal(tupa, tupb, frml))
			continue;
		/* copy to even word boundary */
		bmove(tupa, af, frml);
		bmove(tupb, bf, frml);
		tupa = (char *) &af[0];
		tupb = (char *) &bf[0];

		switch (frmt)
		{

		  case INT:
			switch (frml)
			{

			  case 1:
				return (i1deref(tupa) > i1deref(tupb) ? -1 : 1);

			  case 2:
				return (i2deref(tupa) > i2deref(tupb) ? -1 : 1);

			  case 4:
				return (i4deref(tupa) > i4deref(tupb) ? -1 : 1);
			}

		  case FLOAT:
			switch (frml)
			{

			  case 4:
				return (f4deref(tupa) > f4deref(tupb) ? -1 : 1);

			  case 8:
				return (f8deref(tupa) > f8deref(tupb) ? -1 : 1);
			}
		}
	}
	return (0);
}



/*
**	Replacement for access method routine get_page();
**	and associated globals and routines.
*/

struct accbuf	*Acc_head, Accbuf;
long		Accuread, Accuwrite;

get_page(d1, tid)
struct descriptor	*d1;
struct tup_id		*tid;
{
	register int			i;
	register struct descriptor	*d;
	long				pageid;
	register struct accbuf		*b;

	d = d1;
	if (Trace & 0100)
	{
		printf("get_page: %.14s,", d->relid);
		dumptid(tid);
	}
	b = Acc_head;
	if (b == 0)
	{
		/* initialize buffer */
		Acc_head = &Accbuf;
		b = &Accbuf;
		b->thispage = -1;
	}
	pluck_page(tid, &pageid);
	i = 0;
	if (b->thispage != pageid)
	{
		if (Trace & 040)
			printf("get_page: rdg pg %s\n", locv(pageid));
		b->thispage = pageid;
		if ((lseek(d->relfp, pageid * PGSIZE, 0) < 0) ||
		    ((read(d->relfp, b, PGSIZE)) != PGSIZE))
		{
			i = AMREAD_ERR;
		}
		Accuread++;
	}
	return (i);
}

resetacc(buf)
struct accbuf	*buf;
{
	return (0);
}

acc_err(err)
int	err;
{
	return (err);
}
