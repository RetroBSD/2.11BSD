/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)pass4.c	5.3 (Berkeley) 3/5/86";
#endif not lint

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "fsck.h"

int	pass4check();

pass4()
{
	register ino_t inumber, *zp;
	struct inodesc idesc;
	int n;

	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	for (inumber = ROOTINO; inumber <= lastino; inumber++) {
		idesc.id_number = inumber;
		switch (getstate(inumber)) {

		case FSTATE:
		case DFOUND:
			n = getlncnt(inumber);
			if (n)
				adjust(&idesc, (short)n);
			else {
				zp = zlnlist;
				for (zp = zlnlist; zp < zlnp; zp++) {
					if (*zp == 0) continue;
					if (*zp == inumber) {
						clri(&idesc, "UNREF", 1);
						*zp = 0;
						break;
					}
				}
			}
			break;

		case DSTATE:
			clri(&idesc, "UNREF", 1);
			break;

		case DCLEAR:
		case FCLEAR:
			clri(&idesc, "BAD/DUP", 1);
			break;

		case USTATE:
			break;

		default:
			errexit("BAD STATE %d FOR INODE I=%u",
			    getstate(inumber), inumber);
		}
	}
}

pass4check(idesc)
	register struct inodesc *idesc;
{
	register daddr_t *dlp;
	daddr_t blkno = idesc->id_blkno;

	if (outrange(blkno))
		return (SKIP);
	if (getbmap(blkno)) {
		for (dlp = duplist; dlp < enddup; dlp++)
			if (*dlp == blkno) {
				*dlp = *--enddup;
				return (KEEPON);
			}
		clrbmap(blkno);
		n_blks--;
	}
	return (KEEPON);
}
