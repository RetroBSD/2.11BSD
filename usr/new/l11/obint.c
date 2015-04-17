/*   Object File Interpreter  */

#define	WORD	unsigned int
char *tack();
WORD getword();

# include	<stdio.h>

main(argc, argv)
int	argc;
char	*argv[];

{
	int 	whicharg;
	char	*filename;
	char	*cat_obj();

	if (argc == 1)
	{
		fprintf(stderr, "Error: no argument specified\n");
		exit(1);
	}

	for (whicharg = 1; whicharg < argc; whicharg++)
	{
		filename = tack(argv[whicharg], ".obj");
		printf("\n\n\nFile Name: %s\n", filename);
		gsd(filename);
		code(filename);
		lst(filename);
	}
}

gsd(fname)
char	*fname;
{
	char	sname[7];	/* symbol name */
	int	attr;		/* byte specifing attributes */
	int	type;		/* byte specifing type of symbol */
	int	value;		/* value of the symbol */
	char	*pstring();	/* function returning pointer to psect attribute string */
	char	*gsstring();	/* function returning global symbol attribute string */

	printf("\n\nProgram Sections, Global Symbols and Definitions:\n\n");
	ch_input(fname, 001);
	while(morebytes())
	{
		dc_symbol(sname);
		attr = getbyte();
		type = getbyte();
		value = getword();
		switch(type)
		{
			case 0:
				printf("program title: %s\n", sname);
				break;
			
			case 1:
				printf("program section: <%s>", sname);
				printf(" %s, size: %06o\n", pstring(attr), value);
				break;

			case 2:
				printf("internal symbol table: %s", sname);
				printf(" %03o %06o\n", attr, value);
				break;

			case 3:
				printf("transfer address in <%s>, ", sname);
				printf("location: %06o\n", value);
				break;

			case 4:
				printf("global symbol: %s", sname);
				printf(" %s, value: %06o\n", gsstring(attr), value);
				break;

			case 5:
				printf("program section: <%s>", sname);
				printf(" %s, size: %06o\n", pstring(attr), value);
				break;

			case 6:
				printf("version identification: %s\n", sname);
				break;

			default:
				fprintf(stderr, "gsd type out of range %03o\n", type);
				exit(0);
		}
	}
}

code(fname)
char 	*fname;
{
	char		sname[7];	/* symbol name buffer */
	char		vrstring[40];	/* string used as an argument to */
					/* a virtual register directive */
	int		drctv;		/* code directive */
	int		i;
	int		temp;		/* possible virtual register directive */
	int		getbyte();	/* returns byte from checksum buffer */
	WORD	getword();	/* returns word from checksum buffer */

	printf("\n\nCode and Directives:\n\n");
	ch_input(fname, 017);
	while (morebytes())
	{
		switch (drctv = getbyte())
		{
			case 000:
				printf("load zero word\n");
				break;

			case 002:
				printf("load zero relative \n");
				break;

			case 004:
				printf("load %06o\n", getword());
				break;

			case 006:
				printf("load %06o - (glc + 2)\n", getword());
				break;

			case 010:
				printf("load rel const of current psect\n");
				break;

			case 014:
				printf("load %06o + (rel const of current psect)\n", getword());
				break;

			case 020:
				dc_symbol(sname);
				regcheck(sname);
				printf("load %s\n", sname);
				break;

			case 022:
				dc_symbol(sname);
				regcheck(sname);
				printf("load %s - (glc + 2)\n", sname);
				break;

			case 030:
				dc_symbol(sname);
				printf("load rel const of <%s>\n", sname);
				break;

			case 032:
				dc_symbol(sname);
				printf("load rel const of <%s> - (glc + 2)\n", sname);
				break;

			case 034:
				dc_symbol(sname);
				printf("load rel const of <%s> + %06o\n", sname, getword());
				break;

			case 036:
				dc_symbol(sname);
				printf("load rel const of <%s> + %06o - (glc + 2)\n", sname, getword());
				break;

			case 040:
				do040();
				break;

			case 044:
				sprintf(vrstring, "%06o", getword());
				vrdirect(vrstring, getbyte());
				break;

			case 050:
				strcpy(vrstring, "current psect rel const");
				if ((temp = getbyte()) == 0)
					printf("reset glc to %s\n", vrstring);
				else
					vrdirect(vrstring, temp);
				break;

			case 054:
				sprintf(vrstring, "(%06o + current psect rel const)", getword());
				if ((temp = getbyte()) == 0)
					printf("reset glc to %s\n", vrstring);
				else
					vrdirect(vrstring, temp);
				break;

			case 060:
				dc_symbol(sname);
				regcheck(sname);
				vrdirect(sname, getbyte());
				break;

			case 070:
				dc_symbol(sname);
				sprintf(vrstring, "rel const of <%s>", sname);
				if ((temp = getbyte()) == 0)
					printf("current psect is <%s>; reset glc to %s\n", sname, vrstring);
				else
					vrdirect(vrstring, temp);
				break;

			case 074:
				dc_symbol(sname);
				sprintf(vrstring, "(rel const of <%s> + %06o)", sname, getword());
				if ((temp = getbyte()) == 0)
					printf("current psect is <%s>; reset glc to %s\n", sname, vrstring);
				else
					vrdirect(vrstring, temp);
				break;

			case 0200:
				printf("load zero byte\n");
				break;

			case 0204:
				printf("load byte %03o\n", getbyte());
				break;

			case 0220:
				dc_symbol(sname);
				regcheck(sname);
				printf("load %s as a byte\n", sname);
				break;

			default:
				fprintf(stderr, "Unrecognizable code command: %03o\n", drctv);
				for (i = 0; i < 8; i++)
					fprintf(stderr, "%03o ", getbyte());
				printf("\n");
				exit(1);
		}
	}
}


lst(fname)	/* local symbol table dump */
char	*fname;
{
	char	sname[7];
	char	*sattr;		/* local symbol attributes */
	int	pnumb;		/* number of psect the symbol is defined in */
	char	*lsstring();	/* returns string of attributes */

	printf("\n\nLocal Symbols:\n\n");
	ch_input(fname, 022);	/* change input to local symbols */
	while (morebytes())
	{
		dc_symbol(sname);
		sattr = lsstring(getbyte());
		pnumb = getbyte();
		printf("%s  %06o  section: %03o  %s\n", sname, getword(), pnumb, sattr);
	}
}


char	*pstring(attr)	/* decodes attr (which specifies psect attributes) */
			/* into a string, returns pointer to the string */
int	attr;
{
	static char	string[80];

	if (attr & 001)
		strcpy(string, "shr ");
	else
		strcpy(string, "prv ");
	if (attr & 002)
		strcat(string, "ins ");
	else if (attr & 004)
		strcat(string, "bss ");
	else
		strcat(string, "dat ");
	if (attr & 020)
		strcat(string, "ovr ");
	else
		strcat(string, "cat ");
	if (attr & 040)
		strcat(string, "rel ");
	else
		strcat(string, "abs ");
	if (attr & 0100)
		strcat(string, "gbl");
	else
		strcat(string, "loc");
	return(string);
}


regcheck(s)	/* checks to see if the string is suppossed to */
		/* represent a register rather than a symbol, */
		/* if so it adds "reg." to the string */
char	*s;
{
	if (*s == ' ')
	{
		strcpy(s, "reg.");
		*(s + 4) = ' ';
		*(s + 5) += 'A' - 'a';
	}
}


vrdirect(s, drctv)		/* print virtual register directive */
char	 	*s;		/* string argument to directive */
WORD drctv;	/* the directive */
{
	char	reg;	/* destination character register */

	reg = 'A' + getbyte() - 1;
	switch (drctv)
	{
		case 0200:
			printf("move %s to reg  %c\n", s, reg);
			break;

		case 0201:
			printf("add %s to reg  %c\n", s, reg);
			break;

		case 0202:
			printf("subtract %s from reg  %c\n", s, reg);
			break;

		case 0203:
			printf("multiply %s into reg  %c\n", s, reg);
			break;

		case 0204:
			printf("divide %s int reg  %c\n", s, reg);
			break;

		case 0205:
			printf("bit \"and\" %s with reg  %c\n", s, reg);
			break;

		case 0206:
			printf("bit \"or\" %s with reg  %c\n", s, reg);
			break;

		default:
			fprintf(stderr, "Unrecognizable v.r. directive: %03o\n", drctv);
			exit(1);
	}
}


do040()		/* 040 is used for misc. directives to virtual registers */

{
	int	drctv;		/* the drctv */
	char	getreg();	/* destination character register */
	char	sname[7];	/* symbol name */

	switch (drctv = getbyte())
	{
		case 001:
			printf("move low address of relocatable code to reg  %c\n", getreg());
			break;

		case 002:
			printf("move high address of relocatable code to reg %c\n", getreg());
			break;

		case 003:
			dc_symbol(sname);
			printf("move low reloc address of %s to reg %c\n", sname, getreg());
			break;

		case 004:
			dc_symbol(sname);
			printf("move high reloc address of %s to reg %c\n", sname, getreg());
			break;

		case 0200:
			printf("clear reg  %c\n", getreg());
			break;

		case 0201:
			printf("do nothing to reg  %c\n", getreg());
			break;

		default:
			fprintf(stderr, "Unrecognizable 040 directive: %03o, reg %c\n", drctv, getreg());
			exit(1);
	}
}


char	getreg()	/* reads a byte to determine register character */

{
	return('A' + getbyte() - 1);
}

char 	*gsstring(attr)	/* decodes attr (which specifies global symbol */
			/* attributes) into a string */
int	attr;
{
	static char	string[50];

	if (!(attr & 010))
		strcpy(string, "und ");
	else
	{
		strcpy(string, "def ");
		if (attr & 040)
			strcat(string, "rel ");
		else
			strcat(string, "abs ");
	}
	return (string);
}


char 	*lsstring(attr)	/* decodes attr (which specifies local symbol */
			/* attributes) into a string */
int	attr;
{
	static char	string[50];

	if (!(attr & 010))
		strcpy(string, "und ");
	else
	{
		strcpy(string, "def ");
		if (attr & 040)
			strcat(string, "rel ");
		else
			strcat(string, "abs ");
		if (attr & 0100)
			strcat(string, "gbl ");
		if (attr & 001)
			strcat(string, "reg ");
		if (attr & 002)
			strcat(string, "lbl ");
		if (attr & 0200)
			strcat(string, "hkl ");
	}
	return (string);
}


/***********************  bail_out  ****************************************/


bail_out()
{
	exit();
}
