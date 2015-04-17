/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)net_mbuf.s	2.0 (2.11BSD GTE) 3/13/93
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"

/*
 * mbcopyin(click, off, ptr, len)
 *	u_short click;		/ address of external DMA buffer
 *	u_short off;		/ offset into click of start of buffer
 *	caddr_t ptr;		/ address of internal buffer
 *	short len;		/ bytes to copy from click/off to ptr
 *
 * mbcopyout(ptr, click, off, len)
 *	caddr_t ptr;		/ address of internal buffer
 *	u_short click;		/ address of external DMA buffer
 *	u_short off;		/ offset into click of start of buffer
 *	short len;		/ bytes to copy from ptr to click/off
 *
 * Used for copying data from/to the UNIBUS DMA pages to/from the
 * in-address-space mbufs.  Cannot copy data to or from the stack.  Len
 * cannot exceed 8Kbytes.
 *
 * We remap SDSA6 to do the copy and run at SPL7.
 */

ENTRY(mbcopyin)
	jsr	r5,csv
	mov	012(r5),r0		/ grab byte count
	ble	3f			/ nothing to do if len <= 0 ...
	mov	06(r5),r1		/ grab offset and computer pointer
	add	$140000,r1		/   to mapped in source data
	mov	010(r5),r2		/ grab ptr (destination address)
	mov	PS,-(sp)		/ save PS since we lock out interrupts
	SPL7
	mov	SDSA6,r3		/ save mapping for stack / u. area
	mov	04(r5),SDSA6		/ map in source data
	br	0f

ENTRY(mbcopyout)
	jsr	r5,csv
	mov	012(r5),r0		/ grab byte count
	ble	3f			/ nothing to do if len <= 0 ...
	mov	04(r5),r1		/ grab ptr (source address)
	mov	010(r5),r2		/ grab offset and compute pointer
	add	$140000,r2		/   to mapped destination address
	mov	PS,-(sp)		/ save PS since we lock out interrupts
	SPL7
	mov	SDSA6,r3		/ save mapping for stack / u. area
	mov	06(r5),SDSA6		/ map in destination address data

/*
 * Common loop and return for mbcopyin and mbcopyout.  Copy from (r1) to
 * (r2) for r0 bytes.  r3 contains old seg6 address descriptor.
 */
0:
	cmp	r0,$8192.	/ too big?
	bge	2f		/   yes, ignore it
	cmp	r0,$10.		/ if (length >= 10)
	bhi	4f		/	try words
1:
	movb	(r1)+,(r2)+	/ do  *dst++ = *src++
	sob	r0,1b		/ while (--length)
2:
	mov	r3,SDSA6	/ map back in stack / u. area
	mov	(sp)+,PS	/ restore original priority
3:
	jmp	cret
/*
 * The length of the copy justifies trying a word by word copy.  If src and
 * dst are of the same parity, we can do a word copy by handling any leading
 * and trailing odd bytes separately.  NOTE:  r4 is used to hold the odd byte
 * indicator because the stack is invalid (mapped out).
 */
4:
	bit	$1,r1		/ if (src&1 != dst&1)
	beq	5f		/	do bytes
	bit	$1,r2
	beq	1b		/ (src odd, dst even - do bytes)
	movb	(r1)+,(r2)+	/ copy leading odd byte
	dec	r0
	br	6f
5:
	bit	$1,r2
	bne	1b		/ (src even, dst odd - do bytes)
6:
	mov	r0,r4		/ save trailing byte indicator
	asr	r0		/ length >>= 1 (unsigned)
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	7f		/	handle leading non multiple of four
	mov	(r1)+,(r2)+
7:
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	8f		/	handle leading non multiple of eight
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
8:
	mov	(r1)+,(r2)+	/ do
	mov	(r1)+,(r2)+	/	move eight bytes
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
	sob	r0,8b		/ while (--length)
	asr	r4		/ if (odd trailing byte)
	bcc	2b
	movb	(r1)+,(r2)+	/	copy it
	br	2b		/ and return
