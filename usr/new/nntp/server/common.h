/*
 * Common declarations, includes, and other goodies.
 *
 * @(#)common.h	1.26.1	(2.11BSD) 12/31/93
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/param.h>

#include "../common/nntp.h"
#include "../common/conf.h"

#ifdef SYSLOG
# ifdef FAKESYSLOG
#  include "fakesyslog.h"
# else
#  include <syslog.h>
# endif
#endif

#ifdef USG
extern struct passwd *getpwent(), *getpwuid(), *getpwnam();
# include <string.h>
#else not USG
# include <strings.h>
# include <sys/wait.h>
#endif not USG

#ifdef NDIR
# include <ndir.h>
#else not NDIR
# include <sys/dir.h>
#endif not NDIR

#ifdef FCNTL
# include <fcntl.h>
#endif FCNTL

/*
 * <dbm.h> stupidly defines NULL, which is why the following
 * brain death is necessary.
 */

#ifdef DBM
# undef NULL
# include <dbm.h>
# undef NULL
# define NULL	0
#endif DBM

#ifdef NDBM
# include <ndbm.h>
#endif

/*
 * Some generic maximums.
 */

#ifndef MAXPATHLEN
#define	MAXPATHLEN	1024
#endif not MAXPATHLEN

#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN	256
#endif not MAXHOSTNAMELEN

#define	MAXBUFLEN	1024

/*
 * For "spew()"
 */

#define	ARTICLE	0
#define	HEAD	1
#define	BODY	2
#define	STAT	3

/*
 * For "ngmatch()"
 */

#define	ALLBUT	1

#define	valid_art(s)	(atoi(s) != 0)

#define	putline(s)	fputs((s), stdout); putchar('\r'); putchar('\n');

extern	int	errno;

extern	char	*gets(), *fgets();
extern	char	*mktemp();
extern	FILE	*open_valid_art();
extern	FILE	*openartbyid();
extern	char	*gethistent();
extern	int	restreql();
extern	int	s1strneql();	/* for ngmatch */

extern	char	spooldir[];
extern	char	activefile[];
extern	char	accessfile[];
extern	char	historyfile[];
extern	char	ngdatefile[];
extern	char	inews[];
extern	char	rnews[];

extern	char	**group_array;
extern	int	num_groups;
extern	char	*homedir;
extern	int	ingroup;
extern	int	maxgroups;
extern	int	art_array[];
extern	int	art_ptr;
extern	FILE	*art_fp;
extern	int	num_arts;
extern	int	uid_poster, gid_poster;
extern	int	canread, canpost, canxfer;
extern	char	**ngpermlist;
extern	int	ngpermcount;

extern	char	nntp_version[];

#ifdef LOG
extern	int	grps_acsd, arts_acsd;
extern	char	hostname[];

extern	int	ih_accepted;
extern	int	ih_rejected;
extern	int	ih_failed;

extern	int	nn_told;
extern	int	nn_took;
#endif
