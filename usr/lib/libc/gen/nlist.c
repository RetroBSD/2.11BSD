/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)nlist.c	5.7.1 (2.11BSD GTE) 12/31/93";
#endif

#include <sys/types.h>
#include <sys/file.h>
#include <a.out.h>
#include <stdio.h>

typedef struct nlist NLIST;
#define	_strx	n_un.n_strx
#define	_name	n_un.n_name
#define	ISVALID(p)	(p->_name && p->_name[0])

nlist(name, list)
	char *name;
	NLIST *list;
{
	register NLIST *p, *s;
	struct xexec ebuf;
	FILE *fstr, *fsym;
	NLIST nbuf;
	off_t strings_offset, symbol_offset, symbol_size, lseek();
	int entries, len, maxlen;
	char sbuf[128];

	entries = -1;

	if (!(fsym = fopen(name, "r")))
		return(-1);
	if (fread((char *)&ebuf, 1, sizeof(ebuf), fsym) < sizeof (ebuf.e) ||
	    N_BADMAG(ebuf.e))
		goto done1;

	symbol_offset = N_SYMOFF(ebuf);
	symbol_size = ebuf.e.a_syms;
	strings_offset = N_STROFF(ebuf);
	if (fseek(fsym, symbol_offset, L_SET))
		goto done1;

	if (!(fstr = fopen(name, "r")))
		goto done1;

	/*
	 * clean out any left-over information for all valid entries.
	 * Type and value defined to be 0 if not found; historical
	 * versions cleared other and desc as well.  Also figure out
	 * the largest string length so don't read any more of the
	 * string table than we have to.
	 */
	for (p = list, entries = maxlen = 0; ISVALID(p); ++p, ++entries) {
		p->n_type = 0;
		p->n_ovly = 0;
		p->n_value = 0;
		if ((len = strlen(p->_name)) > maxlen)
			maxlen = len;
	}
	if (++maxlen > sizeof(sbuf)) {		/* for the NULL */
		(void)fprintf(stderr, "nlist: sym 2 big\n");
		entries = -1;
		goto done2;
	}

	for (s = &nbuf; symbol_size; symbol_size -= sizeof(NLIST)) {
		if (fread((char *)s, sizeof(NLIST), 1, fsym) != 1)
			goto done2;
		if (!s->_strx)
			continue;
		if (fseek(fstr, strings_offset + s->_strx, L_SET))
			goto done2;
		(void)fread(sbuf, sizeof(sbuf[0]), maxlen, fstr);
		for (p = list; ISVALID(p); p++)
			if (!strcmp(p->_name, sbuf)) {
				p->n_value = s->n_value;
				p->n_type = s->n_type;
				p->n_ovly = s->n_ovly;
				if (!--entries)
					goto done2;
			}
	}
done2:	(void)fclose(fstr);
done1:	(void)fclose(fsym);
	return(entries);
}
