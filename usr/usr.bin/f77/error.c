#include "defs"
#include "string_defs"

#ifdef	C_OVERLAY
char efilname[] = "/usr/share/misc/f77_strings";
int  efil = -1;

error(index,t,u,type)
unsigned index;
char *t, *u;
register int type;
{
	char buf[100];
	long lseek();

	if (efil < 0) {
		efil = open(efilname, 0);
		if (efil < 0) {
oops:
			perror(efilname);
			exit(1);
		}
	}
	if (lseek(efil, (long) index, 0) < 0 || read(efil, buf, sizeof(buf)) <= 0)
		goto oops;
	switch (type) {
	    case WARN1:
	    case WARN:
		if(nowarnflag)
			return;
		fprintf(diagfile, "Warning on line %d of %s: ", lineno, infname);
		++nwarn;
		break;

	    case YYERR:
	    case ERR2:
	    case ERR1:
	    case ERR:
		fprintf(diagfile, "Error on line %d of %s: ", lineno, infname);
		++nerr;
		break;

	    case FATAL1:
	    case FATAL:
		if (buf[0] != '\0') {
			fprintf(diagfile, buf, t, u);
			fprintf(diagfile, "\n");
		}
		fprintf(diagfile,"f77 compiler error line %d of %s: ", lineno, infname);
		fprintf(diagfile,buf,t,u);
		fputc('\n',diagfile);
		if(debugflag)
			abort();
		done(3);
		exit(3);
	    case EXECERR:
		fprintf(diagfile, "Error on line %d of %s: ", lineno, infname);
		fprintf(diagfile,"Execution error %s");
		++nerr;
		break;
	    case DCLERR:
		if(t)
		    fprintf(diagfile,"Declaration error for %s: %s\n",
		    varstr(VL, (struct nameblock *)t->varname), buf);
		else
		    fprintf(diagfile,"Declaration error %s\n", buf);
		return;

	    default:
		fprintf(diagfile,"unrecognizable error switch\n");
		exit(1);
		/*NOTREACHED*/
	}
	fprintf(diagfile,buf,t,u);
	fprintf(diagfile,"\n");
}

#else	C_OVERLAY

error(str,t,u,type)
char *str, *t, *u;
register int type;
{
	switch (type) {
	    case WARN1:
	    case WARN:
		if(nowarnflag)
			return;
		fprintf(diagfile, "Warning on line %d of %s: ", lineno, infname);
		++nwarn;
		break;

	    case YYERR:
	    case ERR2:
	    case ERR1:
	    case ERR:
		fprintf(diagfile, "Error on line %d of %s: ", lineno, infname);
		++nerr;
		break;

	    case FATAL1:
	    case FATAL:
		if (str != NULL) {
			fprintf(diagfile, str, t, u);
			fprintf(diagfile, "\n");
		}
		fprintf(diagfile,"f77 compiler error line %d of %s: ", lineno, infname);
		if(debugflag)
			abort();
		done(3);
		exit(3);
	    case EXECERR:
		fprintf(diagfile, "Error on line %d of %s: ", lineno, infname);
		fprintf(diagfile, "Execution error %s");
		++nerr;
		break;
	    case DCLERR:
		if(t)
		    fprintf(diagfile,"Declaration error for %s: %s\n",
		    varstr(VL, (struct nameblock *)t->varname), str);
		else
		    fprintf(diagfile,"Declaration error %s\n", str);
		return;

	    default:
		fprintf(diagfile,"unrecognizable error switch\n");
		exit(1);
		/*NOTREACHED*/
	}
	fprintf(diagfile,str,t,u);
	fprintf(diagfile,"\n");
}

#endif	C_OVERLAY
