/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)in_cksum.c	1.1 (2.10BSD Berkeley) 12/1/86
 */
#include "param.h"
#include "../machine/seg.h"

#include "mbuf.h"
#include "domain.h"
#include "protosw.h"
#include <netinet/in.h>
#include <netinet/in_systm.h>

/*
 * Checksum routine for Internet Protocol family headers.  This routine is
 * very heavily used in the network code and should be rewritten for each
 * CPU to be as fast as possible.
 */

#ifdef DIAGNOSTIC
int	in_ckprint = 0;			/* print sums */
#endif

in_cksum(m, len)
	struct mbuf *m;
	int len;
{
	register u_char *w;		/* known to be r4 */
	register u_int mlen = 0;	/* known to be r3 */
	register u_int sum = 0;		/* known to be r2 */
	u_int plen = 0;

#ifdef DIAGNOSTIC
	if (in_ckprint)
		printf("ck m%o l%o", m, len);
#endif
	for (;;) {
		/*
		 * Each trip around loop adds in
		 * words from one mbuf segment.
		 */
		w = mtod(m, u_char *);
		if (plen & 01) {
			/*
			 * The last segment was an odd length, add the high
			 * order byte into the checksum.
			 */
			sum = in_ckadd(sum,(*w++ << 8));
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;
		m = m->m_next;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		plen = mlen;
		if (mlen > 0)
			in_ckbuf();	/* arguments already in registers */
		if (len == 0)
			break;
		/*
		 * Locate the next block with some data.
		 */
		for (;;) {
			if (m == 0) {
				printf("cksum: out of data\n");
				goto done;
			}
			if (m->m_len)
				break;
			m = m->m_next;
		}
	}
done:
#ifdef DIAGNOSTIC
	if (in_ckprint)
		printf(" s%o\n", ~sum);
#endif
	return(~sum);
}
