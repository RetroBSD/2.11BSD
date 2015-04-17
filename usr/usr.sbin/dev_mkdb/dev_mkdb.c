/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#if	!defined(lint) && defined(DOSCCS)
static char copyright[] =
"@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";

static char sccsid[] = "@(#)dev_mkdb.c	8.1.2 (2.11BSD) 1999/10/24";
#endif

#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/dir.h>
#include <ndbm.h>
#include <stdio.h>
#include <paths.h>

extern	void	err();
	void	usage();

int
main(argc, argv)
	int argc;
	char *argv[];
{
	sigset_t set;
	register DIR *dirp;
	register struct direct *dp;
	struct stat sb;
	struct {
		mode_t type;
		dev_t dev;
	} bkey;
	DBM *db;
	datum data, key;
	int ch;
	u_char buf[MAXNAMLEN + 1];
	char dbtmp[MAXPATHLEN + 1], dbname[MAXPATHLEN + 1];
	char *varrun = _PATH_VARRUN;

	umask(022);

	while ((ch = getopt(argc, argv, "")) != EOF)
		switch((char)ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		usage();

	if (chdir(_PATH_DEV))
		err(1, "%s", _PATH_DEV);

	dirp = opendir(".");

	(void)sprintf(dbtmp, "%sdev.tmp", varrun);
	db = dbm_open(dbtmp, O_CREAT|O_EXLOCK|O_RDWR|O_TRUNC, 0644);
	if (db == NULL)
		err(1, "%s", dbtmp);

	/*
	 * Keys are a mode_t followed by a dev_t.  The former is the type of
	 * the file (mode & S_IFMT), the latter is the st_rdev field.  Note
	 * that the structure may contain padding, so we have to clear it
	 * out here.
	 */
	bzero(&bkey, sizeof(bkey));
	key.dptr = (char *)&bkey;
	key.dsize = sizeof(bkey);
	data.dptr = (char *)buf;
	while (dp = readdir(dirp)) {
		if (lstat(dp->d_name, &sb)) {
			warn("%s", dp->d_name);
			continue;
		}

		/* Create the key. */
		if ((sb.st_mode & S_IFMT) == S_IFCHR)
			bkey.type = S_IFCHR;
		else if ((sb.st_mode & S_IFMT) == S_IFBLK)
			bkey.type = S_IFBLK;
		else
			continue;
		bkey.dev = sb.st_rdev;

		/*
		 * Create the data; nul terminate the name so caller doesn't
		 * have to.
		 */
		bcopy(dp->d_name, buf, dp->d_namlen);
		buf[dp->d_namlen] = '\0';
		data.dsize = dp->d_namlen + 1;
		if (dbm_store(db, key, data, DBM_REPLACE) < 0)
			err(1, "dbm_store %s", dbtmp);
	}
	(void)dbm_close(db);

	(void)sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, NULL);

	sprintf(dbname, "%sdev.pag", varrun);
	sprintf(dbtmp,  "%sdev.tmp.pag", varrun);
	if (rename(dbtmp, dbname))
		err(1, "rename %s to %s", dbtmp, dbname);
	sprintf(dbname, "%sdev.dir", varrun);
	sprintf(dbtmp,  "%sdev.tmp.dir", varrun);
	if (rename(dbtmp, dbname))
		err(1, "rename %s to %s", dbtmp, dbname);
	exit(0);
}

void
usage()
{
	err(1, "usage: dev_mkdb");
}
