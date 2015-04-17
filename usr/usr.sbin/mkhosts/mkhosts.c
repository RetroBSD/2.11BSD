/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

/* static char sccsid[] = "@(#)mkhosts.c	5.1 (Berkeley) 5/28/85"; */
static char sccsid[] = "@(#)mkhosts.c	1.2 (2.11BSD) 1995/08/17";
#endif

#include <ctype.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include <netdb.h>
#include <ndbm.h>

void	keylower();
char	buf[BUFSIZ];

main(argc, argv)
	char *argv[];
{
	DBM *dp;
	register struct hostent *hp;
	struct hostent *hp2;
	datum key, content;
	register char *cp, *tp, **sp;
	register int *nap;
	int naliases, naddrs;
	int verbose = 0, entries = 0, maxlen = 0, error = 0;
	char tempname[BUFSIZ], newname[BUFSIZ];
	char lowname[MAXHOSTNAMELEN + 1];

	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		verbose++;
		argv++, argc--;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: mkhosts [ -v ] file\n");
		exit(1);
	}
	if (access(argv[1], R_OK) < 0) {
		perror(argv[1]);
		exit(1);
	}
	umask(0);

	sprintf(tempname, "%s.new", argv[1]);
	dp = dbm_open(tempname, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (dp == NULL) {
		fprintf(stderr, "dbm_open failed: ");
		perror(argv[1]);
		exit(1);
	}
	sethostfile(argv[1]);
	sethostent(1);
	while (hp = gethostent()) {
		cp = buf;
		tp = hp->h_name;
		while (*cp++ = *tp++)
			;
		nap = (int *)cp;
		cp += sizeof (int);

		keylower(lowname, hp->h_name);
		key.dptr = lowname;
		key.dsize = strlen(lowname);
/*
			key.dptr = hp->h_name;
			key.dsize = strlen(hp->h_name);
*/
		hp2 = (struct hostent *)fetchhost(dp, key);
		if (hp2) {
			merge(hp, hp2);
			hp = hp2;
		}
		naliases = 0;
		for (sp = hp->h_aliases; *sp; sp++) {
			tp = *sp;
			while (*cp++ = *tp++)
				;
			naliases++;
		}
		bcopy((char *)&naliases, (char *)nap, sizeof(int));
		bcopy((char *)&hp->h_addrtype, cp, sizeof (int));
		cp += sizeof (int);
		bcopy((char *)&hp->h_length, cp, sizeof (int));
		cp += sizeof (int);
		for (naddrs = 0, sp = hp->h_addr_list; *sp; sp++) {
			bcopy(*sp, cp, hp->h_length);
			cp += hp->h_length;
			naddrs++;
		}
		content.dptr = buf;
		content.dsize = cp - buf;
		if (verbose)
			printf("store %s, %d aliases %d addresses\n", hp->h_name, naliases, naddrs);
		if (dbm_store(dp, key, content, DBM_REPLACE) < 0) {
			perror(hp->h_name);
			goto err;
		}
		for (sp = hp->h_aliases; *sp; sp++) {
			keylower(lowname, *sp);
			key.dptr = lowname;
			key.dsize = strlen(lowname);
/*
				key.dptr = *sp;
				key.dsize = strlen(*sp);
*/
			if (dbm_store(dp, key, content, DBM_REPLACE) < 0) {
				perror(*sp);
				goto err;
			}
		}
		for (sp = hp->h_addr_list; *sp; sp++) {
			key.dptr = *sp;
			key.dsize = hp->h_length;
			if (dbm_store(dp, key, content, DBM_REPLACE) < 0) {
				perror("dbm_store host address");
				goto err;
			}
		}
		entries++;
		if (cp - buf > maxlen)
			maxlen = cp - buf;
	}
	endhostent();
	dbm_close(dp);

	sprintf(tempname, "%s.new.pag", argv[1]);
	sprintf(newname, "%s.pag", argv[1]);
	if (rename(tempname, newname) < 0) {
		perror("rename .pag");
		exit(1);
	}
	sprintf(tempname, "%s.new.dir", argv[1]);
	sprintf(newname, "%s.dir", argv[1]);
	if (rename(tempname, newname) < 0) {
		perror("rename .dir");
		exit(1);
	}
	printf("%d host entries, maximum length %d\n", entries, maxlen);
	exit(0);
err:
	sprintf(tempname, "%s.new.pag", argv[1]);
	unlink(tempname);
	sprintf(tempname, "%s.new.dir", argv[1]);
	unlink(tempname);
	exit(1);
}

/* following code lifted from libc/net/hosttable/gethnamadr.c */

#define	MAXALIASES	35
#define	MAXADDRS	10

static	struct	hostent	host2;
static	char	*hstaliases[MAXALIASES];
static	char	*hstaddrs[MAXADDRS];
static	char	buf2[BUFSIZ];

static struct hostent *
fetchhost(dp, key)
	DBM	*dp;
	datum key;
{
        register char *cp, **ap;
	register int naddrs;
	int naliases;

	key = dbm_fetch(dp, key);
	if (key.dptr == 0)
                return ((struct hostent *)NULL);
	bcopy(key.dptr, buf2, key.dsize);
        cp = buf2;
	host2.h_name = cp;
	while (*cp++)
		;
	bcopy(cp, (char *)&naliases, sizeof(int));
	cp += sizeof (int);
	for (ap = hstaliases; naliases > 0; naliases--) {
		*ap++ = cp;
		while (*cp++)
			;
	}
	*ap = (char *)NULL;
	host2.h_aliases = hstaliases;
	bcopy(cp, (char *)&host2.h_addrtype, sizeof (int));
	cp += sizeof (int);
	bcopy(cp, (char *)&host2.h_length, sizeof (int));
	cp += sizeof (int);
	host2.h_addr_list = hstaddrs;
	naddrs = (key.dsize - (cp - buf2)) / host2.h_length;
	if (naddrs > MAXADDRS)
		naddrs = MAXADDRS;
	for (ap = hstaddrs; naddrs; naddrs--) {
		*ap++ = cp;
		cp += host2.h_length;
	}
	*ap = (char *)NULL;
        return (&host2);
}

merge(hp2, hp)
	struct	hostent	*hp2, *hp;
{
register char	**sp, **sp2;
	char	**endalias, **endadr, **hp2ali, **hp2adr;
	long	l;

	hp2ali = &hp2->h_aliases[0];
	hp2adr = &hp2->h_addr_list[0];

	for (sp = hp->h_addr_list; *sp; sp++)
		;
	endadr = sp;
	for (sp = hp->h_aliases; *sp; sp++)
		;
	endalias = sp;
	for (sp = hp->h_aliases; *sp && *hp2ali; sp++) {
		for (sp2 = hp2ali; *sp2; sp2++) {
			if (!strcmp(*sp2, *sp))
				break;
		}
		if (*sp2 == (char *)NULL) {
			*endalias++ = *hp2ali++;
			*endalias = (char *)NULL;
		}
	}
	for (sp = hp->h_addr_list; *sp && *hp2adr; sp++) {
		for (sp2 = hp2adr; *sp2; sp2++) {
			if (!bcmp(*sp2, *sp, hp->h_length))
				break;
		}
		if (*sp2 == (char *)NULL) {
			*endadr++ = *hp2adr++;
			*endadr = (char *)NULL;
		}
	}
}

void
keylower(out, in)
	register char *out, *in;
	{

	while	(*in)
		{
		if	(isupper(*in))
			*out++ = tolower(*in++);
		else
			*out++ = *in++;
		}
	*out++ = '\0';
	}
