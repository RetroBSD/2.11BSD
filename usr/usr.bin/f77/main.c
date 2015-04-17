char *xxxvers[] = "\nFORTRAN 77 PASS 1, VERSION 1.21,  20 APRIL 1979\n";

#include "defs"
#include "string_defs"


main(argc, argv)
int argc;
char **argv;
{
char *s;
int k, retcode;
FILEP opf();

#define DONE(c)	{ retcode = c; goto finis; }

--argc;
++argv;

while(argc>0 && argv[0][0]=='-')
	{
	for(s = argv[0]+1 ; *s ; ++s) switch(*s)
		{
		case 'w':
			if(s[1]=='6' && s[2]=='6')
				{
				ftn66flag = YES;
				s += 2;
				}
			else
				nowarnflag = YES;
			break;

		case 'U':
			shiftcase = NO;
			break;

		case 'u':
			undeftype = YES;
			break;

		case 'O':
			optimflag = YES;
			if( isdigit(s[1]) )
				{
				k = *++s - '0';
				if(k > MAXREGVAR)
					{
					error("-O%d: too many register variables", k,0,WARN1);
					maxregvar = MAXREGVAR;
					}
				else
					maxregvar = k;
				}
			break;

		case 'd':
			debugflag = YES;
			break;

		case 'p':
			profileflag = YES;
			break;

		case 'C':
			checksubs = YES;
			break;

		case '1':
			onetripflag = YES;
			break;

		case 'I':
			if(*++s == '2')
				tyint = TYSHORT;
			else if(*s == '4')
				{
				shortsubs = NO;
				tyint = TYLONG;
				}
			else if(*s == 's')
				shortsubs = YES;
			else
				error("invalid flag -I%c\n", *s,0,FATAL1);
			tylogical = tyint;
			break;

		default:
			error("invalid flag %c\n", *s,0,FATAL1);
		}
	--argc;
	++argv;
	}

if(argc != 4)
	error("arg count %d", argc,0,FATAL1);
asmfile  = opf(argv[1]);
initfile = opf(argv[2]);
textfile = opf(argv[3]);

initkey();
if(inilex( copys(argv[0]) ))
	DONE(1);
fprintf(diagfile, "%s:\n", argv[0]);
fileinit();
procinit();
if(k = yyparse())
	{
	fprintf(diagfile, "Bad parse, return code %d\n", k);
	DONE(1);
	}
if(nerr > 0)
	DONE(1);
if(parstate != OUTSIDE)
	{
	error("missing END statement",0,0,WARN);
	endproc();
	}
doext();
preven(ALIDOUBLE);
prtail();
#if FAMILY==SCJ
	puteof();
#endif
DONE(0);


finis:
	done(retcode);
	return(retcode);
}



done(k)
int k;
{
static int recurs	= NO;

if(recurs == NO)
	{
	recurs = YES;
	clfiles();
	}
exit(k);
}


LOCAL FILEP opf(fn)
char *fn;
{
FILEP fp;
if( fp = fopen(fn, "w") )
	return(fp);

error("cannot open intermediate file %s", fn,0,FATAL1);
/* NOTREACHED */
}



LOCAL clfiles()
{
clf(&textfile);
clf(&asmfile);
clf(&initfile);
}


clf(p)
FILEP *p;
{
if(p!=NULL && *p!=NULL && *p!=stdout)
	{
	if(ferror(*p))
		error("writing error",0,0,FATAL);
	fclose(*p);
	}
*p = NULL;
}

