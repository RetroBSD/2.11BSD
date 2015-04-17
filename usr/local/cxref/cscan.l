/*
** cscan.l
**
** Does the major work of removing identifiers and constants
** from the input stream, for Cxref. Its output is then extensively
** postprocessed.
**
** Arnold Robbins, Information and Computer Science, Georgia Tech
**	gatech!arnold
** Copyright (c) 1984 by Arnold Robbins.
** All rights reserved.
** This program may not be sold, but may be distributed
** provided this header is included.
*/

%{
extern int line_no;
extern char *fname, *basename();
%}

letter		[A-Za-z_]
digit		[0-9]

%%
int		|
char		|
float		|
double		|
struct		|
union		|
long		|
short		|
unsigned	|
auto		|
extern		|
register	|
typedef		|
static		|
goto		|
return		|
sizeof		|
break		|
continue       	|
if		|
else		|
for		|
do		|
while		|
switch		|
case		|
default		|
entry		|
enum		|
void		|
define		|
undef		|
include		|
ifdef		|
ifndef		|
defined		|
endif		;	/* ignore C and cpp keywords */

"<".+">"	;	/* forget about include-file names */

"\n"		line_no++;

"/*"		{	/* get rid of comments */
			register char c, c1;

		  loop: while((c = input()) != '*' && c != 0)
				if(c == '\n')
					line_no++;

			if(c == 0)
			{
				fprintf(stderr,
				"unexpected EOF in comment at line %d, file %s\n",
					line_no, basename(fname));
				exit(1);
			}

			if((c1 = input()) != '/')
			{
				unput(c1);	/* could be '*' before '/' */
				goto loop;
			}
		}

{letter}({letter}|{digit})*	outid();  /* what we actually want */

'[^\\']'		|
'\\{digit}{1,3}'	|
'\\[\\bfrnlt']'			outchar();

\"			{	/* collect quoted strings */
				register char c;
				register int i;

				for(i = 1, c = input(); ; i++, c = input())
					switch (c) {
					case '"':
						yytext[i] = c;
						yytext[++i] = '\0';
						yyleng = i - 1;
						goto fini;
					
					case '\\':
						yytext[i] = '\\';
						yytext[++i] = input();
						if (yytext[i] == '\n')
						{
							line_no++;
							yytext[i] = 'N';
							/* make visible */
						}
						break;

					case 0:
						fprintf(stderr,
					"unexpected EOF inside string at line %d, file %s\n",
							line_no, basename(fname));
						exit(1);
						break;

					default:
						yytext[i] = c;
						break;
					}

				fini:
					outstring();
		}

[+-]?{digit}+[lL]?			|
[+-]?0[Xx]({digit}|[a-fA-F])+[lL]?		outint();

[+-]?{digit}*"."{digit}+([Ee][+-]?{digit}+)?	|
[+-]?{digit}+"."{digit}*([Ee][+-]?{digit}+)?	|
[+-]?{digit}+[Ee][+-]?{digit}+				outfloat();

.			;	/* delete everything else */

%%

yywrap()	/* wrap up for lex, return 1 */
{
	return(1);
}

#include "constdefs.h"

extern char *fname;
extern char *basename();

outchar()
{
	outtext(CHAR);
}

outstring()
{
	outtext(STRING);
}

outint()
{
	int i = strlen(yytext);

	/* handle long integer constants */

	if (yytext[i-1] == 'l' || yytext[i-1] == 'L')
		yytext[i-1] = '\0';

	outtext(INT);
}

outfloat()
{
	outtext(FLOAT);
}

outtext(type)
char type;
{
	printf("~%c%s\t%s\t%d\n", type, yytext, basename(fname), line_no);
}
