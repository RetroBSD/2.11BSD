
/*  @(#)main.c 1.5 92/02/17
 *
 *  Takes a mail file, a news article or an ordinary file
 *  and pretty prints it on a Postscript printer.
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

/* Command line option flags */
 
bool article  = FALSE ;       /* Set for news in "Article from " format. */
bool content  = FALSE ;       /* Set if Content-Length: has message length. */
bool digest   = FALSE ;       /* Are we are printing a mail digest (-d) */
bool elm_if   = FALSE ;       /* ELM mail frontend intermediate file format */
bool folder   = FALSE ;       /* Set if we are printing a mail folder. */
bool landscape = FALSE ;      /* Set if we are printing in landscape mode. */
bool print_orig = FALSE ;     /* Print From rather than To in mail header. */
bool print_ps = TRUE ;        /* Print PostScript files if set. */
bool text_doc = FALSE ;       /* Printing normal text (-o) */
 
/* Header definitions. */
 
char *POSTSCRIPT_MAGIC = "%!" ;            /* First line of PS file. */
char *FROMHDR       = "From:" ;
char *FROM_HDR      = "From " ;            /* UNIX From header */
char *APP_FROMHDR   = "Apparently_from:" ;
char *TOHDR         = "To:" ;
char *APP_TOHDR     = "Apparently_to:" ;
char *CCHDR         = "Cc:" ;
char *SUBJECTHDR    = "Subject:" ;
char *DATEHDR       = "Date:" ;
char *NEWSGROUPSHDR = "Newsgroups:" ;
char *NEWSGROUPHDR  = "Newsgroup:" ;
char *REPLYHDR      = "Reply_to:" ;
char *CONTENT_LEN   = "Content-Length:" ;

/* Header lines. */

char *from            = NULL ;    /* From: */
char *from_           = NULL ;    /* From_ (UNIX from) */
char *apparently_from = NULL ;    /* Apparently_from: */
char *to[MAXCONT+1] ;             /* To: (can have multiple lines) */
char *apparently_to   = NULL ;    /* Apparently_to: */
char *cc[MAXCONT+1] ;             /* Cc: (can have multiple lines) */
char *subject         = NULL ;    /* Subject: (can be set from command line) */
char *gsubject        = NULL ;    /* Global subject set from command line. */
char *date            = NULL ;    /* Date: */
char *newsgroups      = NULL ;    /* Newsgroups: (news articles only) */
char *reply_to        = NULL ;    /* Reply-to: */
char *content_len     = NULL ;    /* Content-Length: */

/* Strings used in page processing. */

char curfname[MAXPATHLEN] ;       /* Current file being printed. */
char *message_for = "" ;          /* "[Mail,News,Listing] for " line */
char *nameptr ;                   /* Used to getenv the NAME variable. */
char *optarg ;                    /* Optional command line argument. */
char *owner       = NULL ;        /* Name of owner (usually equal to 'to') */
char *progname    = NULL ;        /* Name of this program. */
char *prologue    = PROLOGUE ;    /* Name of PostScript prologue file. */
char proname[MAXPATHLEN] ;        /* Full pathname of the prologue file. */
char *whoami      = NULL ;        /* Login name of user. */

/* Other globals. */

document_type doc_type = DO_MAIL ;  /* Printing type - default mail */
paper_type paper_size = US ;        /* Paper size - default US */

int clen = 0 ;              /* Current line length (including newline). */
int colct = 0;              /* Column count on current page. */
int cmdfiles = 0 ;          /* Set if file to print given on command line. */
int linect = 0 ;            /* Line count on current page. */
int llen = LINELENGTH ;     /* Number of characters per line. */
int mlen = 0 ;              /* Number of characters in message (-C option). */
int numcols = 1 ;           /* Number of columns per page */
int optind ;                /* Optional command line argument indicator. */
int pageno = 1 ;            /* Page number within message. */
int plen = PAGELENGTH ;     /* Number of lines per page. */
int tpn    = 0 ;            /* Total number of pages printed. */

/* Read-ahead variables. */

char nextline[MAXLINE] ;  /* Read-ahead of the mail message, minus nl */

bool end_of_file = FALSE ;     /* EOF indicator */
bool end_of_line ;             /* Is a newline removed from this line */
bool end_of_page = FALSE ;     /* end-of-page indicator - ^L on input */

FILE *fp ;                     /* File pointer for current file. */


int
main(argc, argv)
int argc ;
char **argv ;
{
  to[0] = cc[0] = NULL ;

  progname = argv[0] ;        /* Save this program name. */

/*  Try to get location of the mp prologue file from an environment variable.
 *  If it's not found, then use the default value.
 */

  if ((prologue = getenv("MP_PROLOGUE")) == NULL) prologue = PROLOGUE ;
  SPRINTF(proname, "%s/mp.pro.ps", prologue) ;

  get_options(argc, argv) ;   /* Read and process command line options. */

  show_prologue(proname) ;    /* Send prologue file to output. */

  FPUTS("%%EndProlog\n", stdout) ;

  if (argc - optind != 0) cmdfiles = 1 ;
  if (!cmdfiles)
    {
      fp = stdin ;                 /* Get input from standard input. */
      STRCPY(curfname, "stdin") ;
      printfile() ;                /* Pretty print *just* standard input. */
    }
  else
    for (; optind < argc; ++optind)
      {
        STRCPY(curfname, argv[optind]) ;    /* Current file to print. */
        if ((fp = fopen(curfname, "r")) == NULL)
          {
            FPRINTF(stderr, "%s: cannot open %s\n", progname, curfname) ;
            continue ;
          }
        colct = 0 ;
        pageno = 1 ;       /* Initialise current page number. */
        end_of_file = 0 ;  /* Reset in case there's another file to print. */
        printfile() ;      /* Pretty print current file. */
      }

  show_trailer() ;         /* Send trailer file to output. */

  exit(0) ;
/*NOTREACHED*/
}


int
printfile()    /* Create PostScript to pretty print the current file. */
{
  int blankslate ;    /* Nothing set up for printing. */
  bool eop ;          /* Set if ^L (form-feed) found. */

  readline() ;
  if (end_of_file)
    {
      FPRINTF(stderr, "mp: empty input file, nothing printed\n") ;
      exit(1) ;
    }
 
  if (!text_doc)
    parse_headers(FALSE) ;    /* Parse headers of mail or news article */
  init_setup() ;              /* Set values for remaining globals. */

  startfile();
  startpage() ;               /* Output initial definitions. */
  blankslate = 0 ;
  eop        = FALSE ;

/* Print the document */

  if (doc_type != DO_TEXT)
    {
      show_headers(FALSE) ;
#ifdef WANTED
      FPUTS("sf ", stdout) ;
#endif /*WANTED*/
    }

  while (!end_of_file)
    {
      if (blankslate)
        {
          startfile() ;
          startpage() ;               /* Output initial definitions. */
          blankslate = 0 ;
        }

      if (content && folder && mlen <= 0)
        {

/*  If the count has gone negative, then the Content-Length is wrong, so go
 *  back to looking for "\nFrom".
 */

          if (mlen < 0) content = FALSE ;
          else if ((hdr_equal(FROM_HDR) || hdr_equal(FROMHDR)) &&
                    isupper(nextline[0]))
            {
              eop    = FALSE ;
              linect = plen ;
              reset_headers() ;
              parse_headers(FALSE) ;
              show_headers(FALSE) ;
            }
          else content = FALSE ;
        }

      if (!content && folder &&
          (!elm_if && hdr_equal(FROM_HDR) ||
            elm_if && hdr_equal(FROMHDR)) && isupper(nextline[0]))
        {
          eop    = FALSE ;
          linect = plen ;
          reset_headers() ;
          parse_headers(FALSE) ;
          show_headers(FALSE) ;
        }
      if (digest &&
         (hdr_equal(FROMHDR) || hdr_equal(DATEHDR) || hdr_equal(SUBJECTHDR)) &&
          isupper(nextline[0]))
        {
          linect = plen ;
          parse_headers(TRUE) ;
          show_headers(TRUE) ;
        }

      if (print_ps && hdr_equal(POSTSCRIPT_MAGIC))
        {
          if (numcols) endcol() ;
          endpage() ;
          endfile() ;
          process_postscript() ;
          blankslate = 1 ;
        }
      else if (folder && end_of_page) eop = TRUE ;
      else
        {
          if (eop == TRUE) end_of_page = TRUE ;
          textshow(nextline) ;
          eop = FALSE ;
        }

      if (content) mlen -= clen ;

      readline() ;
    }    

  if (!blankslate)
    {
      if (numcols) endcol() ;
      endpage() ;
      endfile() ;
    }
  
  FCLOSE(fp) ;
}


int
process_postscript()
{
  int firstline = 1 ;   /* To allow a newline after the first line. */

  startpage() ;
  while (!hdr_equal(FROMHDR)    && !hdr_equal(DATEHDR) &&
         !hdr_equal(SUBJECTHDR) && !end_of_file)
    {
      PRINTF("%s", nextline) ;
      if (firstline) FPUTS("\n", stdout) ;
      firstline = 0 ;
      if (fgets(nextline, MAXLINE, fp) == NULL) end_of_file = TRUE ;
    }
  endpage() ;
}


int
show_trailer()
{
  FPUTS("%%Trailer\n", stdout) ;
  PRINTF("%%%%Pages: %1d\n", tpn) ;
}
