# include	<stdio.h>
#ifdef vax
#define WORD	unsigned short
#endif vax
#ifdef pdp11
#define WORD	unsigned 
#endif pdp11

/********************* structure declarations *************************/

struct psect		/* program section structure
			** Since program sections have to be referenced in
			** different ways,  there are multiple link-lists
			** running through each structure.
			** Program sections are referenced 1) by ins-dat-bss
			** type to facilitate the computation of relocation
			** constants (*next), 2) by object file so that relocation
			** constants may be found when interpreting the code
			** section within a object file (*obsame).
			** In addition global program sections of the same 
			** name and type must be grouped together (*pssame).
			** Also, a list of global relocatable symbols defined
			** in the psect in used to relocate these 
			** symbols (*slist). */
{
	char		name[7];
	char		type;		/* attribute byte from M11 */
	int		nbytes;		/* number of bytes in psect */
	WORD		rc;		/* relocation constant */
	struct psect	*next;		/* next psect of same ins-dat-bss type */
	struct psect	*pssame;	/* same psect from different object file */
	struct psect	*obsame;	/* next psect in same object file */
	struct symbol	*slist;		/* list of symbols defined in this psect */
};


struct symbol			/* global symbol structure, the symbol
				** table is a binary tree of these structures .
				** There are also link-lists from each psect
				** running through the symbol table to
				** facilitate  the relocation of symbols. */
{
	char		name[7];
	char		type;		/* attribute byte from M11 */
	int		value;
	int		t_numb;		/* number of the symbol in the out
					** file symbol table */
	struct symbol	*symlist;	/* continuation of list of symbols
					** defined in this symbol's psect */
	struct psect	*prsect;	/* program section of the symbol */
	struct symbol	*left;		/* left child */
	struct symbol	*right;		/* right child */
};


struct objfile		/* object file structure */
{
	char		*fname;
	struct objfile  *nextfile;	/* next file in link-list of files */
	struct psect	*psect_list;	/* root to link-list of psects 
					** in this object file */
	char		pname[7];	/* program name */
	char		*ver_id;	/* version identification */
};


struct outword		/* structure for linking a word to be written in
			** the out file with its relocation bits */
{
	WORD	val;
	WORD	rbits;
};


struct g_sect		/* structure for global program section tree */
{
	char		name[7];	/* psect name */
	char		type;		/* M11 coded type */
	struct psect	*last_sect;	/* pointer to the last psect in the 
					** link-list of global same psects */
	struct g_sect	*leftt;		/* left child */
	struct g_sect	*rightt;	/* right child */
};


/***********************  macros  ****************************************/


	/* macros for requesting input from the different parts of the
	** object files. */

# define	HEADER		001
# define	CODE		017
# define	SYMBOLS 	022

	/* bit flags for attributes of psects and symbols */

# define	SHR		001
# define	INS		002
# define	BSS		004
# define	DEF		010
# define	OVR		020
# define	REL		040
# define	GBL		0100

# define	isabs(x)	(((x)->type & REL) == 0)
# define	isrel(x)	(((x)->type & REL) != 0)
# define	islcl(x)	(((x)->type & GBL) == 0)
# define	isgbl(x)	(((x)->type & GBL) != 0)
# define	isprv(x)	(((x)->type & SHR) == 0)
# define	isshr(x)	(((x)->type & SHR) != 0)
# define	isins(x)	(((x)->type & INS) != 0)
# define	isbss(x)	(((x)->type & BSS) != 0)
# define	isovr(x)	(((x)->type & OVR) != 0)
# define	iscon(x)	(((x)->type & OVR) == 0)
# define	isdef(x)	(((x)->type & DEF) != 0)
# define	isudf(x)	(((x)->type & DEF) == 0)
