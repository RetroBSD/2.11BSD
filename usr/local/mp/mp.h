
/*  @(#)mp.h 1.4 92/02/17
 *
 *  Contains all the global definitions used by mp.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <ctype.h>
#include <time.h>
#include <pwd.h>

#ifdef   SYSV
#include <string.h>
#endif /*SYSV*/

#define  FCLOSE       (void) fclose
#define  FPRINTF      (void) fprintf
#define  FPUTS        (void) fputs
#define  PRINTF       (void) printf
#define  PUTC         (void) putc
#define  SPRINTF      (void) sprintf
#define  SSCANF       (void) sscanf
#define  STRCPY       (void) strcpy
#define  STRNCPY      (void) strncpy
#define  UNGETC       (void) ungetc

/*  For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)                                              
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

/* Configuration constants */

#ifndef  PROLOGUE            /* PostScript prologue file */
#define  PROLOGUE     "/usr/local/lib"
#endif /*PROLOGUE*/

#define  EQUAL(val)   (!strncmp(val, nextline, strlen(val)))
#define  INC          argc-- ; argv++ ;

#ifdef  NOSTRCHR
#define  strchr       index
#endif /*NOINDEX*/

#define  LINELENGTH   80     /* Number of characters per line. */

#ifndef  MAXPATHLEN
#define  MAXPATHLEN   1024
#endif /*MAXPATHLEN*/

#define  MAXSIZES     4      /* Maximum number of different sizes. */

#define  NAMEFIELDS   3      /* Default no. of "words" from passwd gecos. */
#define  NAMELENGTH   18     /* Maximum allowable real user name. */
#define  PAGELENGTH   60     /* Number of lines per page. */
#define  MAXCONT      10     /* Maximum no of continuation header lines */
#define  MAXLINE      256    /* Maximum string length. */
 
#ifndef TRUE
#define TRUE          1
#define FALSE         0
#endif  /*TRUE*/
 
typedef enum {DO_MAIL, DO_NEWS, DO_TEXT} document_type ;
typedef enum { A4, US }                  paper_type ;
typedef char bool ;

extern time_t time          P((time_t *)) ;
extern struct tm *localtime P((const time_t *)) ;

bool emptyline              P((char *)) ;

extern FILE *fopen          P((const char *, const char *)) ;
extern void exit            P((int)) ;
extern char *asctime        P((const struct tm *)) ;
extern char *getenv         P((char *)) ;
extern char *getlogin       P(()) ;
extern char *gets           P((char *)) ;
extern char *malloc         P((unsigned int)) ;

#ifndef SYSV
extern char *realloc        P((char *, unsigned int)) ;
extern char *strchr         P((char *, int)) ;
extern char *strcpy         P((char *, char *)) ;
extern char *strncpy        P((char *, char *, int)) ;
#endif /*SYSV*/

int boldshow                P((char *, char *)) ;
int endcol                  P(()) ;
int endfile                 P(()) ;
int endline                 P(()) ;
int endpage                 P(()) ;
int expand                  P((unsigned char *)) ;
int get_opt                 P((int, char **, char *)) ;
int get_options             P((int, char **)) ;
int hdr_equal               P((char *)) ;
int main                    P((int, char **)) ;
int mixedshow               P((char *, char *)) ;
int printfile               P(()) ;
int process_name_field      P((char *, char *, int, int)) ;
int process_postscript      P(()) ;
int psdef                   P((char *, char *)) ;
int romanshow               P((char *)) ;
int show_prologue           P((char *)) ;
int show_trailer            P(()) ;
int startline               P(()) ;
int startfile               P(()) ;
int startpage               P(()) ;
int textshow                P((char *)) ;
int usage                   P(()) ;
int useline                 P(()) ;

void do_date                P(()) ;
void get_header             P((char *, char **)) ;
void get_mult_hdr           P((char *, char **)) ;
void init_setup             P(()) ;
void parse_headers          P((int)) ;
void readline               P(()) ;
void reset_headers          P(()) ;
void set_defs               P(()) ;
void show_headers           P((int)) ;
void show_mult_hdr          P((char *, char **)) ;
