/*
 * Interpret a C intermediate file.
 */
#include <stdio.h>
#include "c1.h"

extern	opdope[1];
extern	char	*opntab[1];

struct table cctab[1], efftab[1], regtab[1], sptab[1];
char	maprel[1], notrel[1];
char	*outname();

main()
{
	register t, op;
	static char s[9];
	register char *tp;
	double atof();
	char numbuf[64];
	int lbl, cond;
	int sdep;

	sdep = 0;
	for (;;) {
		op = getw(stdin);
		if ((op&0177400) != 0177000) {
			error("Intermediate file error");
			exit(1);
		}
		lbl = 0;
		switch(op &= 0377) {

	case SINIT:
		printf("init %d\n", getw(stdin));
		break;

	case EOFC:
		printf("eof\n");
		exit(0);

	case BDATA:
		if (getw(stdin) == 1) {
			printf(".byte ");
			for (;;)  {
				printf("%d", getw(stdin));
				if (getw(stdin) != 1)
					break;
				printf(",");
			}
			printf("\n");
		}
		break;

	case PROG:
		printf("prog\n");
		break;

	case DATA:
		printf("data\n");
		break;

	case BSS:
		printf("bss\n");
		break;

	case SYMDEF:
		printf("symdef ");
		outname(s);
		printf("%s\n", s);
		break;

	case RETRN:
		printf("return\n");
		break;

	case CSPACE:
		tp = outname(s);
		printf("comm %s,%d\n", tp, getw(stdin));
		break;

	case SSPACE:
		printf("space %d\n", getw(stdin));
		break;

	case EVEN:
		printf("even\n");
		break;

	case SAVE:
		printf("save\n");
		break;

	case SETSTK:
		t = getw(stdin)-6;
		printf("setstack %d\n", t);
		break;

	case PROFIL:
		t = getw(stdin);
		printf("profil %d\n", t);
		break;

	case SNAME:
		tp = outname(s);
		printf("sname %s s%d\n", tp, getw(stdin));
		break;

	case ANAME:
		tp = outname(s);
		printf("aname %s a%d\n", tp, getw(stdin));
		break;

	case RNAME:
		tp = outname(s);
		printf("rname %s r%d\n", tp, getw(stdin));
		break;

	case SWIT:
		t = getw(stdin);
		line = getw(stdin);
		printf("switch line %d def %d\n", line, t);
		while (t = getw(stdin)) {
			printf("   %d %d\n", t, getw(stdin));
		}
		break;

	case CBRANCH:
		lbl = getw(stdin);
		cond = getw(stdin);
	case EXPR:
		line = getw(stdin);
		if (sdep != 1) {
			error("Expression input botch");
			exit(1);
		}
		sdep = 0;
		if (lbl)
			printf("cbranch %d line %d\n", lbl, line);
		else
			printf("expr line %d\n", line);
		break;

	case NAME:
		t = getw(stdin);
		if (t==EXTERN) {
			t = getw(stdin);
			printf("name %o, %s\n", t, outname(s));
		} else if (t==AUTO) {
			t = getw(stdin);
			printf("name %o a%d\n", t, getw(stdin));
		} else if (t==STATIC) {
			t = getw(stdin);
			printf("name %o s%d\n", t, getw(stdin));
		} else if (t==REG) {
			t = getw(stdin);
			printf("name %o r%d\n", t, getw(stdin));
		} else
			printf("name botch\n");
		sdep++;
		break;

	case CON:
		t = getw(stdin);
		printf("const %d %d\n", t, getw(stdin));
		sdep++;
		break;

	case LCON:
		getw(stdin);	/* ignore type, assume long */
		t = getw(stdin);
		op = getw(stdin);
		printf("lconst %D\n", (((long)t<<16) | (unsigned)op));
		sdep++;
		break;

	case FCON:
		t = getw(stdin);
		printf("fcon %s\n", outname(numbuf));
		sdep++;
		break;

	case FSEL:
		printf("fsel %o ", getw(stdin));
		printf("%d ", getw(stdin));
		printf("%d\n", getw(stdin));
		break;

	case STRASG:
		t = getw(stdin);
		printf("strasg %o ", getw(stdin));
		printf("%d\n", getw(stdin));
		break;

	case NULLOP:
		printf("null\n");
		sdep++;
		break;

	case LABEL:
		printf("label %d\n", getw(stdin));
		break;

	case NLABEL:
		tp = outname(s);
		printf("nlabel %s\n", tp);
		break;

	case RLABEL:
		tp = outname(s);
		printf("rlabel %s\n", tp);
		break;

	case BRANCH:
		printf("branch %d\n", getw(stdin));
		break;

	case SETREG:
		printf("nreg %d\n", getw(stdin));
		break;

	default:
		t = getw(stdin);
		if (op <=0 || op >=120) {
			printf("Unknown op %d\n", op);
			exit(1);
		}
		if (opdope[op]&BINARY)
			sdep--;
		if (sdep<=0)
			printf("Binary expression botch\n");
		if (opntab[op] == 0)
			printf("op %d %o\n", op, t);
		else
			printf("%s %o\n", opntab[op], t);
		break;
	}
	}
}

char *
outname(s)
char *s;
{
	register char *p, c;
	register n;

	p = s;
	n = 0;
	while (c = getc(stdin)) {
		*p++ = c;
		n++;
	}
	do {
		*p++ = 0;
	} while (n++ < 8);
	return(s);
}

error(s)
char *s;
{
	printf("%s\n", s);
	exit(1);
}
