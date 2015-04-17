# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"
# include	"../lock.h"

int	Qry_mode;	/* mode of original query (not nec same as Qmode) */


qryproc()
{
	register struct querytree	*root, *q;
	register int			i;
	int				mode, result_num, retr_uniq;
	struct querytree		*trbuild();
	extern long			Accuread, Accuwrite, Accusread;
	extern int			derror();
	extern struct querytree		*readqry();

#	ifdef xDTM
	if (tTf(76, 1))
		timtrace(23, 0);
#	endif
#	ifdef xDTR1
	if (tTf(50, 0))
	{
		Accuread = 0;
		Accusread = 0;
		Accuwrite = 0;
	}
#	endif

	/* initialize query buffer */
	initbuf(Qbuf, QBUFSIZ, QBUFFULL, &derror);

	/* init various variables in decomp for start of this query */
	startdecomp();

	/* Read in query, range table and mode */
	root = readqry();
	mode = Qmode;

	/* Initialize relation descriptors */
	initdesc(mode);

	/* re-build the tree */
	root = trbuild(root);
	if (root == NULL)
		derror(STACKFULL);


	/* locate pointers to QLEND and TREE nodes */
	for (q = root->right; q->sym.type != QLEND; q = q->right);
	Qle = q;

	for (q = root->left; q->sym.type != TREE; q = q->left);
	Tr = q;


	/* map the complete tree */
	mapvar(root, 0);

	/* set logical locks */
	if (Lockrel)
		lockit(root, Resultvar);

	/* If there is no result variable then this must be a retrieve to the terminal */
	Qry_mode = Resultvar < 0 ? mdRETTERM : mode;

	/* if the mode is retrieve_unique, then make a result rel */
	retr_uniq = mode == mdRET_UNI;
	if (retr_uniq)
	{
		mk_unique(root);
		mode = mdRETR;
	}

	/* get id of result relation */
	if (Resultvar < 0)
		result_num = NORESULT;
	else
		result_num = Rangev[Resultvar].relnum;

	/* evaluate aggregates in query */
	aggregate(root);

	/* decompose and process aggregate free query */
	decomp(root, mode, result_num);

	/* If this is a retrieve unique, then retrieve results */
	if (retr_uniq)
		pr_unique(root, Resultvar);

	if (mode != mdRETR)
		i = ACK;
	else
		i = NOACK;
	i = endovqp(i);

	/* call update processor if batch mode */
	if (i == UPDATE)
	{
		initp();
		call_dbu(mdUPDATE, -1);
	}


	/*
	** send eop back to parser to indicate completion
	** if UPDATE then return block comes from dbu else
	** return block comes from decomp
	*/
	writeback(i == UPDATE ? -1 : 1);

#	ifdef xDTM
	if(tTf(76, 1))
		timtrace(24,0);
#	endif
#	ifdef xDTR1
	if (tTf(50, 1))
	{
		printf("DECOMP read %s pages,", locv(Accuread));
		printf("%s catalog pages,", locv(Accusread));
		printf("wrote %s pages\n", locv(Accuwrite));
	}
#	endif

	/* clean decomp */
	reinit();

	/* return */
}
