
/*  @(#)print.c 1.7 92/02/17
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


int
boldshow(hdr, str)      /* Display a header all in bold. */
char *hdr, *str ;
{
  useline() ;
  FPUTS("BoldFont ", stdout) ;
  startline() ;
  expand((unsigned char *) hdr) ;
  expand((unsigned char *) str) ;
  endline() ;
}


int
endcol()
{
  FPUTS("sf ", stdout) ;
  linect = 0 ;
  ++colct ;
  if (landscape == TRUE)
    PRINTF("(%d) %d endcol\n", (2*pageno)+colct-2, colct) ;
  else
    PRINTF("(%d) %d endcol\n", pageno, colct) ;
  if (numcols > 1)
    set_defs() ;         /* Needed for correct heading in multiple columns. */
  if (landscape == TRUE && colct != numcols) return ;
  pageno++ ;
}


int
endfile()
{
  linect = 0 ;
}


int
endline()
{
  PRINTF(") showline\n") ;
}


int
endpage()
{
  linect = 0 ;
  PRINTF("(%1d) endpage\n", tpn) ;
}


int
expand(s)  /* Display a string with PostScript-sensitive characters escaped */
unsigned char *s ;
{
  for (; s && *s; s++)
    {
      switch (*s)
        {
          case '\\' : FPUTS("\\\\", stdout) ;
                      break ;
          case '('  : FPUTS("\\(", stdout) ;
                      break ;
          case ')'  : FPUTS("\\)", stdout) ;
                      break ;
          default   : if ((*s < 0x20) || (*s > 0x7e))
                        FPRINTF(stdout,"\\%03o",*s) ;
                      else
                        if (isprint(*s)) PUTC(*s, stdout) ;
        }
    }
}


int
mixedshow(hdr, str)     /* Display a header in mixed bold/Roman. */
char *hdr, *str ;
{
  useline() ;
  FPUTS("BoldFont ", stdout) ;
  startline() ;
  expand((unsigned char *) hdr) ;
  FPUTS(") show pf (", stdout) ;
  expand((unsigned char *) str) ;
  endline() ;
}


int
psdef(name, def)        /* Do a PostScript define. */
char *name, *def ;
{
  PRINTF("/%s (", name) ;
  expand((unsigned char *) def) ;
  PRINTF(") def\n") ;
}


int
romanshow(str)          /* Display a header all in Roman. */
char *str ;
{
  useline() ;
  FPUTS("pf ", stdout) ;
  startline() ;
  expand((unsigned char *) str) ;
  endline() ;
}


void
set_defs()               /* Setup PostScript definitions. */
{
  int i ;

  if (article == TRUE)
    message_for = "Article from" ;                    /* MailFor. */
  else if (print_orig == TRUE && from != NULL)
    message_for = "From" ;
  psdef("MailFor", message_for) ;

  if (article == TRUE && newsgroups != NULL)          /* User. */
    {
      for (i = 0; i < strlen(newsgroups); i++)
        if (newsgroups[i] == ',' ||
            newsgroups[i] == '\0') break ;
      owner = (char *) realloc(owner, (unsigned int) i+1) ;
      STRNCPY(owner, newsgroups, i) ;
      owner[i] = '\0' ;
    }
  else if (print_orig == TRUE && from != NULL)
    {
      i = strlen(from) ;
      owner = (char *) realloc(owner, (unsigned int) i+1) ;
      STRNCPY(owner, from, i) ;
      owner[i] = '\0' ;
    }

  psdef("User", owner) ;
 
  do_date() ;                                         /* TimeNow. */
 
  if (text_doc && cmdfiles) subject = curfname ;
  psdef("Subject",
        (gsubject != NULL) ? gsubject : subject) ;    /* Subject. */
}


/* Display the PostScript prologue file for mp */
                   
int
show_prologue(pro)
char *pro ;              /* Prologue file name */
{
  FILE *cf, *pf ;
  char buf[MAXLINE], tmpstr[MAXLINE] ;
  char cpro[MAXPATHLEN] ;  /* Full pathname of the common prologue file. */
  int t2 ;                 /* Possible extract page or line length. */

  if ((pf = fopen(pro, "r")) == NULL)                                
    {
      FPRINTF(stderr,"%s: Prologue file %s not found.\n",progname, pro) ;
      exit(1) ;
    }
  while (fgets(buf, MAXLINE, pf) != NULL)
    {

/* NOTE: This is not nice code but...
 *
 *       Check if the line just read starts with /fullheight
 *       If this is the case, then replace it with the appropriate
 *       page height (A4 or US).
 */

      if (!strncmp(buf, "/fullwidth", 10))
        {
          if (paper_size == US)
            FPUTS("/fullwidth 8.5 inch def\n", stdout) ;
          else if (paper_size == A4)
            FPUTS("/fullwidth 595 def\n", stdout) ;
        }
      else if (!strncmp(buf, "/fullheight", 11))
        {
          if (paper_size == US)
            FPUTS("/fullheight 11 inch def\n", stdout) ;
          else if (paper_size == A4)
            FPUTS("/fullheight 842 def\n", stdout) ;
        }
      else FPUTS(buf, stdout) ;

/* Check for new line or page length. */
 
      tmpstr[0] = '\0' ;
      SSCANF(buf, "%s %d", tmpstr, &t2) ;
           if (strcmp(tmpstr, "%%PageLength") == 0)
        plen = t2 ;               /* Change the page length. */
      else if (strcmp(tmpstr, "%%LineLength") == 0)
        llen = t2 ;               /* Change the line length. */
      else if (strcmp(tmpstr, "%%NumCols")    == 0)
        numcols = t2 ;            /* Change the number of columns. */
      else if (strcmp(tmpstr, "%%EndComments") == 0)
        {

/* If this is the %%EndComments line from the prologue file, then we
 *  need to read (and output to stdout), the contents of the common
 *  prologue file, mp.common.ps
 */

          SPRINTF(cpro, "%s/mp.common.ps", prologue) ;
          if ((cf = fopen(cpro, "r")) == NULL)
            {
              FPRINTF(stderr,"%s: Common prologue file %s not found.\n",
                                  progname, cpro) ;
              exit(1) ;
            }
          while (fgets(buf, MAXLINE, cf) != NULL) FPUTS(buf, stdout) ;
          FCLOSE(cf) ;
        }
    }
  FCLOSE(pf) ;
}


int
startline()
{
  PRINTF("(") ;
}


int
startpage()
{
  PRINTF("%%%%Page: ? %1d\n", ++tpn) ;
  PRINTF("%1d newpage\n", tpn) ;
  set_defs() ;
  FPUTS("sf ", stdout) ;
}


int
startfile()
{
}


int
textshow(s)
char *s ;
{
  useline() ;
  startline() ;
  expand((unsigned char *) s) ;
  endline() ;
}


int
useline()   /* Called in order to ready a line for printing. */
{
  if (++linect > plen || end_of_page == TRUE)
    {
      endcol() ;
      if (colct < numcols) return ;
      colct = 0 ;
      endpage() ;
      linect = 1 ;
      startpage() ;
    }
}
