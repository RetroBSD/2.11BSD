
/*  @(#)header.c 1.5 92/02/17
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
#include "extern.h"


/*  If input line is header of type 'hdr', get the contents of the header
 *  into 'dest' (dynamically allocated).
 */

void
get_header(hdr, dest)
char *hdr ;
char **dest ;
{
  if (hdr_equal(hdr))
    {
      *dest = malloc((unsigned) (strlen(nextline) - strlen(hdr) + 1)) ;
      STRCPY(*dest, nextline + strlen(hdr)) ;
    }
}


/*  If input line is header of type 'hdr', get header into dest. The header
 *  may have multiple lines. This skips input to next line.
 */
 
void
get_mult_hdr(hdr, dest)
char *hdr ;
char *dest[] ;
{ 
  int i = 0 ;
  
  if (hdr_equal(hdr))
    {
      get_header(hdr, dest) ;
      i++ ;
      readline() ;
      while (i < MAXCONT && !emptyline(nextline) && isspace(nextline[0]))
        {
          dest[i] = malloc((unsigned) (strlen(nextline) + 1)) ;
          STRCPY(dest[i], nextline) ;
          i++ ;
          readline() ;
        }
      dest[i] = NULL ;
    }
}


/*  Compare the first word of the current line (converted to lower-case,
 *  with the given header definition. Determine if they are equal.
 */

int
hdr_equal(val)
char val[MAXLINE] ;
{
  register char *nptr = nextline ;
  register char *wptr = val ;
  register int n, w ;

  do
    {
      n = *nptr++ ;
      w = *wptr++ ;
      if (isupper(n)) n = tolower(n) ;
      if (isupper(w)) w = tolower(w) ;
      if (n != w && w != '\0') return(0) ;
    }
  while (n != '\0' && w != '\0') ;
  return(1) ;
}


/*  Parse_headers is a function which reads and parses the message headers,
 *  extracting the bits which are of interest.
 *
 *  The document is on standard input; the document is read up to the end of
 *  the header; the next line is read ahead into 'nextline'.
 *
 *  Parameter:
 *  digest  indicates if parsing is of digest headers instead of message
 *          headers
 *
 *  Implicit Input:
 *  nextline  contains the next line from standard input
 *
 *  Side-effects:
 *  The function fills in the global header variables with headers found.
 *  The global variable doc_type is set to the document type
 *  The global variable nextline is set
 *  The document is read up to the line following the headers
 */


void
parse_headers(digest)
int digest ;            /* Parsing digest headers */
{
  char *colon ;         /* Pointer to colon in line */
  char *c ;             /* General character pointer */
  char tmpstr[MAXLINE] ;

/*  If not processing digest headers, determine if this article is an
 *  ordinary text file.
 */

  if (!digest)
    {
      if (!hdr_equal(FROM_HDR))         /* UNIX From_ header? */
        {
          colon = strchr(nextline, ':') ;
          if (colon == NULL)        /* No colon => not a header line */
            {
              doc_type = DO_TEXT ;
              return ;
            }
          c = nextline ;
          while (c < colon && (!isspace(*c))) c++ ;
          if (c != colon)      /* Whitespace in header name => not header */
            {
              doc_type = DO_TEXT ;
              return ;
            }
        }    
    }    

  doc_type = DO_MAIL ;    /* Default to mail document */

/* Parse headers */

  while (TRUE)
    {
      if (emptyline(nextline)) break ;    /* End of headers */

      if (!digest)
        {
          get_header(FROM_HDR,      &from_) ;
          get_header(APP_FROMHDR,   &apparently_from) ;
          get_header(APP_TOHDR,     &apparently_to) ;
          get_header(NEWSGROUPSHDR, &newsgroups) ;
          get_header(NEWSGROUPHDR,  &newsgroups) ;
          get_header(REPLYHDR,      &reply_to) ;

          get_mult_hdr(TOHDR,       to) ;
          if (emptyline(nextline)) break ;

          get_mult_hdr(CCHDR,       cc) ;
          if (emptyline(nextline)) break ;

          if (doc_type != DO_NEWS && hdr_equal(NEWSGROUPSHDR))
            doc_type = DO_NEWS ;
          if (doc_type != DO_NEWS && hdr_equal(NEWSGROUPHDR))
            doc_type = DO_NEWS ;
        }
      get_header(FROMHDR, &from) ;
      get_header(SUBJECTHDR, &subject) ;
      get_header(DATEHDR, &date) ;

      if (content)
        {
          get_header(CONTENT_LEN, &content_len) ;
          if (hdr_equal(CONTENT_LEN))
            {
              SSCANF(nextline, "%s %d", tmpstr, &mlen) ;

/*  The Content-Length: doesn't seem to include the initial blank line
 *  between the mail header and the message body, or the blank line after
 *  the message body and before the start of the next "From " header, so add
 *  in two for those.
 */
              mlen += 2 ;
            }
        }

      if (!hdr_equal(TOHDR) && !hdr_equal(CCHDR))
        {
          while (!end_of_file && !end_of_line)
            readline() ;                       /* Skip rest of long lines */
          readline() ;
        }
    }    
}


void
reset_headers()          /* Reset header values for next message. */
{
  int i ;
 
  if (from != NULL) free(from) ;
  if (from_ != NULL) free(from_) ;
  if (apparently_from != NULL) free(apparently_from) ;
  if (apparently_to != NULL) free(apparently_to) ;
  if (content_len != NULL)   free(content_len) ;
  if (date != NULL) free(date) ;
  if (newsgroups != NULL) free(newsgroups) ;
  if (reply_to != NULL) free(reply_to) ;
 
  from = from_ = apparently_from = apparently_to = NULL ;
  date = newsgroups = reply_to = subject = NULL ;
 
  for (i = 0; i < MAXCONT+1; i++)
    {
      if (to[i] != NULL) free(to[i]) ;
      if (cc[i] != NULL) free(cc[i]) ;
      to[i] = cc[i] = NULL ;
    }
}


/*  Show_headers outputs the headers in PostScript. Different headers are
 *  output depending 'digest'.
 */

void
show_headers(digest)
int digest ;
{
  if (digest)
    {
      if (from)       mixedshow(FROMHDR,    from) ;
      if (subject)    mixedshow(SUBJECTHDR, subject) ;
      if (date)       mixedshow(DATEHDR,    date) ;
    }
  else
    {
      if (from_)           boldshow(FROM_HDR,       from_) ;
      if (from)            mixedshow(FROMHDR,       from) ;
      if (apparently_from) mixedshow(APP_FROMHDR,   apparently_from) ;
      if (to[0])           show_mult_hdr(TOHDR,     to) ;
      if (apparently_to)   mixedshow(APP_TOHDR,     apparently_to) ;
      if (cc[0])           show_mult_hdr(CCHDR,     cc) ;
      if (reply_to)        mixedshow(REPLYHDR,      reply_to) ;
      if (newsgroups)      mixedshow(NEWSGROUPSHDR, newsgroups) ;
      if (subject)         mixedshow(SUBJECTHDR,    subject) ;
      if (date)            mixedshow(DATEHDR,       date) ;
    }
  FPUTS("sf ", stdout) ;
}


void
show_mult_hdr(hdr, val)
char *hdr ;              /* Name of header */
char *val[] ;            /* Value of header */
{
  mixedshow(hdr, *val) ;
  val++ ;
  while (*val) romanshow(*val++) ;
}

