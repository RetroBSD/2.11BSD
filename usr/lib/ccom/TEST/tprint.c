/*
 * tree printer routine for C
 */

#include "c1.h"

char *strop[] {
"0",
";",
"{",
"}",
"[",
"]",
"(",
")",
":",
",",
"10",
"11",
"12",
"13",
"14",
"15",
"16",
"17",
"18",
"19",
"",
"",
"string",
"fcon",
"sfcon",
"25",
"26",
"",
"",
"sizeof",
"++pre",
"--pre",
"++post",
"--post",
"!un",
"&un",
"*un",
"-un",
"~un",
".",
"+",
"-",
"*",
"/",
"%",
">>",
"<<",
"&",
"|",
"^",
"->",
"itof",
"ftoi",
"&&",
"||",
"&~",
"ftol",
"ltof",
"itol",
"ltoi",
"==",
"!=",
"<=",
"<",
">=",
">",
"<p",
"<=p",
">p",
">=p",
"=+",
"=-",
"=*",
"=/",
"=%",
"=>>",
"=<<",
"=&",
"=|",
"=^",
"=",
"&(test)",
"82",
"83",
"84",
"=&~",
"86",
"87",
"88",
"89",
"?",
"91",
"92",
"93",
"94",
"95",
"96",
"97",
"98",
"99",
"call",
"mcall",
"jump",
"cbranch",
"init",
"setreg",
"106",
"107",
"108",
"109",
"forcereg",
};

treeprint(tp)
struct tnode *tp;
{
	register f;
	extern fout;
	static tout;

	if (tout==0)
		tout = dup(1);
	flush();
	f = fout;
	fout = tout;
	printf("\n");
	tprt(tp, 0);
	flush();
	fout = f;
}

tprt(at, al)
struct tnode *at;
{
	register struct tnode *t;
	register i, l;

	t = at;
	l = al;
	if (i=l)
		do
			printf(". ");
		while (--i);
	if (t<treebase || t>=spacemax) {
		printf("%o: bad tree ptr\n", t);
		return;
	}
	if (t->op<0 || t->op>RFORCE) {
		printf("%d\n", t->op);
		return;
	}
	printf("%s", strop[t->op]);
	switch (t->op) {

	case SETREG:
		printf("%d\n", t->type);
		return;

	case PLUS:
	case MINUS:
	case TIMES:
	case DIVIDE:
	case MOD:
	case LSHIFT:
	case RSHIFT:
	case AND:
	case OR:
	case EXOR:
	case NAND:
	case LOGAND:
	case LOGOR:
	case EQUAL:
	case NEQUAL:
	case LESSEQ:
	case LESS:
	case GREATEQ:
	case GREAT:
	case LESSEQP:
	case LESSP:
	case GREATQP:
	case GREATP:
	case ASPLUS:
	case ASMINUS:
	case ASTIMES:
	case ASDIV:
	case ASMOD:
	case ASRSH:
	case ASLSH:
	case ASSAND:
	case ASSNAND:
	case ASOR:
	case ASXOR:
	case ASSIGN:
	case QUEST:
	case CALL:
	case MCALL:
	case CALL1:
	case CALL2:
	case TAND:
		prtype(t);

	case COLON:
	case COMMA:
		printf("\n");
		tprt(t->tr1, l+1);
		tprt(t->tr2, l+1);
		return;

	case INCBEF:
	case INCAFT:
	case DECBEF:
	case DECAFT:
		printf(" (%d)", t->tr2);

	case EXCLA:
	case AMPER:
	case STAR:
	case NEG:
	case COMPL:
	case INIT:
	case JUMP:
	case LOAD:
	case RFORCE:
	case ITOF:
	case FTOI:
	case FTOL:
	case LTOF:
	case LTOI:
	case ITOL:
		prtype(t);
		printf("\n");
		tprt(t->tr1, l+1);
		return;

	case NAME:
	case CON:
	case SFCON:
	case FCON:
	case AUTOI:
	case AUTOD:
		pname(t, 0);
		prtype(t);
		printf("\n");
		return;

	case CBRANCH:
		printf(" (L%d)\n", t->lbl);
		tprt(t->btree, l+1);
		return;

	default:
		printf(" unknown\n");
		return;
	}
}

char *typetab[] {
	"int",
	"char",
	"float",
	"double",
	"struct",
	"(?5)",
	"long",
	"(?7)",
};

char	*modtab[] {
	0,
	"*",
	"()",
	"[]",
};

prtype(atp)
struct tnode *atp;
{
	register struct tnode *tp;
	register t;

	tp = atp;
	printf(" %s", typetab[tp->type&07]);
	t = (tp->type>>3) & 017777;
	while (t&03) {
		printf(modtab[t&03]);
		t =>> 2;
	}
	printf(" (%d)", tp->degree);
}
