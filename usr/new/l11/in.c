/* Input functions */

# include 	"link.h"

	/* define maximum size of checksum contents */
# define	MAXSIZE		40

static char	*Fname = NULL;	/* name of current input file */
static FILE	*Fp = NULL;	/* file pointer of current file */
static char	Buff[MAXSIZE];	/* buffer for current checksum module */
static char	*Next;		/* next byte to be popped from buffer */
static int	Count;		/* number of bytes left */
static int	Type;		/* type of checksum module */
static char	No_code = 0;	/* flag set if a code section was attempted to
				** be found but was not there */


/***************************  ch_input  ************************************/


ch_input(newfile, newmod)	/* change input checksum buffer contents */
char	*newfile;
int	newmod;

{
	FILE	*fopen();

	if (Fname == NULL || strcmp(Fname, newfile))	/* new file is 
							** different */
	{
		Fname = newfile;
		if (Fp != NULL)
			fclose(Fp);
		if ((Fp = fopen(Fname, "r")) == NULL)
			inerror("not found");
		Type = 0;
	}
	if (newmod != Type)			/* if not right module type already */
		while (newmod != read_mod())	/* read until correct module type */
		{
			/* check for missing code section */
			if ( newmod == CODE && Type == SYMBOLS)
			{
				No_code = 1;
				break;
			}
			if (Type == 6)		/* check for EOF module */
			{
				No_code = 1;
				break;
			}
				/*
				inerror("EOF \(linker error\)");
				*/
		}
}


/**************************  morebytes  ************************************/


morebytes()	/* returns 1 if there are unread bytes of the current */
		/* checksum module type, returns 0 if not */
{
	register int	temptype;

	if (No_code)	/* if no code section, return 0 and reset */
	{
		No_code = 0;
		return (0);
	}
	else if (Count > 0)
		return (1);
	else
	{
			/* read next module and check for same type */
		temptype = Type;
		if (temptype == read_mod())
			return (1);
		else
			return (0);
	}
}


/******************************  getbyte  ************************************/


getbyte()	/* return next byte of current checksum module type */

{
		/* check for empty buffer, if so check if next module is */
		/* the same type */
	if ((Count == 0) && !morebytes())
	{
		lerror("End of checksum module");
		/* NOTREACHED */
	}
	else
	{
		Count--;
		return (*Next++ & 0377);
	}
}


/****************************  getword  ************************************/


WORD getword()	/* return next word */
{
	register int	temp;

	temp = 0377 & getbyte();
	return (0400 * getbyte() + temp);
}


/****************************  inerror  **********************************/


inerror(mess)	/* print error message and filename then exit. */
		/* called when a user error has occurred concerning the */
		/* input file */
char 	*mess;
{
	fprintf(stderr, "%s: %s\n", Fname, mess);
	bail_out();
}


/****************************************************************************/


int	sum;	/* sum of input bytes */


/***************************  read_mod  **************************************/


read_mod()	/* read a checksum module and return type */

{
	register int	i;
	sum = 0;
	if (getb() != 1)
	{
		if (feof(Fp) && No_code)
			return ( 6 );
		else
			inerror("Not in object file format");
	}
			/* clear zero byte */
	getb();
			/* Count = next word - 6 (# of bytes in header) */
	Count = getb();
	Count += 0400 * getb() - 6;
	if (Count > MAXSIZE)
		lerror("checksum size too large");
	Type = getb();
			/* clear zero byte */
	getb();
			/* read checksum contents into buffer */
	for (i = 0; i < Count; i++)
		Buff[i] = getb();
			/* read checksum */
	getb();
	if (sum % 0400 != 0)
		inerror("Checksum error, reassemble");
			/* clear zero trailer byte if Count even */
	if (Count % 2 == 0)
		getb();
			/* set pointer to next character to be read */
	Next = Buff;
	return (Type);
}


/*************************  getb  ***************************************/


getb()		/* get a byte from input file, add to "sum" */
		/* check for EOF, return the byte */
{
	register int	k;
	
	sum += (k = getc(Fp));

	if (k == EOF)
		No_code = 1;
	return (0377 & k);
}
