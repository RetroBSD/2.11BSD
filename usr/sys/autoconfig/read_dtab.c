/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)read_dtab.c	2.1 (2.11BSD GTE) 1/10/94
 */

#include <machine/autoconfig.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include "dtab.h"
#include "uprobe.h"

extern UPROBE	uprobe[];

int	guess_ndev = 0;		/* Guess as to size of nlist table */
#define STRSAVE(str)	(strcpy(malloc((u_int)(strlen(str) + 1)),str))

/*
 * read the device table (/etc/dtab) into internal structures
 * format of lines in the device table are:
 *	device_name unit_number address vector br handler[0-3]	; comment
 *								# comment
 */
read_dtab()
{
	register DTAB	*dp,
			*cdp;
	UPROBE	*up;
	HAND	*sp;
	FILE	*fp;
	int	nhandlers,	/* number of handlers per line */
		line;		/* line number in dtab file */
	short	cnt;		/* general counter */
	char	*cp,		/* traveling char pointer */
		*save,		/* save string position */
		buf[500],	/* line buffer */
		name[20],	/* device name */
		unit[5],	/* unit number */
		*index(), *malloc(), *strcpy(), *fgets();

	if (!(fp = fopen(dtab_name,"r"))) {
		perror(dtab_name);
		exit(AC_SETUP);
	}
	for (line = 1,devs = NULL;fgets(buf, sizeof(buf), fp);++line) {
		if (cp = index(buf, '\n'))
			*cp = EOS;
		else {
			fprintf(stderr,"%s: line %d too long.\n",myname,line);
			exit(AC_SINGLE);
		}
		for (cp = buf;isspace(*cp);++cp);
		if (!*cp || cp == ';' || *cp == '#')
			continue;
		dp = (DTAB *)malloc(sizeof(DTAB));
		if (sscanf(buf," %s %s %o %o %o ",name,unit,&dp->dt_addr,&dp->dt_vector,&dp->dt_br) != 5) {
			fprintf(stderr,"%s: missing information on line %d.\n",myname,line);
			exit(AC_SINGLE);
		}
		dp->dt_name = STRSAVE(name);
		dp->dt_unit = *unit == '?' ? -1 : atoi(unit);
		for (cnt = 0;cnt < 5;++cnt) {
			for (;!isspace(*cp);++cp);
			for (;isspace(*cp);++cp);
		}
		dp->dt_probe = dp->dt_attach = (NLIST *)0;
		dp->dt_handlers = (HAND *)0;
		for (nhandlers = 0;;nhandlers) {
			if (!*cp || *cp == ';' || *cp == '#')
				break;
			if (++nhandlers == 4)
				fprintf(stderr,"%s: warning: more than three handlers for device %s on line %d.\n",myname,dp->dt_name,line);
			for (save = cp;!isspace(*cp);++cp);
			*cp = EOS;
			addent(&dp->dt_handlers,STRSAVE(save));
			for (++cp;isspace(*cp);++cp);
		}
		guess_ndev += nhandlers;
/*
 * In addition to the "handler" symbols for a device we need 3 more
 * symbols: 'xxVec', 'xxprobe', and 'xxattach'.
 *
 * N.B.  If more symbols are added (to the 'DTAB' structure) the following
 *       line may need to be modified.
*/
		guess_ndev += 3;
		for (up = uprobe;up->up_name;++up)
			if (!strcmp(dp->dt_name,up->up_name)) {
				dp->dt_uprobe = up->up_func;
				break;
			}
		dp->dt_next = NULL;
		if (!devs)
			devs = cdp = dp;
		else {
			cdp->dt_next = dp;
			cdp = dp;
		}
	}
}

static
addent(listp, cp)
HAND	**listp;
char	*cp;
{
	HAND	*el,
		*sp;
	char	*malloc();

	el = (HAND *)malloc(sizeof(HAND));
	el->s_str = cp;
	el->s_next = NULL;
	if (!*listp)
		*listp = el;
	else {
		for (sp = *listp;sp->s_next; sp = sp->s_next);
		sp->s_next = el;
	}
}
