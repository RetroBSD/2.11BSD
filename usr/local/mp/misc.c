
/*  @(#)misc.c 1.5 92/02/17
 *
 *  Copyright (c) Steve Holden and Rich Burridge.
 *                All rights reserved.
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed, and no monies are exchanged.
 *
 *  No responsibility is taken for any errors inherent either
 *  to the comments or the code of this program, but if reported
 *  to me then an attempt will be made to fix them.
 */

#include "mp.h"
#include "patchlevel.h"
#include "extern.h"


void
do_date()        /* Output Postscript definition for the date and time. */
{
  char *ptr ;            /* Pointer to current time or date string. */
  int len ;
  long clock ;           /* Used by the localtime function call. */
  struct tm *tm ;        /* Used by the localtime and asctime calls. */
  char *timenow ;        /* Used to set TimeNow field with users name. */

  if (date == NULL)
    {
      clock = time((time_t *) 0) ;
      tm = localtime(&clock) ;
      ptr = asctime(tm) ;
      ptr[24] = '\0' ;
    }
  else ptr = date ;

  if (article != TRUE) psdef("TimeNow", ptr) ;
  else
    {
      len = strlen(ptr) ;
      timenow = malloc((unsigned) (len + 6 + strlen(whoami) + 1)) ;
      SPRINTF(timenow, "%s  - (%s)", ptr, whoami) ;
      psdef("TimeNow", timenow) ;
    }
}


int
get_opt(argc, argv, options)
int argc ;
char **argv, *options ;
{
  char opch, *str, *ptr ;
  static int flag = 0 ;
  static int cur_argc ;
  static char **cur_argv ;

  if (flag == 0)
    {
      cur_argc = argc ;
      cur_argv = argv ;
      flag = 1 ;
      optind = 1 ;
    }

  if (cur_argc <= 1) return -1 ;

  if (--cur_argc >= 1)
    {
      str = *++cur_argv ;
      if (*str != '-') return -1 ;    /* Argument is not an option   */
      else
        {                             /* Argument is an option */
          if ((ptr = strchr(options, opch = *++str)) != (char *) 0)
            {
              ++optind ;
              optarg = ++str ;        /* Point to rest of argument if any  */
              if ((*++ptr == ':') && (*optarg == '\0'))
                {
                  if (--cur_argc <= 0) return '?' ;
                  optarg = *++cur_argv ;
                  ++optind ;
                }
              return opch ;
            }
          else if (opch == '-')
            {                         /* End of options */
              ++optind ;
              return -1 ;
            }
          else return '?' ;
        }
    } 
  return 0 ;                          /* Should never be reached. */
}


int
get_options(argc, argv)      /* Read and process command line options. */
int argc ;
char *argv[] ;
{
  int opch ;

  while ((opch = get_opt(argc, argv, "aA:CdefFlmop:P:s:t:U:v")) != -1)
    switch (opch)
      {
        case 'a' : article = TRUE ;      /* "Article from" format. */
                   break ;
        case 'A' : if (!strcmp(optarg, "4"))    /* A4 paper size. */
                     paper_size = A4 ;
                   break ;
        case 'C' : content = TRUE ;      /* Use Content-Length: header. */
                   break ;
        case 'd' : digest = TRUE ;       /* Print digest. */
                   break ;
        case 'e' : elm_if = TRUE ;       /* ELM intermediate file format. */
                   folder = TRUE ;       /* Kind of folder. */
                   break ;
        case 'F' : print_orig = TRUE ;   /* Print originators name. */
                   break ;
        case 'f' :      if (!strcmp(optarg, "f"))     /* Filofax output. */
                     SPRINTF(proname, "%s/mp.pro.ff.ps", prologue) ;
                   else if (!strcmp(optarg, "p"))     /* Franklin Planner. */
                     SPRINTF(proname, "%s/mp.pro.fp.ps", prologue) ;
                   break ;
        case 'l' : landscape = TRUE ;    /* Landscape printing. */
                   SPRINTF(proname, "%s/mp.pro.l.ps", prologue) ;
                   break ;
        case 'm' : folder = TRUE ;       /* Print mail folder. */
                   break ;
        case 'o' : text_doc = TRUE ;     /* Print ordinary text file */
                   break ;
        case 'P' : if (!strcmp(optarg, "S"))    /* Print PostScript files. */
                     print_ps = FALSE ;
                   break ;
        case 'p' : if (strlen(optarg))
                     STRCPY(proname, optarg) ;  /* New prologue file. */
                   break ;
        case 's' : if (strlen(optarg))
                     gsubject = optarg ;        /* New subject line. */
                   break ;
        case 't' :      if (!strcmp(optarg, "m"))     /* Time Manager. */
                     SPRINTF(proname, "%s/mp.pro.tm.ps", prologue) ;
                   else if (!strcmp(optarg, "s"))     /* Time/System Int. */
                     SPRINTF(proname, "%s/mp.pro.ts.ps", prologue) ;
                   break ;
        case 'U' : if (!strcmp(optarg, "S"))    /* US paper size. */
                     paper_size = US ;
                   break ;
        case '?' :
        case 'v' : usage() ;
      }
}


void
init_setup()            /* Set default values for various options. */
{
  char *c ;
  int amp_cnt = 0 ;     /* Number of ampersands in gecos field. */
  int i, len ;
  int namefields ;      /* Number of "words" from passwd gecos. */
  int namelength ;      /* Maximum number of characters from passwd gecos. */
  struct passwd *pp ;

#ifdef GECOSFIELDS
  namefields = GECOSFIELDS ;  /* New no. of "words" from passwd gecos. */
#else
  namefields = NAMEFIELDS ;   /* Not supplied; use default. */
#endif /*GECOSFIELDS*/

#ifdef GECOSLENGTH
  namelength = GECOSLENGTH ;  /* New max. no. of chars. from passwd gecos. */
#else
  namelength = NAMELENGTH ;   /* Not supplied; use default. */
#endif /*GECOSLENGTH*/

  c = getlogin() ;      /* Pointer to users login name. */
  if (c == NULL)        /* Get username from password file */
    {
      pp = getpwuid(geteuid()) ;
      if (pp == NULL) c = "printing" ;
      else c = pp->pw_name ;
    }
  owner = malloc((unsigned) (strlen(c) + 1)) ;
  STRCPY(owner, c) ;
  whoami = malloc((unsigned) (strlen(c) + 1)) ;   /* Save User login name */
  STRCPY(whoami, c) ;
 
/*  Have a look for the users gecos (normally real name), so that its a bit
 *  more recognisable. If this field is too long, then we need to truncate
 *  sensibly. We also need to check a few things. If we've extracted
 *  namefields "words" or have found a comma, then exit. If an ampersand is
 *  found, this is expanded to the users name in capitals.
 */    
     
  pp = getpwnam(owner) ;
  if (pp != NULL && pp->pw_gecos && pp->pw_gecos[0] != '\0')
    {  
      len = strlen(pp->pw_gecos) ;
      for (i = 0; i < len; i++)
        if (pp->pw_gecos[i] == '&') amp_cnt++ ;
 
      owner = malloc((unsigned) (strlen(pp->pw_gecos) +
                                 amp_cnt * strlen(c) + 1)) ;

      if ((nameptr = getenv("NAME")) != NULL)
        process_name_field(c, nameptr, namefields, namelength) ;
      else
        process_name_field(c, pp->pw_gecos, namefields, namelength) ;
    }

  if (text_doc) doc_type = DO_TEXT ;
  switch (doc_type)
    {
      case DO_TEXT : message_for = "Listing for ";
                     digest = FALSE ;
                     break ;
      case DO_MAIL : message_for = digest ? "Mail digest for " : "Mail for " ;
                     break ;
      case DO_NEWS : message_for = digest ? "News digest for " : "News for " ;
                     break ;
    }
}


/* Extract user name from $NAME or passwd GECOS. */

int
process_name_field(c, ptr, fields, length)
char *c, *ptr ;
int fields, length ;
{
  int i, j, len, n, spaces, slen ;

  n = spaces = 0 ;
  slen = strlen(ptr) ;
  for (i = 0; i < slen; i++)
    {
           if (*ptr == ',') break ;
      else if (*ptr == '&')
        {
          if (islower(c[0])) owner[n++] = toupper(c[0]) ;
          len = strlen(c) ;
          for (j = 1; j < len; j++)
            owner[n++] = c[j] ;
          ptr++ ;
        } 
      else if (*ptr == ' ' || *ptr == '\t')
        {
          if (++spaces == fields) break ;
          else
            while (*ptr == ' ' || *ptr == '\t') owner[n++] = *ptr++ ;
        } 
      else owner[n++] = *ptr++ ;
      if (n >= length) break ;
    } 
  if (n > length) n = length ;
  owner[n] = '\0' ;
}


int
usage()     /* Print usage message and exit. */
{
  FPRINTF(stderr, "%s version 2.5.%1d\n\n", progname, PATCHLEVEL) ;
  FPRINTF(stderr, "Usage: %s [-A4] [-F] [-PS] [-US] ", progname) ;
  FPRINTF(stderr, "[-a] [-d] [-e] [-f] [-l] [-m] [-o]\n", progname) ;
  FPRINTF(stderr, "\t[-p prologue] [-s subject] [-tm] [-ts] [-v] ") ;
  FPRINTF(stderr, "[-?] filename ...\n") ;
  exit(1) ;
}

