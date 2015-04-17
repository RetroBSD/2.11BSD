#if	!defined(lint) && defined(DOSCCS)
static	char	sccsid[] = "@(#)opset.c 2.7 (2.11BSD) 2000/2/28";
#endif

#include "defs.h"

	char	*errflg;
	long	dot;
	int	dotinc;
	long	var[];
	MAP     datmap;

#undef  SINGLE

/* instruction printing */

#define DOUBLE  0
#define DOUBLW  1
#define SINGLE  2
#define SINGLW  3
#define REVERS  4
#define BRANCH  5
#define NOADDR  6
#define DFAULT  7
#define TRAP    8
#define SYS     9
#define SOB     10
#define JMP     11
#define JSR     12

struct optab {
	int     mask;
	int     val;
	int     itype;
	char    *iname;
} optab[] = {
	0107777, 0010000, DOUBLE, "mov",
	0107777, 0020000, DOUBLE, "cmp",
	0107777, 0030000, DOUBLE, "bit",
	0107777, 0040000, DOUBLE, "bic",
	0107777, 0050000, DOUBLE, "bis",
	0007777, 0060000, DOUBLW, "add",
	0007777, 0160000, DOUBLW, "sub",
	0100077, 0005000, SINGLE, "clr",
	0100077, 0005100, SINGLE, "com",
	0100077, 0005200, SINGLE, "inc",
	0100077, 0005300, SINGLE, "dec",
	0100077, 0005400, SINGLE, "neg",
	0100077, 0005500, SINGLE, "adc",
	0100077, 0005600, SINGLE, "sbc",
	0100077, 0005700, SINGLE, "tst",
	0100077, 0006000, SINGLE, "ror",
	0100077, 0006100, SINGLE, "rol",
	0100077, 0006200, SINGLE, "asr",
	0100077, 0006300, SINGLE, "asl",
	0000077, 0000100, JMP,    "jmp",
	0000077, 0000300, SINGLE, "swab",
	0000077, 0170100, SINGLW, "ldfps",
	0000077, 0170200, SINGLW, "stfps",
	0000077, 0170300, SINGLW, "stst",
	0000077, 0170400, SINGLW, "clrf",
	0000077, 0170500, SINGLW, "tstf",
	0000077, 0170600, SINGLW, "absf",
	0000077, 0170700, SINGLW, "negf",
	0000077, 0006700, SINGLW, "sxt",
	0000077, 0006600, SINGLW, "mtpi",
	0000077, 0106600, SINGLW, "mtpd",
	0000077, 0006500, SINGLW, "mfpi",
	0000077, 0106500, SINGLW, "mfpd",
	0000077, 0106700, SINGLW, "mfps",
	0000077, 0106400, SINGLW, "mtps",
	0000777, 0070000, REVERS, "mul",
	0000777, 0071000, REVERS, "div",
	0000777, 0072000, REVERS, "ash",
	0000777, 0073000, REVERS, "ashc",
	LOBYTE,  0000400, BRANCH, "br",
	LOBYTE,  0001000, BRANCH, "bne",
	LOBYTE,  0001400, BRANCH, "beq",
	LOBYTE,  0002000, BRANCH, "bge",
	LOBYTE,  0002400, BRANCH, "blt",
	LOBYTE,  0003000, BRANCH, "bgt",
	LOBYTE,  0003400, BRANCH, "ble",
	LOBYTE,  0100000, BRANCH, "bpl",
	LOBYTE,  0100400, BRANCH, "bmi",
	LOBYTE,  0101000, BRANCH, "bhi",
	LOBYTE,  0101400, BRANCH, "blos",
	LOBYTE,  0102000, BRANCH, "bvc",
	LOBYTE,  0102400, BRANCH, "bvs",
	LOBYTE,  0103000, BRANCH, "bcc",
	LOBYTE,  0103400, BRANCH, "bcs",
	0000000, 0000000, NOADDR, "halt",
	0000000, 0000001, NOADDR, "wait",
	0000000, 0000002, NOADDR, "rti",
	0000000, 0000003, NOADDR, "bpt",
	0000000, 0000004, NOADDR, "iot",
	0000000, 0000005, NOADDR, "reset",
	0000000, 0000006, NOADDR, "rtt",
	LOBYTE,  0171000, REVERS, "mulf",
	LOBYTE,  0171400, REVERS, "modf",
	LOBYTE,  0172000, REVERS, "addf",
	LOBYTE,  0172400, REVERS, "movf",
	LOBYTE,  0173000, REVERS, "subf",
	LOBYTE,  0173400, REVERS, "cmpf",
	LOBYTE,  0174000, DOUBLW, "movf",
	LOBYTE,  0174400, REVERS, "divf",
	LOBYTE,  0175000, DOUBLW, "movei",
	LOBYTE,  0175400, DOUBLW, "movfi",
	LOBYTE,  0176000, DOUBLW, "movfo",
	LOBYTE,  0176400, REVERS, "movie",
	LOBYTE,  0177000, REVERS, "movif",
	LOBYTE,  0177400, REVERS, "movof",
	0000000, 0170000, NOADDR, "cfcc",
	0000000, 0170001, NOADDR, "setf",
	0000000, 0170002, NOADDR, "seti",
	0000000, 0170011, NOADDR, "setd",
	0000000, 0170012, NOADDR, "setl",
	0000000, 0000007, NOADDR, "mfpt",
	0000077, 0007000, JMP, "csm",
	0000077, 0007300, SINGLW, "wrtlck",
	0000077, 0007200, SINGLW, "tstset",
	0000777, 0004000, JSR,    "jsr",
	0000777, 0074000, DOUBLE, "xor",
	0000007, 0000200, SINGLE, "rts",
	0000017, 0000240, DFAULT, "cflg",
	0000017, 0000260, DFAULT, "sflg",
	LOBYTE,  0104000, TRAP,   "emt",
	LOBYTE,  0104400, SYS,    "sys",
	0000077, 0006400, TRAP,   "mark",
	0000777, 0077000, SOB,    "sob",
	0000007, 0000230, DFAULT,   "spl",
	0177777, 0000000, DFAULT, "<illegal op>",
};

char *systab[] = {
	"indir",		/* 0 - indir*/
	"exit",
	"fork",
	"read",
	"write",
	"open",
	"close",
	"wait4",
	NULL,			/* 8 - unused */
	"link",
	"unlink",		/* 10 */
	"execv",
	"chdir",
	"fchdir",
	"mknod",
	"chmod",
	"chown",
	"chflags",
	"fchflags",
	"lseek",
	"getpid",		/* 20 */
	"mount",
	"umount",
	"__sysctl",
	"getuid",
	"geteuid",		/* 25 */
	"ptrace",
	"getppid",
	NULL,			/* 28 - unused */
	NULL,			/* 29 - unused */
	NULL,			/* 30 - unused */
	"sigaction",		/* 31 - sigaction */
	"sigprocmask",		/* 32 - sigprocmask */
	"access",
	"sigpending",		/* 34 - sigpending */
	"sigaltstack",		/* 35 - sigaltstack */
	"sync",
	"kill",
	"stat",
	"_getlogin",		/* 39 - _getlogin */
	"lstat",
	"dup",
	"pipe",
	"setlogin",		/* 43 - unused */
	"profil",
	"setuid",		/* 45 - setuid */
	"seteuid",		/* 46 - seteuid */
	"getgid",
	"getegid",
	"setgid",		/* 49 - setgid */
	"setegid",		/* 50 - setegid */
	"acct",
	"phys",
	"lock",
	"ioctl",
	"reboot",
	NULL,			/* 56 - unused */
	"symlink",
	"readlink",
	"execve",
	"umask",
	"chroot",
	"fstat",
	NULL,			/* 63 - unused */
	NULL,			/* 64 - unused */
	"pselect",		/* 65 - pselect */
	"vfork",
	NULL,			/* 67 - unused */
	NULL,			/* 68 - unused */
	"sbrk",
	NULL,			/* 70 - unused */
	NULL,			/* 71 - unused */
	NULL,			/* 72 - unused */
	NULL,			/* 73 - unused */
	NULL,			/* 74 - unused */
	NULL,			/* 75 - unused */
	"vhangup",
	NULL,			/* 77 - unused */
	NULL,			/* 78 - unused */
	"getgroups",
	"setgroups",
	"getpgrp",
	"setpgrp",
	"setitimer",
	NULL,			/* 84 - unused */
	NULL,			/* 85 - unused */
	"getitimer",
	NULL,			/* 87 - unused */
	NULL,			/* 88 - unused */
	"getdtablesize",
	"dup2",
	NULL,			/* 91 - unused */
	"fcntl",
	"select",
	NULL,			/* 94 - unused */
	"fsync",
	"setpriority",
	"socket",
	"connect",
	"accept",
	"getpriority",
	"send",
	"recv",			/* 102 - recv */
	"sigreturn",
	"bind",
	"setsockopt",
	"listen",
	"sigsuspend",		/* 107 - sigsuspend */
	NULL,			/* 108 - unused */
	NULL,			/* 109 - unused */
	NULL,			/* 110 - unused */
	NULL,			/* 111 - unused */
	"old sigstack",		/* 112 - sigstack COMPAT-43 for zork */
	"recvmsg",
	"sendmsg",
	NULL,			/* 115 - unused */
	"gettimeofday",
	"getrusage",
	"getsockopt",
	NULL,			/* 119 - unused */
	"readv",
	"writev",
	"settimeofday",
	"fchown",
	"fchmod",
	"recvfrom",
	NULL,			/* 126 - unused */
	NULL,			/* 127 - unused */
	"rename",
	"truncate",
	"ftruncate",
	"flock",
	NULL,			/* 132 - unused */
	"sendto",
	"shutdown",
	"socketpair",
	"mkdir",
	"rmdir",
	"utimes",
	NULL,			/* 139 - unused */
	"adjtime",
	"getpeername",
	NULL,			/* 142 - unused */
	NULL,			/* 143 - unused */
	"getrlimit",
	"setrlimit",
	"killpg",
	NULL,			/* 147 - unused */
	"setquota",
	"quota",
	"getsockname",
	/*
	 * 2.11BSD special calls
	 */
	NULL,			/* 151 - unused */
	"nostk",
	"fetchi",
	"ucall",
	"fperr",
};

#define	NUMSYSCALLS	(sizeof (systab) / sizeof (char *))

char	*regname[] = { "r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc"};

	u_int	type, space, incp;

printins(idsp,ins)
	register int ins;
{
	int	byte;
	register struct optab *p;

	type=DSYM; space=idsp; incp=2;
	FOR p=optab;; p++
	DO      IF (ins & ~p->mask) == p->val
		THEN    break;
		FI
	OD
	printf(p->iname);
	byte=ins&0100000;
	ins &= p->mask;

	switch (p->itype) {

	    case JMP:
		type=ISYM;

	    case SINGLE:
		IF byte THEN printc('b'); FI
	    case SINGLW:
		paddr("%8t",ins);
		break;

	    case REVERS:
		doubl(ins&077,(ins>>6)&07);
		break;

	    case JSR:
		type=ISYM;

	    case DOUBLE:
		IF byte THEN printc('b'); FI
	    case DOUBLW:
		doubl(ins>>6,ins);

	    case NOADDR:
		break;

	    case SOB:
		paddr("%8t",(ins>>6)&07);
		branch(",",-(ins&077));
		break;

	    case BRANCH:
		branch("%8t",ins);
		break;

	    case SYS:
		if (ins < NUMSYSCALLS && systab[ins])
			printf("%8t%s", systab[ins]);
		else
			printf("%8t%d", ins);
		break;

	    case TRAP:
	    case DFAULT:
	    default:
		printf("%8t%o", ins);
	}
	dotinc=incp;
}

doubl(a,b)
{
	paddr("%8t",a); paddr(",",b);
}

branch(s,ins)
	char	*s;
	register int ins;
{
	printf(s);
	IF ins&0200 THEN ins |= 0177400; FI
	ins = shorten(dot) + (ins<<1) + 2;
	psymoff(leng(ins),ISYM,"");
}

paddr(s, a)
	char	*s;
	register int a;
{
	register int r;

	var[2]=var[1];
	r = a&07; a &= 070;

	printf(s);
	IF r==7 ANDF a&020
	THEN IF a&010 THEN printc('*'); FI
	     IF a&040
	     THEN IF space==NSP
		  THEN printc('?');
		  ELSE  var[1]=chkget(inkdot(incp),space) + shorten(inkdot(incp+2));
		       psymoff(var[1],(a&010?DSYM:type),"");
		  FI
	     ELSE printc('$');
		  IF space==NSP
		  THEN printc('?');
		  ELSE var[1]=chkget(inkdot(incp), space);
			psymoff(var[1], (a&010?type:NSYM), "");
		  FI
	     FI
	     incp += 2;
	     return;
	FI
	r = (int)regname[r];
	switch (a) {
	    /* r */
	    case 000:
		printf(r);
		return;

	    /* (r) */
	    case 010:
		printf("(%s)", r);
		return;

	    /* *(r)+ */
	    case 030:
		printc('*');

	    /* (r)+ */
	    case 020:
		printf("(%s)+", r);
		return;

	    /* *-(r) */
	    case 050:
		printc('*');

	    /* -(r) */
	    case 040:
		printf("-(%s)", r);
		return;

	    /* *x(r) */
	    case 070:
		printc('*');

	    /* x(r) */
	    case 060:
		IF space==NSP
		THEN printc('?');
		ELSE    var[1]=chkget(inkdot(incp), space);
			psymoff(var[1], (a==070?type:NSYM), "");
		FI
		incp += 2;
		printf("(%s)", r);
		return;
	}
}
