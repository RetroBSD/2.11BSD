
/*  @(#)io.c 1.4 92/02/17
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


/* Emptyline returns true if its argument is empty or whitespace only */
         
bool
emptyline(str)
char *str ;
{
  while (*str)
    {
      if (!isspace(*str)) return(FALSE) ;
      str++ ;
    }
  return(TRUE) ;
}


/*  Read an input line into nextline, setting end_of_file, end_of_page
 *  and end_of_line appropriately.
 */

void
readline()
{
  int c ;
  int i = 0 ;      /* Index into current line being read. */
  int len = 0 ;    /* Length of the current line. */

  if (end_of_file) return ;
  end_of_page = end_of_line = FALSE ;

  while (len < llen && (c = getc(fp)) != EOF && c != '\n' && c != '\f')
    {
      if (c == '\t')
        {
          do
            {
              nextline[i++] = ' ' ;
              len++ ;
            }
          while (len % 8 != 0 && len <= llen) ;
        }
      else
        { 
          nextline[i++] = c ;
          len++ ;
        }
      if (c == '\b')
        {
          len -= 2 ;
          i -= 2 ;
        }
    }
  nextline[i] = '\0' ;

  if (len == llen && c != EOF && c != '\n' && c != '\f')
    {
      c = getc(fp) ;
      if (c != EOF && c != '\n' && c != '\f') UNGETC(c, fp) ;
    }

  if (elm_if && c == '\f')
    {
      len-- ;
      i-- ;
    }

  switch (c)
    {
      case EOF  : if (i == 0) end_of_file = TRUE ;
                  else
                    { 
                      UNGETC(c, fp) ;
                      end_of_line = TRUE ;
                    }
                  break ;
      case '\n' : end_of_line = TRUE ;
                  break ;

/*  /usr/ucb/mail for some unknown reason, appends a bogus formfeed at
 *  the end of piped output. The next character is checked; if it's an
 *  EOF, then end_of_file is set, else the character is put back.
 */

      case '\f' : if ((c = getc(fp)) == EOF) end_of_file = TRUE ;
                  else UNGETC(c, fp) ;

                  end_of_line = TRUE ;
                  end_of_page = TRUE ;
                  break ;
    }
  clen = len + 1 ;          /* Current line length (includes newline). */
}
