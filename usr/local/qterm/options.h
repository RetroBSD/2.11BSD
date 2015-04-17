/*
 * Copyright (c) 1990 Michael A. Cooper.
 * This software may be freely distributed provided it is not sold for 
 * profit and the author is credited appropriately.
 */

/*
 * $Header: RCS/options.h,v 1.7.1 96/3/23 18:13:30 sms Exp $
 *
 * $Log:	options.h,v $
 * Revision 1.7  90/12/15  18:13:30  mcooper
 * Add copywrite notice.
 * 
 * Revision 1.6  90/11/13  15:28:39  mcooper
 * Add OptBool cvtarg routine.
 * 
 * Revision 1.5  90/10/29  19:34:03  mcooper
 * Fixed comment for NoArg.
 * 
 * Revision 1.4  90/10/29  18:48:43  mcooper
 * Cleanup some comments.
 * 
 * Revision 1.3  90/10/29  14:47:29  mcooper
 * UsageString is now a real function.
 * 
 * Revision 1.2  90/10/26  15:55:44  mcooper
 * Add defines for "__" and ArgHidden.
 * 
 * Revision 1.1  90/10/26  14:42:53  mcooper
 * Initial revision
 * 
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/errno.h>

#define Num_Opts(o)	(sizeof(o)/sizeof(OptionDescRec))
#define HELPSTR		"-help"
#define __		(caddr_t)

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

/*
 * Values for OptionDescRec.flags.
 */
#define NoArg		0x001	/* No argument for this option.  Use
				   OptionDescRec.value. */
#define IsArg		0x002	/* Value is the option string itself */
#define SepArg		0x004	/* Value is in next argument in argv */
#define StickyArg	0x008	/* Value is characters immediately following 
				   option */
#define SkipArg		0x010	/* Ignore this option and the next argument in 
				   argv */
#define SkipLine	0x020	/* Ignore this option and the rest of argv */
#define SkipNArgs	0x040	/* Ignore this option and the next 
				   OptionDescRes.value arguments in argv */
#define ArgHidden	0x080	/* Don't show in usage or help messages */

/*
 * Option description record.
 */
typedef struct {
    char	*option;		/* Option string in argv	    */
    int		 flags;			/* Flag bits			    */
    int		(*cvtarg)();		/* Function to convert argument     */
    caddr_t	 valp;			/* Variable to set		    */
    caddr_t	 value;			/* Default value to provide	    */
    char	*usage;			/* Usage message		    */
    char	*desc;			/* Description message		    */
} OptionDescRec, *OptionDescList;

void UsageOptions();
void HelpOptions();
void UserError();
int ParseOptions();
OptionDescRec *FindOption();

int OptBool();
int OptInt();
int OptLong();
int OptShort();
int OptStr();

extern char *OptionChars;
extern int errno;
extern char *sys_errlist[];
extern long strtol();
extern char *malloc();
extern char *strcpy();
