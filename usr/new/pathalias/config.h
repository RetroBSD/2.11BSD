/* pathalias -- by steve bellovin, as told to peter honeyman */

#undef STRCHR		/* have strchr -- system v and many others */

#undef UNAME		/* have uname() -- probably system v or 8th ed. */
#undef MEMSET		/* have memset() -- probably system v or 8th ed. */

#define GETHOSTNAME	/* have gethostname() -- probably bsd */
#define BZERO		/* have bzero() -- probably bsd */

/* default place for dbm output of makedb (or use -o at run-time) */
#define	ALIASDB	"/usr/new/lib/news/paths"

#define TMPFILES	/* use scratch files to reduce memory requirements. */


/**************************************************************************
 *									  *
 * +--------------------------------------------------------------------+ *
 * |									| *
 * |			END OF CONFIGURATION SECTION			| *
 * |									| *
 * |				EDIT NO MORE				| *
 * |									| *
 * +--------------------------------------------------------------------+ *
 *									  *
 **************************************************************************/

#ifdef MAIN
#ifndef lint
static char	*c_sccsid = "@(#)config.h	9.1 87/10/04";
#endif /*lint*/
#endif /*MAIN*/

/*
 * malloc/free fine tuned for pathalias.
 *
 * MYMALLOC should work everwhere, so it's not a configuration
 * option (anymore).  nonetheless, if you're getting strange
 * core dumps (or panics!), comment out the following manifest,
 * and use the inferior C library malloc/free.
 *
 * please report problems to citi!honey or honey@citi.umich.edu.
 */
#ifndef TMPFILES	/* don't use MYMALLOC with TMPFILES. */
#define MYMALLOC	/**/
#endif /*TMPFILES*/

#ifdef MYMALLOC
#define malloc mymalloc
#define calloc(n, s) malloc ((n)*(s))
#define free(s)
#define cfree(s)
extern char *memget();
#else /* !MYMALLOC */
extern char *calloc();
#endif /* MYMALLOC */

#ifdef STRCHR
#define index strchr
#define rindex strrchr
#else
#define strchr index
#define strrchr rindex
#endif

#ifdef BZERO
#define strclear(s, n)	((void) bzero((s), (n)))
#else /*!BZERO*/

#ifdef MEMSET
extern char	*memset();
#define strclear(s, n)	((void) memset((s), 0, (n)))
#else /*!MEMSET*/
extern void	strclear();
#endif /*MEMSET*/

#endif /*BZERO*/

extern char	*malloc();
extern char	*strcpy(), *index(), *rindex();
