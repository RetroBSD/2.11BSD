/*
 * mkovmake.c	by dennisf@ndcvx.cc.nd.edu  March 1990
 *	makes an overlay makefile for specified object modules.
 *  v1.1: adds -v option, and #defines for ovinit and module[].overlay.
 *  v1.2: -o option renamed to -f, new -o and -l options for output name
 *	and libraries.
 *  v2: Overlay optimizer!  Many changes, practically different program.
 *  v3: Modified to use new object file (strings table) format. 1/9/94 - sms.
 */

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <sys/param.h>
#include <a.out.h>
#include <sys/file.h>

#define MAXSYMLEN 32
#define IN_BASE	0
#define UNCOMMITTED	-1
#define isobj(name)	name[0] && name[0] != '-' && rindex(name,'.') \
	&& !strcmp(rindex(name,'.'), ".o")
#define isarc(name)	name[0] && name[0] != '-' && rindex(name,'.') \
	&& !strcmp(rindex(name,'.'), ".a")

struct modstruct
{
	char *name;
	unsigned text;
	short overlay;
	char **textnames;
	char **undfnames;
} *module;
FILE *output;
char *malloc(), *realloc(), *adlib(), *libs;
int optimize, listnames;
double metoo();

main(argc, argv)
int argc;
char **argv;
{
	register i, n;
	int ovinit;	/* initial ov number, IN_BASE or UNCOMMITTED */
	int j;		/* redundant-module index */
	int nobj;	/* number of modules, used for malloc()ing */
	long ovtext;	/* total size of text of UNCOMMITTED modules */
	int ov;		/* overlay number, used in for loop */
	int max_ovs;	/* max number of ovs mkovmake can make */
	int ovs_used;	/* number of ovs actually used */
	int modsleftout;/* number of modules not assigned to an ov */
	unsigned max_ovsize;	/* sizeof biggest overlay, multiple of 8k */
	unsigned bigovmod;	/* sizeof biggest module not assigned to base */
	unsigned ovspace;	/* space occupied in current ov */
	unsigned basesize;	/* limit on base, if optimizing */
	int mostroutines;	/* module with biggest global text routines
				   per kb text ratio, if optimizing base */
	double baseopt;		/* the best such ratio so far */
	int wannabe;	/* module that most wants to be on the overlay,
			   if optimizing */
	double affinity;/* metoo(ov,wannabe) */
	double affinn;	/* metoo(ov,n) */
	int Zused;	/* -Z option specified somewhere used in -O2 */
	long tmpl;
	char *makename, *program;
	char *arg, switchc;

	/* Usage */
	if (argc == 1)
	{
usage:		fprintf(stderr,
		 "usage:  mkovmake [-On [basesize] [-n]] [-fmakefile] [-sovsize] [-vmax_ovs]\n");
		fprintf(stderr, "\t\t[-oprogram] [-llibs ...] bases.o ... -Z ovmodules.o ...\n");
		exit(1);
	}

	/* Set defaults */
	Zused = listnames = bigovmod = ovtext = nobj = optimize = 0;
	output = stdout;
	max_ovsize = basesize = UNCOMMITTED;
	ovinit = IN_BASE;
	max_ovs = NOVL;
	program = "$(PROGRAM)";
	libs = malloc(2);
	libs[0] = 0;
	/* First parsing: get options, count object modules */
	for (i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-')
		{
			if (isobj(argv[i]))
				++nobj;
			else if (isarc(argv[i]))
				adlib(argv[i]);
			else
			{
				fprintf(stderr, "mkovmake:  %s: unknown file type.\n",
				 argv[i]);
				exit(5);
			}
		}
		else
		{
			switchc = argv[i][1];
			arg = argv[i] + 2;
			if (!arg[0])
				arg = argv[++i];
			switch (switchc)
			{
			case 'O':
				optimize = 1; /* Use given BASE */
				if (arg[0] == '2')
				{
					optimize = 2; /* Determine BASE */
					if (isdigit(argv[i+1][0]))
					{
						basesize = atoi(argv[++i]);
						if (index(argv[i],'k')
						 || index(argv[i],'K'))
							basesize *= 1024;
						if (basesize < 10)
							basesize *= 8192;
					}
				}
				else
					--i; /* no argument */
				break;
			case 'n':
				++listnames;
				--i; /* no argument */
				break;
			case 'Z':
				if (optimize != 2 && !nobj)
					fprintf(stderr, "Nothing in the BASE?  Ok...\n");
				++Zused;
				--i; /* no argument */
				break;
			case 'f':
				makename = arg;
				if ((output = fopen(makename, "w")) == NULL)
				{
					fprintf(stderr,
				 	"%s: cannot open for output.\n", makename);
					exit(2);
				}
				break;
			case 's':
				max_ovsize = atoi(arg);
				if (index(arg,'k') || index(arg,'K'))
					max_ovsize *= 1024;
				if (max_ovsize < 10)
					max_ovsize *= 8192;
				break;
			case 'v':
				max_ovs = atoi(arg);
				if (max_ovs > NOVL)
				{
					fprintf(stderr,
					 "mkovmake:  can only use %d overlays.\n",
					 NOVL);
					max_ovs = NOVL;
				}
				break;
			case 'o':
				program = arg;
				break;
			case 'l':
				adlib("-l");
				adlib(arg);
				break;
			default:
				goto usage;
			}
		}
	}
	if (!libs[0])
	{
		free(libs);
		libs = "$(LIBS) ";
	}
	if (!Zused && optimize == 2)
		ovinit = UNCOMMITTED;

	/* Second parsing: malloc for module[] array and load its members */
	if (!(module = (struct modstruct *) malloc(sizeof(struct modstruct) * ++nobj)))
	{
		fprintf(stderr, "mkovmake:  not enough memory for module list.\n");
		fprintf(stderr, "mkovmake:  can't use mkovmake!  Help!\n");
		exit(6);
	}
	for (i = 1, n = 0; i < argc; ++i)
	{
		for (j = 1; j < i; ++j)
			if (!strcmp(argv[i], argv[j]))
				break;
		if (argv[i][0] == '-' && argv[i][1] == 'Z')
			ovinit = UNCOMMITTED;
		else if (j == i && isobj(argv[i]))
		{
			module[n].name = argv[i];
			module[n].overlay = ovinit;
			getnames(n);  /* gets sizeof text and name lists */
			if (ovinit != IN_BASE)
			{
				ovtext += module[n].text;
				if (module[n].text > bigovmod)
					bigovmod = module[n].text;
			}
			++n;
		}
	}
	module[n].name = "";
	if (max_ovsize == UNCOMMITTED)
	{
		max_ovsize = (unsigned) (ovtext / (7680L * max_ovs) + 1) * 8192;
		if (bigovmod > max_ovsize)
			max_ovsize = (bigovmod / 8192 + 1) * 8192;
		fprintf(output, "# Using %dk overlays.\n", max_ovsize/1024);
	}

	/*
	 * Optimizer levels:
	 * 1: use given BASE, all other modules are UNCOMMITTED.
	 * 2: determine BASE, all modules are UNCOMMITTED.
	 * 3: programmer gets COMMITTED.
	 */
	if (optimize == 2)
	{
		if (basesize == UNCOMMITTED)
		{
			/* is this a fudge or what?? */
			tmpl = ((tmpl = 2048 + nlibs()*2048L + ovtext/5)
			 > 24576L) ? 24576L : tmpl;
			basesize = (65536L - max_ovsize - tmpl);
			fprintf(output, "# Using %u-byte base, without libraries.\n",
			 basesize);
			/* If enough people are interested, I'll make a version
			   of this that adds up routines used within libraries */
		}
		n = -1;
		while (module[++n].name[0])
			if (module[n].overlay == IN_BASE)
				basesize -= module[n].text;
		if (basesize < 0)
		{
			fprintf(stderr, "mkovmake:  specified modules don't fit in base.\n");
			fprintf(stderr, "mkovmake:  specify fewer modules or larger base.\n");
			exit(9);
		}
		do /* load the base */
		{
			baseopt = 0.;
			n = -1;
			while(module[++n].name[0])
				if (module[n].overlay != IN_BASE
				 && module[n].text
				 && module[n].text <= basesize
				 && (double) (sizeof(module[n].textnames)-1)
				 / module[n].text > baseopt)
				{
					mostroutines = n;
					baseopt = (double)
					 (sizeof(module[n].textnames)-1)
					 / module[n].text;
				}
			if (baseopt)
			{
				module[mostroutines].overlay = IN_BASE;
				basesize -= module[mostroutines].text;
			}
		} while(baseopt);
	}
	listmodules(IN_BASE);

	/*
	 * overlay packing:
	 * If not optimizing, just pack modules until no more can fit.
	 * Otherwise, add a module only if it's the one thats to be on
	 *	the ov the most, using metoo().
	 */
	for (ov = 1; ov <= max_ovs; ++ov)
	{
		ovspace = 0;
addmod:		n = -1;
		while (module[++n].name[0])
		{
			if (module[n].overlay == UNCOMMITTED
			 && module[n].text + ovspace <= max_ovsize)
			{
				module[n].overlay = ov;
				ovspace += module[n].text;
				/* optimizer needs one */
				if (optimize && module[n].text)
					break;
			}
		}
		if (!ovspace) /* max_ovsize is too small for a module */
			break;
		if (optimize && module[n].name[0])
		{
			for (;;) /* only escape is the goto! yuck! */
			{
				affinity = 0.;
				n = -1;
				while (module[++n].name[0])
				{
					if (module[n].overlay == UNCOMMITTED
					 && module[n].text
					 && module[n].text + ovspace <= max_ovsize
					 && (affinn = metoo(ov,n)) > affinity)
					{
						wannabe = n;
						affinity = affinn;
					}
				}
				if (!affinity)
					goto addmod; /* will another mod fit? */
				module[wannabe].overlay = ov;
				ovspace += module[wannabe].text;
			}
		}
		listmodules(ov);
	}
	ovs_used = ov;

	/* And what if they just don't all fit? */
	n = modsleftout = 0;
	while (module[n].name[0])
		if (module[n++].overlay == UNCOMMITTED)
			++modsleftout;
	if (modsleftout)
	{
		fprintf(stderr, "\nAfter %d overlay", ovs_used-1);
		if (ovs_used != 2)  fprintf(stderr, "s");
		fprintf(stderr, ", the following ");
		if (modsleftout == 1)
			fprintf(stderr, "module was\n");
		else
			fprintf(stderr, "%d modules were\n", modsleftout);
		fprintf(stderr,
		 "left out of the BASE and OVerlay definitions:\n");
		n = -1;
		while (module[++n].name[0])
			if (module[n].overlay == UNCOMMITTED)
				fprintf(stderr, "  %s\n", module[n].name);
		fprintf(stderr,
		 "\nYou can 1) Use more or bigger overlays,\n");
		fprintf(stderr, "  2) Use a smaller base, or\n");
		fprintf(stderr, "  3) \"Go buy a VAX.\"\n");
		fclose(output);
		exit(3);
	}

	fprintf(output,
	 "%s:\n\t/bin/ld -i -X -o %s /lib/crt0.o \\\n\t", program, program);
	fprintf(output, "$(BASE) ");
	for (ov = 1; ov < ovs_used; ++ov)
	{
		if (ov % 4 == 1)
			fprintf(output, "\\\n\t");
		fprintf(output, "-Z $(OV%d) ", ov);
	}
	fprintf(output, "\\\n\t-Y %s-lc\n", libs);
	fclose(output);
	free((char *) module);
	free(libs);
	exit(0);
}

/*
 * listmodules(ov)
 *	lists modules in overlay ov (ov=0 is BASE), each line
 *	preceded by a tab and not exceeding LISTWIDTH characters.
 */

#define LISTWIDTH	60

listmodules(ov)
int ov;
{
	int currentwidth = 0, n = -1, namelength;
	unsigned listovspace = 0;

	if (ov == IN_BASE)
		fprintf(output, "\nBASE=\t");
	else
		fprintf(output, "OV%d=\t", ov);
	while (module[++n].name[0])
	{
		if (module[n].overlay == ov)
		{
			namelength = strlen(module[n].name);
			if (currentwidth + namelength > LISTWIDTH)
			{
				fprintf(output, "\\\n\t");
				currentwidth = 0;
			}
			currentwidth += (namelength + 1);
			fprintf(output, "%s ", module[n].name);
			listovspace += module[n].text;
		}
	}
	fprintf(output, "\n# %u bytes.\n\n", listovspace);
}

/*
 * metoo()	returns how much a module wants to be on an overlay.
 */
double
metoo(ov, mod)
int ov, mod;
{
	int n, postnews;

	if (!module[mod].text)
		return(0.);
	postnews = 0;
	n = -1;
	while (module[++n].name[0])
		if (module[n].overlay == ov)
			postnews += ilikeu(n, mod) + ilikeu(mod, n);
	return((double) postnews / module[mod].text);
}

/*
 * ilikeu()	returns how much one module likes another.
 *	Just like life, it's not necessarily symmetrical.
 */
ilikeu(me, you)
int me, you;
{
	int friendly;
	char **ineed, **youhave;

	friendly = 0;  /* Who are you? */
	ineed = module[me].undfnames;
	youhave = module[you].textnames;
	while(**ineed)
	{
		while(**youhave)
		{
			if (!strcmp(*ineed, *youhave++))
			{
				++friendly;
				break;
			}
		}
		++ineed;
	}
	return(friendly);
}

/*
 * adlib(s)	adds s onto libs.
 */
char *
adlib(s)
char *s;
{
	char *morelibs;

	if (!(morelibs = realloc(libs, strlen(libs)+strlen(s)+3)))
	{
		fprintf(stderr, "mkovmake:  not enough memory for library names.\n");
		exit(7);
	}
	strcat(morelibs, s);
	if (s[0] != '-')
		strcat(morelibs, " ");
	libs = morelibs;
	return(morelibs);
}

/*
 * nlibs()	How many libs are there?
 */
nlibs()
{
	int i=0;
	char *s;
	s = libs;
	while (*s)
		if (*s++ == ' ')
			++i;
	return(i);
}

/*
 * getnames(n)	opens object module n and gets size of text,
 *	and name lists if using optimizer.
 */
getnames(n)
int n;
{
	struct xexec exp;
	struct nlist namentry;
	FILE *obj, *strfp = NULL;
	off_t stroff;
	int nundf, ntext;
	char name[MAXSYMLEN + 2];

	bzero(name, sizeof (name));
	if ((obj = fopen(module[n].name,"r")) == NULL)
	{
		fprintf(stderr, "mkovmake:  cannot open %s.\n", module[n].name);
		exit(8);
	}
	fread((char *)&exp, 1, sizeof(exp), obj);
	module[n].text = exp.e.a_text;
	if (!optimize)
	{
		fclose(obj);
		return(0);
	}

	fseek(obj, N_SYMOFF(exp), L_SET);

	ntext = nundf = 0;
	while (fread((char *)&namentry, sizeof(namentry), 1, obj) == 1)
	{
		if (feof(obj) || ferror(obj))
			break;
		if (namentry.n_type & N_EXT)
		{
			switch (namentry.n_type&N_TYPE)
			{
			case N_UNDF:
				if (!namentry.n_value)
					++nundf;
				break;
			case N_TEXT:
				++ntext;
				break;
			}
		}
	}
	module[n].textnames = (char **) malloc(++ntext * 2);
	module[n].undfnames = (char **) malloc(++nundf * 2);
	if (!module[n].textnames || !module[n].undfnames)
	{
nosyms:		fprintf(stderr, "mkovmake:  out of memory for symbols list.\n");
		fprintf(stderr, "mkovmake:  can't optimize.\n");
		optimize = 0;
		fclose(obj);
		if (strfp)
			fclose(strfp);
		return(1);
	}

	strfp = fopen(module[n].name, "r");
	ntext = nundf = 0;
	fseek(obj, N_SYMOFF(exp), L_SET);
	stroff = N_STROFF(exp);
	while (fread((char *)&namentry, sizeof(namentry), 1, obj) == 1)
	{
		if (namentry.n_type & N_EXT)
		{
			switch (namentry.n_type&N_TYPE)
			{
			case N_UNDF:
				if (!namentry.n_value)
				{
					fseek(strfp,
						stroff + namentry.n_un.n_strx,
						L_SET);
					fread(name, sizeof (name), 1, strfp);
					if (!(module[n].undfnames[nundf] 
					 	= strdup(name)))
						goto nosyms;
					nundf++;
					if (listnames)
						pname(n, name, 0);
				}
				break;
			case N_TEXT:
				fseek(strfp, stroff + namentry.n_un.n_strx,
					L_SET);
				fread(name, sizeof (name), 1, strfp);
				if (!(module[n].textnames[ntext]= strdup(name)))
					goto nosyms;
				ntext++;
				if (listnames)
					pname(n,name,1);
				break;
			}
		}
	}
	module[n].undfnames[nundf] = "";
	module[n].textnames[ntext] = "";
	fclose(obj);
	fclose(strfp);
	return(0);
}

/*
 * pname(n,s,t)
 *	prints global Text(t=1) and Undf(t=0) name s encountered in module n.
 */
pname(n,s,t)
int n,t;
char *s;
{
	if (t)
		fprintf(stderr, "%s: T %s\n", module[n].name, s);
	else
		fprintf(stderr, "%s: U %s\n", module[n].name, s);
}
