#include	"link.h"
/* linker support functions */



char *strcpy(), *strcat();
/************************  lerror  ****************************************/


lerror(mess)	/* linker program error, print message and exit */
register char	*mess;
{
	fprintf(stderr, "linker program error: %s\n", mess);
	bail_out();
}


/**************************  dc_symbol  **************************************/


dc_symbol(s)	/* decode rad50 symbol in input stream and place in */
		/* the buffer s. */
register char	*s;

{
	WORD getword();
	
	derad50(getword(), s);
	derad50(getword(), s + 3);
	*(s + 6) = '\0';
}


/******************************  derad50  ************************************/


derad50(x,s)		/* decode a word in which 3 characters are coded by */
			/* the RAD50 scheme. */
register unsigned short	x;
register char 	*s;

{
	s[2] = returnchar(x % 40);
	x /= 40; 
	s[1] = returnchar(x % 40);
	x /= 40;
	s[0] = returnchar(x % 40); 
}


/******************************  returnchar  *******************************/


returnchar(k)   	/* return a character according to RAD50 coding */
			/* scheme, called by derad50 */
register unsigned short	k;
{
	if (k >= 1 && k <= 26)
		/* k represents a letter */
		return('a' + k - 1);

	else if (k >= 30 && k <= 39)
		/* k represents a digit */
		return('0' + k - 30);

	else switch (k)
	{
		case 0:
			return(' ');
		
		case 27:
			return('$');

		case 28:
			return('.');

		case 29:
			return ('_');

		default:
			lerror("RAD50 non-character");
			/* NOTREACHED */
	}

}


/*****************************  lalloc  **************************************/


char	*lalloc(amount)		/* storage allocator, calls calloc and if */
				/* null is returned calls error */
register int	amount;		/* number of bytes of storage needed */
{
	char *calloc();
	register char	*temp;

	if ((temp = calloc(1, (unsigned)amount)) == NULL)
	{
		fprintf(stderr, "Error: core size exceeded.\n");
		bail_out();
		/* NOTREACHED */
	}
	else
		return (temp);
}


/**********************************  tack  ***********************************/


char	*tack(s, t)	/* catenate s with t if s does not already end with t */

register char	*s;
register char	*t;
{
	register char	*new;
	int 		s_len;
	int		t_len;

	s_len = strlen(s);
	t_len = strlen(t);
	if (s_len < t_len || strcmp(s + s_len - t_len, t))
	{
		new = lalloc(s_len + t_len + 1);
		strcpy(new, s);
		strcat(new, t);
		return(new);
	}
	else
		return (s);
}


/******************************  strip  ************************************/


strip(s, t)	/* strip t off the end of s if it is there */

register char	*s;
register char	*t;
{
	register int	s_len;
	int		t_len;

	s_len = strlen(s);
	t_len = strlen(t);
	if (s_len >= t_len && !strcmp(s + s_len - t_len, t))
		*(s + s_len - t_len) = '\0';
}
