/*
** ident.h
**
** Author: Peter Eriksson <pen@lysator.liu.se>
** Intruder: Pär Emanuelsson <pell@lysator.liu.se>
*/

#ifndef __IDENT_H__
#define __IDENT_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* Sigh */
#ifdef __STDC__
#  if __STDC__ == 1
#    define IS_STDC 1
#  endif
#endif

#ifdef __P
#  undef __P
#endif
    
#ifdef IS_STDC
#  define __P(AL)	                  AL

#ifdef IN_LIBIDENT_SRC
    
#  define __P1(t1,a1) \
    (t1 a1)
    
#  define __P2(t1,a1,t2,a2) \
    (t1 a1, t2 a2)
	
#  define __P3(t1,a1,t2,a2,t3,a3) \
    (t1 a1, t2 a2, t3 a3)
	    
#  define __P4(t1,a1,t2,a2,t3,a3,t4,a4) \
    (t1 a1, t2 a2, t3 a3, t4 a4)

#  define __P5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) \
    (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)

#  define __P7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) \
    (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7)
#endif
    
#else

#  define __P(AL)	                  ()

#ifdef IN_LIBIDENT_SRC
    
#  define __P1(t1,a1)                     (a1) \
    t1 a1;
#  define __P2(t1,a1,t2,a2)               (a1, a2) \
    t1 a1; \
    t2 a2;
#  define __P3(t1,a1,t2,a2,t3,a3)         (a1, a2, a3) \
    t1 a1; \
    t2 a2; \
    t3 a3;
#  define __P4(t1,a1,t2,a2,t3,a3,t4,a4)   (a1, a2, a3, a4) \
    t1 a1; \
    t2 a2; \
    t3 a3; \
    t4 a4;
#  define __P5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5)   (a1, a2, a3, a4, a5) \
    t1 a1; \
    t2 a2; \
    t3 a3; \
    t4 a4; \
    t5 a5;
#  define __P7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) \
    (a1, a2, a3, a4, a5, a6, a7) \
    t1 a1; \
    t2 a2; \
    t3 a3; \
    t4 a4; \
    t5 a5; \
    t6 a6; \
    t7 a7;
#endif
#endif

#ifdef IS_STDC
#  undef IS_STDC
#endif

#ifdef _AIX
#  include <sys/select.h>
#endif
#ifdef __sgi
#  include <bstring.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>

#if defined(VMS) && !defined(FD_SETSIZE)
#  define FD_SETSIZE 64
#endif

/*
 * Sigh, GCC v2 complains when using undefined struct tags
 * in function prototypes...
 */
#if defined(__GNUC__) && !defined(INADDR_ANY)
#  define __STRUCT_IN_ADDR_P	void *
#else
#  define __STRUCT_IN_ADDR_P	struct in_addr *
#endif

#if defined(__GNUC__) && !defined(DST_NONE)
#  define __STRUCT_TIMEVAL_P	void *
#else
#  define __STRUCT_TIMEVAL_P	struct timeval *
#endif

#if defined(__sgi) && defined(_POSIX_SOURCE)
#  undef  __STRUCT_TIMEVAL_P
#  define __STRUCT_TIMEVAL_P	void *
#endif
	
#ifndef IDBUFSIZE
#  define IDBUFSIZE 2048
#endif

#ifndef IDPORT
#  define IDPORT	113
#endif

typedef struct
{
  int fd;
  char buf[IDBUFSIZE];
} ident_t;

typedef struct {
  int lport;                    /* Local port */
  int fport;                    /* Far (remote) port */
  char *identifier;             /* Normally user name */
  char *opsys;                  /* OS */
  char *charset;                /* Charset (what did you expect?) */
} IDENT;                        /* For higher-level routines */

/* Low-level calls and macros */
#define id_fileno(ID)	((ID)->fd)

extern ident_t * id_open __P((__STRUCT_IN_ADDR_P laddr,
			   __STRUCT_IN_ADDR_P faddr,
			   __STRUCT_TIMEVAL_P timeout));
  
extern int    id_close __P((ident_t *id));
  
extern int    id_query __P((ident_t *id,
			    int lport,
			    int fport,
			    __STRUCT_TIMEVAL_P timeout));
  
extern int    id_parse __P((ident_t *id,
			    __STRUCT_TIMEVAL_P timeout,
			    int *lport,
			    int *fport,
			    char **identifier,
			    char **opsys,
			    char **charset));
  
/* High-level calls */

extern IDENT *ident_lookup __P((int fd, int timeout));

extern char  *ident_id __P((int fd, int timeout));

extern IDENT *ident_query __P(( __STRUCT_IN_ADDR_P laddr, __STRUCT_IN_ADDR_P raddr, int lport, int rport, int timeout));

extern void   ident_free __P((IDENT *id));

extern char  id_version[];

#ifdef IN_LIBIDENT_SRC

extern char *id_strdup __P((char *str));
extern char *id_strtok __P((char *cp, char *cs, char *dc));

#endif

#ifdef	__cplusplus
}
#endif

#endif
