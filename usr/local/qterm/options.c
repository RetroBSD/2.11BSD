/*
 * Copyright (c) 1990 Michael A. Cooper.
 * This software may be freely distributed provided it is not sold for 
 * profit and the author is credited appropriately.
 */

#if	!defined(lint) && defined(DOSCCS)
static char *RCSid = "$Header: options.c,v 1.13.1 96/3/23 18:13:28 sms Exp $";
#endif

/*
 * $Log:	options.c,v $
 * Revision 1.13  90/12/15  18:13:28  mcooper
 * Add copywrite notice.
 * 
 * Revision 1.12  90/12/15  17:51:46  mcooper
 * Add #ifdef HAS_VARARGS around include for <varargs>.
 * 
 * Revision 1.11  90/11/13  16:39:28  mcooper
 * Add #ifdef HAS_VARARGS for systems without
 * varargs.
 * 
 * Revision 1.10  90/11/13  15:28:01  mcooper
 * - Add OptBool cvtarg routine.
 * - Print default values in HelpOptions() 
 *   when appropriate.
 * 
 * Revision 1.9  90/11/13  15:19:00  mcooper
 * Added supported for options being both
 * SepArg and StickyArg.
 * 
 * Revision 1.8  90/10/30  21:02:31  mcooper
 * Need to exit() if -help is specified.
 * 
 * Revision 1.7  90/10/30  20:24:33  mcooper
 * Fixed bug in UsageString().
 * 
 * Revision 1.6  90/10/30  19:53:05  mcooper
 * Cleaned up some NeXT cc and lint stuff.
 * 
 * Revision 1.5  90/10/30  19:45:31  mcooper
 * Remove unneeded paramter to HelpOptions().
 * 
 * Revision 1.4  90/10/29  14:47:42  mcooper
 * Added real function UsageString() to
 * handle formating usage option strings.
 * 
 * Revision 1.3  90/10/29  14:17:00  mcooper
 * Allow options to be abbreviated 
 * (for all non StickArg options).
 * 
 * Revision 1.2  90/10/26  15:56:11  mcooper
 * - Fix bug in SepArg code that ate arguments.
 * - Cleanup help message.
 * - Add ArgHidden code.
 * 
 * Revision 1.1  90/10/26  14:42:51  mcooper
 * Initial revision
 * 
 */

/*
 * Functions to parse options.
 */

#include "options.h"
#ifdef HAS_VARARGS
#include <varargs.h>
#endif

char *OptionChars = "-+";	/* Default option switching characters */
char *ProgramName = NULL;	/* Name of this program */
char *UsageString();
static int isopt();
static int suppress_help_msg = 0;
char *strcat();

/*
 * ParseOptions - Parse options found in argv using "options".
 *		  Returns the number of options parsed if there
 *		  were no errors.  Returns -1 if an error occurs.
 */
int ParseOptions(options, num_options, argc, argv)
     OptionDescRec *options;
     int num_options;
     int argc;
     char **argv;
{
    OptionDescRec *opt;
    register int x;
    char *p;

    if (ProgramName == NULL)
	ProgramName = argv[0];

#ifdef OPTION_DEBUG
    (void) printf("Option list is:\n");
    for (x = 0; x < num_options; ++x) {
	opt = &options[x];
	(void) printf("%s\n", opt->option);
    }

    (void) printf("Arguments (%d):", argc);
    for (x = 0; x < argc; ++x) {
	(void) printf(" %s", argv[x]);
    }
    (void) printf("\n");
#endif /* OPTION_DEBUG */

    for (x = 1; x < argc; ++x) {
	if (strcmp(HELPSTR, argv[x]) == 0) {
	    HelpOptions(options, num_options, (char **)NULL);
	    exit(0);
	}

	opt = FindOption(options, num_options, argv[x]);
	if (opt == NULL) {
	    if (isopt(argv[x])) { /* this was suppose to be an option */
		UsageOptions(options, num_options, argv[x]);
		return(-1);
	    } else { /* must be end of options */
		break;
	    }
	}

	if (opt->flags & NoArg) {
	    if (!(*opt->cvtarg)(opt, opt->value, FALSE)) {
		UsageOptions(options, num_options, opt->option);
		return(-1);
	    }
	} else if (opt->flags & IsArg) {
	    if (!(*opt->cvtarg)(opt, opt->option, FALSE)) {
		UsageOptions(options, num_options, opt->option);
		return(-1);
	    }
	} else if ((opt->flags & StickyArg) && (opt->flags & SepArg)) {
	    p = (char *) &argv[x][strlen(opt->option)];
	    if (!*p) {		/*** SepArg ***/
		if (x + 1 >= argc || isopt(argv[x+1])) {
		    if (opt->value == (caddr_t) NULL) {
			UserError("%s: Option requires an argument.", argv[x]);
			UsageOptions(options, num_options, opt->option);
			return(-1);
		    }
		    p = opt->value;
		} else {
		    p = argv[++x];
		}
	    }
	    if (!(*opt->cvtarg)(opt, p, TRUE)) {
		UsageOptions(options, num_options, opt->option);
		return(-1);
	    }
	} else if (opt->flags & StickyArg) {
	    p = (char *) &argv[x][strlen(opt->option)];
	    if (!*p) {
		if (opt->value == (caddr_t) NULL) {
		    UserError("%s: Option requires an argument.", argv[x]);
		    UsageOptions(options, num_options, opt->option);
		    return(-1);
		} else {
		    p = opt->value;
		}
	    }
	    if (!(*opt->cvtarg)(opt, p, TRUE)) {
		UsageOptions(options, num_options, opt->option);
		return(-1);
	    }
	} else if (opt->flags & SepArg) {
	    if (x + 1 >= argc || isopt(argv[x+1])) {
		if (opt->value == (caddr_t) NULL) {
		    UserError("%s: Option requires an argument.", argv[x]);
		    UsageOptions(options, num_options, opt->option);
		    return(-1);
		} else {
		    p = opt->value;
		}
	    } else {
		p = argv[++x];
	    }
	    if (!(*opt->cvtarg)(opt, p, TRUE)) {
		UsageOptions(options, num_options, opt->option);
		return(-1);
	    }
	} else if (opt->flags & SkipArg) {
	    x += 2;
	} else if (opt->flags & SkipLine) {
	    return(x);
	} else if (opt->flags & SkipNArgs) {
	    if (opt->value) {
		x += atoi(opt->value);
	    } else {
		UserError("Internal Error: No 'value' set for SkipNArgs.");
		return(-1);
	    }
	} else {
	    UserError("Internal Error: Unknown argument type for option '%s'.",
		     opt->option);
	    return(-1);
	}
    }

    return(x);
}

/*
 * FindOption - Find "option" in "options".  Returns NULL if not found.
 */
OptionDescRec *FindOption(options, num_options, option)
     OptionDescRec *options;
     int num_options;
     char *option;
{
    OptionDescRec *opt;
    register int x;

    for (x = 0; x < num_options; ++x) {
	opt = &options[x];
	if (opt->flags & StickyArg) {
	    if (strncmp(option, opt->option, strlen(opt->option)) == 0)
		return(opt);
	} else {
	    if (strncmp(option, opt->option, strlen(option)) == 0)
		return(opt);
	}
    }

    return(NULL);
}

/*
 * isopt - Is "str" an option string?  Compare first char of str against
 *	   list of option switch characters.  Returns TRUE if it is an option.
 */
static int isopt(str)
     char *str;
{
    register char *p;

    for (p = OptionChars; p && *p; *++p) {
	if (*str == *p) {
	    return(TRUE);
	}
    }

    return(FALSE);
}

/*
 * UsageOptions - Print a usage message based on "options".
 */
void UsageOptions(options, num_opts, badOption)
     OptionDescRec *options;
     int num_opts;
     char *badOption;
{
    OptionDescRec *opt;
    char *optstr;
    register int x;
    int col, len;

    if (badOption) 
	(void) fprintf (stderr, "%s:  bad command line option \"%s\"\r\n\n",
			ProgramName, badOption);

    (void) fprintf (stderr, "usage:  %s", ProgramName);
    col = 8 + strlen(ProgramName);
    for (x = 0; x < num_opts; x++) {
	opt = &options[x];
	if (opt->flags & ArgHidden)
	    continue;
	optstr = UsageString(opt);
	len = strlen(optstr) + 3;	/* space [ string ] */
	if (col + len > 79) {
	    (void) fprintf (stderr, "\r\n   ");  /* 3 spaces */
	    col = 3;
	}
	(void) fprintf (stderr, " [%s]", optstr);
	col += len;
    }

    if (suppress_help_msg)
	(void) fprintf(stderr, "\r\n\n");
    else
	(void) fprintf(stderr, 
		       "\r\n\nType \"%s %s\" for a full description.\r\n\n",
		       ProgramName, HELPSTR);
}

/*
 * HelpOptions - Print a nice help/usage message based on options.
 */
void HelpOptions(options, num_opts, message)
     OptionDescRec *options;
     int num_opts;
     char **message;
{
    OptionDescRec *opt;
    register int x;
    char **cpp;

    suppress_help_msg = 1;
    UsageOptions(options, num_opts, (char *)NULL);
    suppress_help_msg = 0;

    (void) fprintf (stderr, "where options include:\n");
    for (x = 0; x < num_opts; x++) {
	opt = &options[x];
	if (opt->flags & ArgHidden)
	    continue;
	(void) fprintf (stderr, "    %-28s %s\n", UsageString(opt), 
		 (opt->desc) ? opt->desc : "");
	if (opt->value && opt->cvtarg != OptBool)
	    (void) fprintf (stderr, "    %-28s [ Default value is %s ]\n", 
			    "", opt->value);
    }

    if (message) {
	(void) putc ('\n', stderr);
	for (cpp = message; *cpp; cpp++) {
	    (void) fputs (*cpp, stderr);
	    (void) putc ('\n', stderr);
	}
	(void) putc ('\n', stderr);
    }
}

/*
 * UserError - Print a user error.
 */
#ifdef HAS_VARARGS
void UserError(va_alist)
     va_dcl
{
    va_list args;
    char *fmt;

    va_start(args);
    if (ProgramName)
	(void) fprintf(stderr, "%s: ", ProgramName);
    fmt = (char *) va_arg(args, char *);
    (void) vfprintf(stderr, fmt, args);
    va_end(args);
    (void) fprintf(stderr, "\n");
}
#else
void UserError(fmt, a1, a2, a3, a4, a5, a6)
     char *fmt;
{
    if (ProgramName)
	(void) fprintf(stderr, "%s: ", ProgramName);
    (void) fprintf(stderr, fmt, a1, a2, a3, a4, a5, a6);
    (void) fprintf(stderr, "\n");
}
#endif

OptBool(opt, value, docopy)
     OptionDescRec *opt;
     caddr_t value;
     int docopy; /*ARGSUSED*/
{
    char *vpp;

    *(int *) opt->valp = (int) strtol(value, &vpp, 0);
    if (*vpp) {
	UserError("Invalid integer argument for '%s'.", opt->option);
	return(FALSE);
    } else {
	return(TRUE);
    }
}

OptInt(opt, value, docopy)
     OptionDescRec *opt;
     caddr_t value;
     int docopy; /*ARGSUSED*/
{
    char *vpp;

    *(int *) opt->valp = (int) strtol(value, &vpp, 0);
    if (*vpp) {
	UserError("Invalid integer argument for '%s'.", opt->option);
	return(FALSE);
    } else {
	return(TRUE);
    }
}

OptShort(opt, value, docopy)
     OptionDescRec *opt;
     caddr_t value;
     int docopy; /*ARGSUSED*/
{
    char *vpp;

    *(short *) opt->valp = (short) strtol(value, &vpp, 0);
    if (*vpp) {
	UserError("Invalid integer argument for '%s'.", opt->option);
	return(FALSE);
    } else {
	return(TRUE);
    }
}

OptLong(opt, value, docopy)
     OptionDescRec *opt;
     caddr_t value;
     int docopy; /*ARGSUSED*/
{
    char *vpp;

    *(long *) opt->valp = (long) strtol(value, &vpp, 0);
    if (*vpp) {
	UserError("Invalid integer argument for '%s'.", opt->option);
	return(FALSE);
    } else {
	return(TRUE);
    }
}

OptStr(opt, value, docopy)
     OptionDescRec *opt;
     caddr_t value;
     int docopy;
{
    char *p;

    if (docopy) {
	if ((p = (char *) malloc((unsigned)strlen(value)+1)) == NULL) {
	    UserError("Cannot malloc memory.");
	    return(FALSE);
	}
	(void) strcpy(p, value);
    } else {
	p = value;
    }

    *(char **) opt->valp = p;

    return(TRUE);
}

static char *UsageString(opt)
     OptionDescRec *opt;
{
    static char buf[BUFSIZ], buf2[BUFSIZ];

    (void) sprintf(buf, opt->option);
    (void) strcpy(buf2, "");
    if (opt->usage) {
	(void) sprintf(buf2, "%s%s%s%s",
		       ((opt->flags & StickyArg) && 
			!((opt->flags & StickyArg) && (opt->flags & SepArg))) 
		       ? "" : " ",
		       (opt->value) ? "[" : "",
		       opt->usage,
		       (opt->value) ? "]" : ""
		       );
    }
    (void) strcat(buf, buf2);

    return(buf);
}
