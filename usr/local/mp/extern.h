
/*  @(#)extern.h 1.4 92/02/17
 *
 *  Contains all the external definitions used by mp.
 *
 *  Copyright (c) Steve Holden and Rich Burridge.
 *                All rights reserved.
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed, and no monies are exchanged.
 *
 *  No responsibility is taken for any errors or inaccuracies inherent
 *  either to the comments or the code of this program, but if
 *  reported to me then an attempt will be made to fix them.
 */

extern char *APP_FROMHDR ;
extern char *APP_TOHDR ;
extern char *CCHDR ;
extern char *CONTENT_LEN ;
extern char *DATEHDR ;
extern char *FROMHDR ;
extern char *FROM_HDR ;          /* UNIX From header */
extern char *NEWSGROUPSHDR ;
extern char *NEWSGROUPHDR ;
extern char *REPLYHDR ;
extern char *SUBJECTHDR ;
extern char *TOHDR ;

extern char *apparently_from ;   /* Apparently_from: */
extern char *apparently_to ;     /* Apparently_to: */
extern char *cc[] ;              /* Cc: (can have multiple lines) */
extern char *content_len ;       /* Content-Length: */
extern char *date ;              /* Date: */
extern char *from ;              /* From: */
extern char *from_ ;             /* From_ (UNIX from) */
extern char *gsubject ;          /* Global Subject set from command line. */
extern char *newsgroups ;        /* Newsgroups: (news articles only) */
extern char *reply_to ;          /* Reply-to: */
extern char *subject ;           /* Subject: (can be set from command line) */
extern char *to[] ;              /* To: (can have multiple lines) */

extern char curfname[] ;     /* Current file being printed. */
extern char *message_for ;   /* "[Mail,News,Listing] for " line */
extern char *nameptr ;       /* Used to getenv the NAME variable. */
extern char nextline[] ;     /* Read-ahead of the mail message, minus nl */
extern char *optarg ;        /* Optional command line argument. */
extern char *owner ;         /* Name of owner (usually equal to 'to') */
extern char *progname ;      /* Name of this program. */
extern char *prologue ;      /* Name of PostScript prologue file. */
extern char proname[] ;      /* Full pathname of the prologue file. */
extern char *whoami ;        /* Login name of user. */

extern int clen ;            /* Current line length (including newline). */
extern int colct ;           /* Column count on current page. */
extern int cmdfiles ;        /* Set if file to print given on command line. */
extern int linect ;          /* Line count on current page. */
extern int llen ;            /* Number of characters per line. */
extern int mlen ;            /* Number of characters in message (-C). */
extern int numcols ;         /* Number of columns per page */
extern int optind ;          /* Optional command line argument indicator. */
extern int pageno ;          /* Page number within message. */
extern int plen ;            /* Number of lines per page. */
extern int tpn ;             /* Total number of pages printed. */

extern bool article ;        /* Set for news in "Article from " format. */
extern bool content ;        /* Set if Content-Length: has message length. */
extern bool digest ;         /* Are we are printing a mail digest (-d) */
extern bool elm_if ;         /* Elm mail frontend intermediate file. */
extern bool folder ;         /* Set if we are printing a mail folder. */
extern bool landscape ;      /* Set if we are printing in landscape mode. */
extern bool print_orig ;     /* Print originators name rather then Mail for. */
extern bool print_ps ;       /* Print PostScript files if set. */
extern bool text_doc ;       /* Printing normal text (-o) */

extern bool end_of_file ;    /* EOF indicator */
extern bool end_of_line ;    /* Is a newline removed from this line */
extern bool end_of_page ;    /* end-of-page indicator - ^L on input */

extern document_type doc_type ;  /* Printing type - default mail */
extern paper_type paper_size ;   /* Paper size - default US */

extern FILE *fp ;            /* File pointer for current file. */
