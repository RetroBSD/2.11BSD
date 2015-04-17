#ifndef lint
static char	*sccsid = "@(#)misc.c	1.25	(Berkeley) 2/6/88";
#endif

#include "../common/conf.h"

#include "common.h"

/*
 * open_valid_art -- determine if a given article name is valid;
 *		if it is, return a file pointer to the open article,
 *		along with a unique id of the article.
 *
 *	Parameters:	"artname" is a string containing the
 *			name of the article.
 *			"id" is space for us to put the article
 *			id in.
 *
 *	Returns:	File pointer to the open article if the
 *			article is valid; NULL otherwise
 *
 *	Side effects:	None.
 */

FILE *
open_valid_art(artname, id)
	char		*artname;
	char		*id;
{
	static int	crnt_art_num;
	static char	crnt_art_id[MAXBUFLEN];
	int		fd;
	struct stat	statbuf;

	if (art_fp != NULL) {
		if (crnt_art_num == atoi(artname)) {
			if (fseek(art_fp, (long) 0, 0) < 0)
				close_crnt();
			else {
				(void) strcpy(id, crnt_art_id);
				return (art_fp);
			}
		} else 
			close_crnt();
	}

	art_fp = fopen(artname, "r");

	if (art_fp == NULL)
		return (NULL);

	fd = fileno(art_fp);

	if (fstat(fd, &statbuf) < 0) {
		close_crnt();
		return (NULL);
	}

	if ((statbuf.st_mode & S_IFREG) != S_IFREG) {
		close_crnt();
		return (NULL);
	}

	get_id(art_fp, id);
	(void) strcpy(crnt_art_id, id);
	crnt_art_num = atoi(artname);
	return (art_fp);
}


/*
 * gethistent -- return the path name of an article if it's
 * in the history file.
 *
 *	Parameters:	"msg_id" is the message ID of the
 *			article, enclosed in <>'s.
 *
 *	Returns:	A char pointer to a static data area
 *			containing the full pathname of the
 *			article, or NULL if the message-id is not
 *			in thef history file.
 *
 *	Side effects:	opens dbm database
 *			(only once, keeps it open after that).
 *			Converts "msg_id" to lower case.
 */

#ifndef NDBM
# ifndef DBM
#  ifndef USGHIST
#   define USGHIST
#  endif not USGHIST
# endif not DBM
#endif not DBM

char *
gethistent(msg_id)
	char		*msg_id;
{
	char		line[MAXBUFLEN];
	char		*tmp;
	register char	*cp;
	long		ltmp;
	static char	path[MAXPATHLEN];
#ifdef USGHIST
	char		*histfile();
	register int	len;
#else not USGHIST
#ifdef DBM
	static int	dbopen = 0;
	datum		fetch();
#else not DBM
	static DBM	*db = NULL;	/* History file, dbm version */
#endif DBM
	datum		 key, content;
#endif USGHIST
	static FILE	*hfp = NULL;	/* history file, text version */

	for (cp = msg_id; *cp != '\0'; ++cp)
		if (isupper(*cp))
			*cp = tolower(*cp);

#ifdef USGHIST
	hfp = fopen(histfile(msg_id), "r");
	if (hfp == NULL) {
#ifdef SYSLOG
		syslog(LOG_ERR, "gethistent: histfile: %m");
#endif SYSLOG
		return (NULL);
	}

	len = strlen(msg_id);
	while (fgets(line, sizeof (line), hfp))
		if (!strncasecmp(msg_id, line, len))
			break;

	if (feof(hfp)) {
		(void) fclose(hfp);
		return (NULL);
	}
#else not USGHIST
#ifdef DBM
	if (!dbopen) {
		if (dbminit(historyfile) < 0) {
#ifdef SYSLOG
			syslog(LOG_ERR, "openartbyid: dbminit %s: %m",
				historyfile);
#endif SYSLOG
			return (NULL);
		} else
			dbopen = 1;
	}
#else	/* ndbm */
	if (db == NULL) {
		db = dbm_open(historyfile, O_RDONLY, 0);
		if (db == NULL) {
#ifdef SYSLOG
			syslog(LOG_ERR, "openartbyid: dbm_open %s: %m",
				historyfile);
#endif SYSLOG
			return (NULL);
		}
	}
#endif DBM

	key.dptr = msg_id;
	key.dsize = strlen(msg_id) + 1;

#ifdef DBM
	content = fetch(key);
#else	/* ndbm */
	content = dbm_fetch(db, key);
#endif DBM
	if (content.dptr == NULL)
		return (NULL);

	if (hfp == NULL) {
		hfp = fopen(historyfile, "r");
		if (hfp == NULL) {
#ifdef SYSLOG
			syslog(LOG_ERR, "message: fopen %s: %m",
				historyfile);
#endif SYSLOG
			return (NULL);
		}
	}

	bcopy(content.dptr, (char *)&ltmp, sizeof (long));
	if (fseek(hfp, ltmp, 0) < 0) {
#ifdef SYSLOG
		syslog(LOG_ERR, "message: fseek: %m");
#endif SYSLOG
		return (NULL);
	}

	(void) fgets(line, sizeof(line), hfp);
#endif USGHIST

	if ((cp = index(line, '\n')) != NULL)
		*cp = '\0';
	cp = index(line, '\t');
	if (cp != NULL)
		cp = index(cp+1, '\t');
	if (cp == NULL) {
#ifdef SYSLOG
		syslog(LOG_ERR,
		"message: malformed line in history file at %ld bytes, id %s",
			ltmp, msg_id);
#endif SYSLOG
		return (NULL);
	}
	tmp = cp+1;

	if ((cp = index(tmp, ' ')) != NULL)
		*cp = '\0';
	
	while ((cp = index(tmp, '.')) != NULL)
		*cp = '/';

	(void) strcpy(path, spooldir);
	(void) strcat(path, "/");
	(void) strcat(path, tmp);

	return (path);
}

/*
 * openartbyid -- open an article by message-id.
 *
 *	Arguments:	"msg_id" is the message-id of the article
 *			to open.
 *
 *	Returns:	File pointer to opened article, or NULL if
 *			the article was not in the history file or
 *			could not be opened.
 *
 *	Side effects:	Opens article.
 */

FILE *
openartbyid(msg_id)
	char	*msg_id;
{
	char	*path;

	path = gethistent(msg_id);
	if (path != NULL)
		return (fopen(path, "r"));
	else
		return (NULL);
}


/*
 * check_ngperm -- check to see if they're allowed to see this
 * article by matching Newsgroups: and Distribution: line.
 *
 *	Parameters:	"fp" is the file pointer of this article.
 *
 *	Returns:	0 if they're not allowed to see it.
 *			1 if they are.
 *
 *	Side effects:	None.
 */

check_ngperm(fp)
	register FILE	*fp;
{
	char		buf[MAXBUFLEN];
	register char	*cp;
	static char	**ngarray;
	int		ngcount;

	if (ngpermcount == 0)
		return (1);

	while (fgets(buf, sizeof (buf), fp) != NULL) {
		if (buf[0] == '\n')		/* End of header */
			break;
		if (buf[0] != 'N' && buf[0] != 'n')
			continue;
		cp = index(buf, '\n');
		if (cp)
			*cp = '\0';
		cp = index(buf, ':');
		if (cp == NULL)
			continue;
		*cp = '\0';
		if (!strcasecmp(buf, "newsgroups")) {
			ngcount = get_nglist(&ngarray, cp+2);
			break;
		}
	}

	(void) rewind(fp);

	if (ngcount == 0)	/* Either no newgroups or null entry */
		return (1);

	return (ngmatch(s1strneql, ALLBUT,
		ngpermlist, ngpermcount, ngarray, ngcount));
}


/*
 * spew -- spew out the contents of a file to stdout, doing
 * the necessary cr-lf additions at the end.  Finish with
 * a "." on a line by itself, and an fflush(stdout).
 *
 *	Parameters:	"how" tells what part of the file we
 *			want spewed:
 *				ARTICLE   The entire thing.
 *				HEAD	  Just the first part.
 *				BODY	  Just the second part.
 *			"fp" is the open file to spew from.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Changes current position in file.
 */

spew(fp, how)
	FILE		*fp;
	int		how;
{
	char		line[NNTP_STRLEN];
	register char	*cp;

#ifdef LOG
	++arts_acsd;
#endif

	if (how == STAT) {
		(void) fflush(stdout);
		return;
	}

	while (fgets(line, sizeof(line)-6, fp) != NULL && *line != '\n') {
		if (how == BODY)	/* We need to skip this anyway */
			continue;
		cp = index(line, '\n');
		if (cp != NULL)
			*cp = '\0';
		if (*line == '.')
			putchar('.');
		putline(line);
		if (cp == NULL) {
			for (;;) {
				if ((fgets(line, sizeof(line)-6, fp) == NULL)
				    || (index(line, '\n') != NULL))
					break;
			}
		}
	}

	if (how == HEAD) {
		putchar('.');
		putchar('\r');
		putchar('\n');
		(void) fflush(stdout);
		return;
	} else if (how == ARTICLE) {
		putchar('\r');
		putchar('\n');
	}

	while (fgets(line, sizeof(line)-6, fp) != NULL) {
		cp = index(line, '\n');
		if (cp != NULL)
			*cp = '\0';
		if (*line == '.')
			putchar('.');
		putline(line);

		if (cp == NULL) {
			for (;;) {
				if ((fgets(line, sizeof(line)-6, fp) == NULL)
				    || (index(line, '\n') != NULL))
					break;
			}
		}
	}
	putchar('.');
	putchar('\r');
	putchar('\n');
	(void) fflush(stdout);
}


/*
 * get_id -- get the message id of the current article.
 *
 *	Parameters:	"art_fp" is a pointer to the open file.
 *			"id" is space for the message ID.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Seeks and rewinds on "art_fp".
 *			Changes space pointed to by "id".
 */

get_id(art_fp, id)
	register FILE	*art_fp;
	char		*id;
{
	char		line[MAXBUFLEN];
	register char	*cp;

	while (fgets(line, sizeof(line), art_fp) != NULL) {
		if (*line == '\n')
			break;
		if (*line == 'M' || *line == 'm') {	/* "Message-ID" */
			if ((cp = index(line, ' ')) != NULL) {
				*cp = '\0';
				if (!strcasecmp(line, "Message-ID:")) {
					(void) strcpy(id, cp + 1);
					if ((cp = index(id, '\n')) != NULL)
						*cp = '\0';
					(void) rewind(art_fp);
					return;
				}
			}
		}
	}
	(void) rewind(art_fp);
	(void) strcpy(id, "<0>");
}
		

/*
 * close_crnt -- close the current article file pointer, if it's
 *	open.
 *
 *	Parameters:	None.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Closes "art_fp" if it's open; sets "art_fp" to NULL.
 */

close_crnt()
{
	if (art_fp != NULL)
		(void) fclose(art_fp);
	art_fp = NULL;
}


/*
 * findart -- find an article number in the article array.
 *
 *	Parameters:	"artname" is a string containing
 *			the name of the article.
 *
 *	Returns:	An index into "art_array",
 *			or -1 if "artname" isn't in "art_array".
 *			
 *	Side effects:	None.
 *
 *	Improvement:	Replace this linear search with a binary one.
 */

findart(artname)
	char		*artname;
{
	register int	i, artnum;

	artnum = atoi(artname);

	for (i = 0; i < num_arts; ++i)
		if (art_array[i] == artnum)
			return(i);

	return (-1);
}


/*
 * get_distlist -- return a nicely set up array of distribution groups
 * along with a count, when given an NNTP-spec distribution list
 * in the form <dist1,dist2,...,distn>.
 *
 *	Parameters:		"array" is storage for our array,
 *				set to point at some static data.
 *				"list" is the NNTP distribution list.
 *
 *	Returns:		Number of distributions found.
 *				-1 on error.
 *
 *	Side effects:		Changes static data area.
 */

get_distlist(array, list)
	char		***array;
	char		*list;
{
	char		*cp;
	int		distcount;
	static char	**dist_list = (char **) NULL;

	if (list[0] != '<')
		return (-1);

	cp = index(list + 1, '>');
	if (cp != NULL)
		*cp = '\0';
	else
		return (-1);

	for (cp = list + 1; *cp != '\0'; ++cp)
		if (*cp == ',')
			*cp = ' ';
	distcount = parsit(list + 1, &dist_list);
	*array = dist_list;
	return (distcount);
}


/*
 * lower -- convert a character to lower case, if it's upper case.
 *
 *	Parameters:	"c" is the character to be
 *			converted.
 *
 *	Returns:	"c" if the character is not
 *			upper case, otherwise the lower
 *			case eqivalent of "c".
 *
 *	Side effects:	None.
 */

char
lower(c)
	register char	c;
{
	if (isascii(c) && isupper(c))
		c = c - 'A' + 'a';
	return (c);
}


/* the following is from news 2.11 */

#ifdef USG
/*
** Generate the appropriate history subfile name
*/
char *
histfile(hline)
char *hline;
{
	char chr;	/* least significant digit of article number */
	static char subfile[BUFSIZ];

	chr = findhfdigit(hline);
	sprintf(subfile, "%s.d/%c", HISTORY_FILE, chr);
	return subfile;
}

findhfdigit(fn)
char *fn;
{
	register char *p;
	register int chr;

	p = index(fn, '@');
	if (p != NULL && p > fn)
		chr = *(p - 1);
	else
		chr = '0';
	if (!isdigit(chr))
		chr = '0';
	return chr;
}
bcopy(s, d, l)
	register char *s, *d;
	register int l;
{
	while (l-- > 0)
		*d++ = *s++;
}

bcmp(s1, s2, l)
	register char *s1, *s2;
	register int l;
{
	if (l == 0)
		return (0);

	do
		if (*s1++ != *s2++)
			break;
	while (--l);

	return (l);
}

bzero(p, l)
	register char *p;
	register int l;
{
	while (l-- > 0)
		*p++ = 0;
}

dup2(x,y)
int x,y;
{ 
	close(y); 
	return(fcntl(x, F_DUPFD,y ));
}
#endif USG
