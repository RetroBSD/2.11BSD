#undef DEBUG
/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "config.h"
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)glob.c	5.12 (Berkeley) 6/24/91";
#endif /* LIBC_SCCS and not lint */
/*
 * Glob: the interface is a superset of the one defined in POSIX 1003.2,
 * draft 9.
 *
 * The [!...] convention to negate a range is supported (SysV, Posix, ksh).
 *
 * Optional extra services, controlled by flags not defined by POSIX:
 *
 * GLOB_QUOTE:
 *	Escaping convention: \ inhibits any special meaning the following
 *	character might have (except \ at end of string is retained).
 * GLOB_MAGCHAR:
 *	Set in gl_flags if pattern contained a globbing character.
 * GLOB_ALTNOT:
 *	Use ^ instead of ! for "not".
 * gl_matchc:
 *	Number of matches in the current invocation of glob.
 */

#ifdef notdef
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
typedef void * ptr_t;
#endif

#include "sh.h"
#undef QUOTE
#undef TILDE
#undef META
#undef CHAR
#undef ismeta
#undef Strchr

#include <glob.h>

#ifndef S_ISDIR
#define S_ISDIR(a)	(((a) & S_IFMT) == S_IFDIR)
#endif

#if !defined(S_ISLNK) && defined(S_IFLNK)
#define S_ISLNK(a)	(((a) & S_IFMT) == S_IFLNK)
#endif

#if !defined(S_ISLNK) && !defined(lstat)
#define lstat stat
#endif

#ifdef notdef
typedef unsigned short Char;
#endif

static	int	 glob1 		__P((Char *, glob_t *, int));
static	int	 glob2		__P((Char *, Char *, Char *, glob_t *, int));
static	int	 glob3		__P((Char *, Char *, Char *, Char *,
				     glob_t *, int));
static	int	 globextend	__P((Char *, glob_t *));
static	int	 match		__P((Char *, Char *, Char *, int));
static	int	 compare	__P((const void *, const void *));
static 	DIR	*Opendir	__P((Char *));
#ifdef S_IFLNK
static	int	 Lstat		__P((Char *, struct stat *));
#endif
static 	Char 	*Strchr		__P((Char *, int));
#ifdef DEBUG
static	void	 qprintf	__P((Char *));
#endif

#define	DOLLAR		'$'
#define	DOT		'.'
#define	EOS		'\0'
#define	LBRACKET	'['
#define	NOT		'!'
#define ALTNOT		'^'
#define	QUESTION	'?'
#define	QUOTE		'\\'
#define	RANGE		'-'
#define	RBRACKET	']'
#define	SEP		'/'
#define	STAR		'*'
#define	TILDE		'~'
#define	UNDERSCORE	'_'

#ifdef	SHORT_STRINGS
#define	M_META		0x8000
#define M_PROTECT	0x4000
#define	M_MASK		0xffff
#define	M_ASCII		0x00ff
#else
#define	M_META		0x0080 /* 0200 */
#define M_PROTECT	0x007f /* 0177 0300 */
#define	M_MASK		0x00ff /* 0377 */
#define	M_ASCII		0x007f /* 0177 */
#endif	/* SHORT_STRINGS */

#define	CHAR(c)		((c)&M_ASCII)
#define	META(c)		((c)|M_META)
#define	M_ALL		META('*')
#define	M_END		META(']')
#define	M_NOT		META('!')
#define	M_ALTNOT	META('^')
#define	M_ONE		META('?')
#define	M_RNG		META('-')
#define	M_SET		META('[')
#define	ismeta(c)	(((c)&M_META) != 0)

/*
 * Need to dodge two kernel bugs:
 * opendir("") != opendir(".")
 * NAMEI_BUG: on plain files trailing slashes are ignored in some kernels.
 *            POSIX specifies that they should be ignored in directories.
 */

static DIR *
Opendir(str)
    register Char *str;
{
    char    buf[MAXPATHLEN];
    register char *dc = buf;

    if (!*str)
	return (opendir("."));
    while (*dc++ = *str++);
    return (opendir(buf));
}

#ifdef S_IFLNK
static int
Lstat(fn, sb)
    register Char *fn;
    struct stat *sb;
{
    char    buf[MAXPATHLEN];
    register char *dc = buf;

    while (*dc++ = *fn++);
# ifdef NAMEI_BUG
    {
	int     st;

	st = lstat(buf, sb);
	if (*buf)
	    dc--;
	return (*--dc == '/' && !S_ISDIR(sb->st_mode) ? -1 : st);
    }
# else
    return (lstat(buf, sb));
# endif	/* NAMEI_BUG */
}
#else
#define Lstat Stat
#endif /* S_IFLNK */

static int
Stat(fn, sb)
    register Char *fn;
    struct stat *sb;
{
    char    buf[MAXPATHLEN];
    register char *dc = buf;

    while (*dc++ = *fn++);
#ifdef NAMEI_BUG
    {
	int     st;

	st = lstat(buf, sb);
	if (*buf)
	    dc--;
	return (*--dc == '/' && !S_ISDIR(sb->st_mode) ? -1 : st);
    }
#else
    return (stat(buf, sb));
#endif /* NAMEI_BUG */
}

static Char *
Strchr(str, ch)
    Char *str;
    int ch;
{
    do
	if (*str == ch)
	    return (str);
    while (*str++);
    return (NULL);
}

#ifdef DEBUG
static void
qprintf(s)
Char *s;
{
    Char *p;

    for (p = s; *p; p++)
	printf("%c", *p & 0xff);
    printf("\n");
    for (p = s; *p; p++)
	printf("%c", *p & M_PROTECT ? '"' : ' ');
    printf("\n");
    for (p = s; *p; p++)
	printf("%c", *p & M_META ? '_' : ' ');
    printf("\n");
}
#endif /* DEBUG */

static int
compare(p, q)
    const void  *p, *q;
{
    return (strcmp(*(char **) p, *(char **) q));
}

/*
 * The main glob() routine: comp_s the pattern (optionally processing
 * quotes), calls glob1() to do the real pattern matching, and finally
 * sorts the list (unless unsorted operation is requested).  Returns 0
 * if things went well, nonzero if errors occurred.  It is not an error
 * to find no matches.
 */
int
glob(pattern, flags, errfunc, pglob)
    const char *pattern;
    int     flags;
    int     (*errfunc) __P((char *, int));
    glob_t *pglob;
{
    int     err, o_pthc;
    Char *bufnext, *bufend, *comp_buf, m_not;
    const unsigned char *comp_pat, *patnext;
    int     c, not;
    Char patbuf[MAXPATHLEN + 1], *qpatnext;
    int     no_match;

    patnext = (unsigned char *) pattern;
    if (!(flags & GLOB_APPEND)) {
	pglob->gl_p_c = 0;
	pglob->gl_p_v = NULL;
	if (!(flags & GLOB_DOOFFS))
	    pglob->gl_offs = 0;
    }
    pglob->gl_flags = flags & ~GLOB_MAGCHAR;
    pglob->gl_errfunc = errfunc;
    o_pthc = pglob->gl_p_c;
    pglob->gl_matchc = 0;

    if (pglob->gl_flags & GLOB_ALTNOT) {
	not = ALTNOT;
	m_not = M_ALTNOT;
    }
    else {
	not = NOT;
	m_not = M_NOT;
    }

    no_match = *patnext == not;
    if (no_match)
	patnext++;

    bufnext = patbuf;
    bufend = bufnext + MAXPATHLEN;
    comp_buf = bufnext;
    comp_pat = patnext;
    if (flags & GLOB_QUOTE) {
	/* Protect the quoted characters */
	while (bufnext < bufend && (c = *patnext++) != EOS) 
	    if (c == QUOTE) {
		if ((c = *patnext++) == EOS) {
		    c = QUOTE;
		    --patnext;
		}
		*bufnext++ = c | M_PROTECT;
	    }
	    else
		*bufnext++ = c;
    }
    else 
	while (bufnext < bufend && (c = *patnext++) != EOS) 
	    *bufnext++ = c;
    *bufnext = EOS;

    bufnext = patbuf;
    qpatnext = patbuf;
    /* we don't need to check for buffer overflow any more */
    while ((c = *qpatnext++) != EOS) {
	switch (c) {
	case LBRACKET:
	    pglob->gl_flags |= GLOB_MAGCHAR;
	    c = *qpatnext;
	    if (c == not)
		++qpatnext;
	    if (*qpatnext == EOS ||
		Strchr(qpatnext + 1, RBRACKET) == NULL) {
		*bufnext++ = LBRACKET;
		if (c == not)
		    --qpatnext;
		break;
	    }
	    *bufnext++ = M_SET;
	    if (c == not)
		*bufnext++ = m_not;
	    c = *qpatnext++;
	    do {
		*bufnext++ = CHAR(c);
		if (*qpatnext == RANGE &&
		    (c = qpatnext[1]) != RBRACKET) {
		    *bufnext++ = M_RNG;
		    *bufnext++ = CHAR(c);
		    qpatnext += 2;
		}
	    } while ((c = *qpatnext++) != RBRACKET);
	    *bufnext++ = M_END;
	    break;
	case QUESTION:
	    pglob->gl_flags |= GLOB_MAGCHAR;
	    *bufnext++ = M_ONE;
	    break;
	case STAR:
	    pglob->gl_flags |= GLOB_MAGCHAR;
	    *bufnext++ = M_ALL;
	    break;
	default:
	    *bufnext++ = CHAR(c);
	    break;
	}
    }
    *bufnext = EOS;
#ifdef DEBUG
    qprintf(patbuf);
#endif

    if ((err = glob1(patbuf, pglob, no_match)) != 0)
	return (err);

    if (pglob->gl_p_c == o_pthc && flags & GLOB_NOCHECK) {
	if (!(flags & GLOB_QUOTE)) {
	    Char *dp = comp_buf;
	    const unsigned char *sp = comp_pat;

	    while (*dp++ = *sp++);
	}
	else {
	    /*
	     * copy pattern, interpreting quotes; this is slightly different
	     * than the interpretation of quotes above -- which should prevail?
	     */
	    while (*comp_pat != EOS) {
		if (*comp_pat == QUOTE) {
		    if (*++comp_pat == EOS)
			--comp_pat;
		}
		*comp_buf++ = (unsigned char) *comp_pat++;
	    }
	    *comp_buf = EOS;
	}
	return (globextend(patbuf, pglob));
    }
    else if (!(flags & GLOB_NOSORT))
	qsort((char *) (pglob->gl_p_v + pglob->gl_offs + o_pthc),
	      pglob->gl_p_c - o_pthc, sizeof(char *), compare);
    return (0);
}

static int
glob1(pattern, pglob, no_match)
    Char *pattern;
    glob_t *pglob;
    int     no_match;
{
    Char pathbuf[MAXPATHLEN + 1];

    /*
     * a null pathname is invalid -- POSIX 1003.1 sect. 2.4.
     */
    if (*pattern == EOS)
	return (0);
    return (glob2(pathbuf, pathbuf, pattern, pglob, no_match));
}

/*
 * functions glob2 and glob3 are mutually recursive; there is one level
 * of recursion for each segment in the pattern that contains one or
 * more meta characters.
 */
static int
glob2(pathbuf, pathend, pattern, pglob, no_match)
    Char *pathbuf, *pathend, *pattern;
    glob_t *pglob;
    int     no_match;
{
    struct stat sbuf;
    int anymeta;
    Char *p, *q;

    /*
     * loop over pattern segments until end of pattern or until segment with
     * meta character found.
     */
    anymeta = 0;
    for (;;) {
	if (*pattern == EOS) {	/* end of pattern? */
	    *pathend = EOS;
	    if (Lstat(pathbuf, &sbuf))
		return (0);

	    if (((pglob->gl_flags & GLOB_MARK) &&
		 pathend[-1] != SEP) &&
		(S_ISDIR(sbuf.st_mode)
#ifdef S_IFLNK
		 || (S_ISLNK(sbuf.st_mode) &&
		     (Stat(pathbuf, &sbuf) == 0) &&
		     S_ISDIR(sbuf.st_mode))
#endif
		 )) {
		*pathend++ = SEP;
		*pathend = EOS;
	    }
	    ++pglob->gl_matchc;
	    return (globextend(pathbuf, pglob));
	}

	/* find end of next segment, copy tentatively to pathend */
	q = pathend;
	p = pattern;
	while (*p != EOS && *p != SEP) {
	    if (ismeta(*p))
		anymeta = 1;
	    *q++ = *p++;
	}

	if (!anymeta) {		/* no expansion, do next segment */
	    pathend = q;
	    pattern = p;
	    while (*pattern == SEP)
		*pathend++ = *pattern++;
	}
	else			/* need expansion, recurse */
	    return (glob3(pathbuf, pathend, pattern, p, pglob, no_match));
    }
    /* NOTREACHED */
}


static int
glob3(pathbuf, pathend, pattern, restpattern, pglob, no_match)
    Char *pathbuf, *pathend, *pattern, *restpattern;
    glob_t *pglob;
    int     no_match;
{
    extern int errno;
    DIR    *dirp;
    struct dirent *dp;
    int     err;
    Char m_not = (pglob->gl_flags & GLOB_ALTNOT) ? M_ALTNOT : M_NOT;

    *pathend = EOS;
    errno = 0;

    if (!(dirp = Opendir(pathbuf)))
	/* todo: don't call for ENOENT or ENOTDIR? */
	if (pglob->gl_errfunc &&
	    (*pglob->gl_errfunc) (pathbuf, errno) ||
	    (pglob->gl_flags & GLOB_ERR))
	    return (GLOB_ABEND);
	else
	    return (0);

    err = 0;

    /* search directory for matching names */
    while ((dp = readdir(dirp))) {
	register unsigned char *sc;
	register Char *dc;

	/* initial DOT must be matched literally */
	if (dp->d_name[0] == DOT && *pattern != DOT)
	    continue;
	for (sc = (unsigned char *) dp->d_name, dc = pathend; *dc++ = *sc++;);
	if (match(pathend, pattern, restpattern, m_not) == no_match) {
	    *pathend = EOS;
	    continue;
	}
	err = glob2(pathbuf, --dc, restpattern, pglob, no_match);
	if (err)
	    break;
    }
    /* todo: check error from readdir? */
    (void) closedir(dirp);
    return (err);
}


/*
 * Extend the gl_p_v member of a glob_t structure to accomodate a new item,
 * add the new item, and update gl_p_c.
 *
 * This assumes the BSD realloc, which only copies the block when its size
 * crosses a power-of-two boundary; for v7 realloc, this would cause quadratic
 * behavior.
 *
 * Return 0 if new item added, error code if memory couldn't be allocated.
 *
 * Invariant of the glob_t structure:
 *	Either gl_p_c is zero and gl_p_v is NULL; or gl_p_c > 0 and
 *	 gl_p_v points to (gl_offs + gl_p_c + 1) items.
 */
#define JPN
#ifndef	JPN
static int
globextend(path, pglob)
    Char *path;
    glob_t *pglob;
{
    register char **pathv;
    register int i;
    unsigned int newsize;
    char   *copy;
    Char *p;

    newsize = sizeof(*pathv) * (2 + pglob->gl_p_c + pglob->gl_offs);
    pathv = (char **) (pglob->gl_p_v ?
		       realloc((ptr_t) pglob->gl_p_v, newsize) :
		       malloc((size_t) newsize));
    if (pathv == NULL)
	return (GLOB_NOSPACE);

    if (pglob->gl_p_v == NULL && pglob->gl_offs > 0) {
	/* first time around -- clear initial gl_offs items */
	pathv += pglob->gl_offs;
	for (i = pglob->gl_offs; --i >= 0;)
	    *--pathv = NULL;
    }
    pglob->gl_p_v = pathv;

    for (p = path; *p++;);
    if ((copy = (char *) malloc((size_t) (p - path))) != NULL) {
	register char *dc = copy;
	register Char *sc = path;

	while (*dc++ = *sc++);
	pathv[pglob->gl_offs + pglob->gl_p_c++] = copy;
    }
    pathv[pglob->gl_offs + pglob->gl_p_c] = NULL;
    return ((copy == NULL) ? GLOB_NOSPACE : 0);
}
#else
/*
 * jpn: pdp11 version to get around quadradic behavior of above version
 * by managing the increments of storage allocation directly
 * note: gl_offs not used in tcsh6.00.00
 */
#define NFIRST 100
#define NINC 50

static int
globextend(path, pglob)
    Char *path;
    glob_t *pglob;
{
    register char **pathv;
    register char **opathv;
    register int i;
    unsigned getmem;
    static unsigned int globsize = 0;
    static unsigned int pathslots = 0;

    if (pglob->gl_p_c == 0) {
	/* first pass; allow for NULL at end of array */
	getmem = sizeof(*pathv) * (1 + NFIRST + pglob->gl_offs);
	pathv = (char **) malloc((size_t) getmem);
	if (pathv == NULL)
	    return (GLOB_NOSPACE);
	globsize = getmem;
#undef DEBUG
#ifdef	DEBUG
	xprintf("new gl_p_c globsize=%d\n",globsize);
#endif	DEBUG
	pathslots = NFIRST;
	pglob->gl_p_v = pathv;
	if (pglob->gl_offs > 0) {
	    /* first time around -- clear initial gl_offs items */
	    pathv += pglob->gl_offs;
	    for (i = pglob->gl_offs; --i >= 0;)
		*--pathv = NULL;
	}
    } else if (pglob->gl_p_c >= pathslots) {
	/* high water, increment needed */
	char **npp;
	char **opp;
	opathv = pglob->gl_p_v;
	getmem = globsize + (NINC * sizeof(*pathv));
	pathv = (char **) malloc((size_t) getmem);
	if (pathv == NULL)
	    return (GLOB_NOSPACE);
	globsize = getmem;
#ifdef	DEBUG
	xprintf("new increment, globsize=%d\n",globsize);
#endif	DEBUG
	pathslots += NINC;

	/* copy slots including gl_offs if any, whether or not NULL */
	i = pglob->gl_offs + pglob->gl_p_c ;
	for( npp=pathv, opp=opathv; i > 0 ; i--) {
	    *npp++ = *opp++;
	}
	*npp = NULL;	/* no assumption that slots are NULL */
	pglob->gl_p_v = pathv;
	xfree(opathv);	/* free the old base pointer; data has been copied */
    } else {
	/* still room for more */
	pathv = pglob->gl_p_v;
#ifdef	DEBUG
	xprintf("same gl_p_v globsize=%d\n",globsize);
#endif	DEBUG
    }

    /* now that a free slot has been assured, save the path there */
    {
	register Char *p;
	char   *copy;
	register char *dc;
	register Char *sc;
	int i;

	for (p = path; *p++;);
	i = p - path;
	copy = (char *) malloc((size_t) (p - path));
	if (copy == NULL)
	    return (GLOB_NOSPACE);
	dc = copy;
	sc = path;
	while (*dc++ = *sc++);
	pathv[pglob->gl_offs + pglob->gl_p_c] = copy;
	pglob->gl_p_c++;
	pathv[pglob->gl_offs + pglob->gl_p_c] = NULL;
#ifdef	DEBUG
	xprintf("gl_p_c= %d\n",pglob->gl_p_c);
#endif	DEBUG
#ifdef	DEBUG2
	xprintf("path is %s\n",path);
	xprintf("malloc size is %d\n",i);
	xprintf("copy is %s\n",copy);
#endif	DEBUG2
    }
    return (0);
}
#endif	/* pdp11 */

/*
 * pattern matching function for filenames.  Each occurrence of the *
 * pattern causes a recursion level.
 */
static  int
match(name, pat, patend, m_not)
    register Char *name, *pat, *patend;
    int m_not;
{
    int ok, negate_range;
    Char c, k;

    while (pat < patend) {
	c = *pat++;
	switch (c & M_MASK) {
	case M_ALL:
	    if (pat == patend)
		return (1);
	    for (; *name != EOS; ++name) {
		if (match(name, pat, patend, m_not))
		    return (1);
	    }
	    return (0);
	case M_ONE:
	    if (*name++ == EOS)
		return (0);
	    break;
	case M_SET:
	    ok = 0;
	    k = *name++;
	    if (negate_range = ((*pat & M_MASK) == m_not))
		++pat;
	    while (((c = *pat++) & M_MASK) != M_END) {
		if ((*pat & M_MASK) == M_RNG) {
		    if (c <= k && k <= pat[1])
			ok = 1;
		    pat += 2;
		}
		else if (c == k)
		    ok = 1;
	    }
	    if (ok == negate_range)
		return (0);
	    break;
	default:
	    if (*name++ != c)
		return (0);
	    break;
	}
    }
    return (*name == EOS);
}

/* free allocated data belonging to a glob_t structure */
void
globfree(pglob)
    glob_t *pglob;
{
    register int i;
    register char **pp;

    if (pglob->gl_p_v != NULL) {
	pp = pglob->gl_p_v + pglob->gl_offs;
	for (i = pglob->gl_p_c; i--; ++pp)
	    if (*pp)
		xfree((ptr_t) * pp);
	xfree((ptr_t) pglob->gl_p_v);
	pglob->gl_p_c = 0; /* jpn */
    }
#ifdef DEBUG
    xprintf("did globfree\n");
#endif
}
