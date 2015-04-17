/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_xxx.s	1.5 (2.11BSD GTE) 12/15/94
 */
#include "DEFS.h"
#include "../machine/mch_iopage.h"
#include "../machine/koverlay.h"

/*
 * noop()
 *
 * Do nothing.  Typically used to provide enough time for interrupts to
 * happen between a pair of spl's in C.  We use noop rather than inserting
 * meaningless instructions between the spl's to prevent any future C
 * optimizer `improvements' from causing problems.
 *
 * delay(usec)
 *	long	usec;
 *
 * Delay (approximately) usec micro-seconds.  It really isn't very acurrate
 * since we can be interrupted and take much longer than we intended, but
 * that's alright - we just don't want to come home early ...
 */
ENTRY(delay)
	mov	2(sp),r0		/ r0 = hiint(usec)
	mov	4(sp),r1		/ r1 = loint(usec)
	ashc	$1,r0			/ sob's ~= 1/2 micro second,
	beq	2f			/ oops, got passed a delay of 0L-leave
	tst	r1
	/*
	 * If the low int of the loop counter is zero, the double sob loop
	 * below will perform correctly, otherwise the high byte must be
	 * increment.
	 */
	beq	1f
	inc	r0			/ correct for looping
1:
	sob	r1,1b			/ sit on our hands for a while ...
	sob	r0,1b
2:
ENTRY(noop)
	rts	pc

/*
 * idle()
 *
 * Sit and wait for something to happen ...
 */

/*
 * If you have a console display it's amusing to see a slowly rotating 
 * sequence of lights in the display.  If the system is very active the display
 * will appear blurred.
 */
INT(LOCAL, rdisply, 0377)		/ idle pattern
INT(LOCAL, wcount, 2)			/ rotate rdisply every wcount calls

ENTRY(idle)
	mov	PS,-(sp)		/ save current SPL, indicate that no
	mov	$1,_noproc		/   process is running
	dec	wcount			/ if (--wcount <= 0) {
	bgt	1f
	mov	$2,wcount		/   wcount = 2
	clc				/   rdisply <<= 1
	rol	rdisply
	bpl	1f			/   if (``one shifted out'')
	bis	$1,rdisply		/     rdisply |= 1
1:					/ }
	mov	rdisply,r0		/ wait displays contents of r0
	SPLLOW				/ set SPL low so we can be interrupted
	wait				/ wait for something to happen
	mov	(sp)+,PS		/ restore previous SPL
	rts	pc			/   and return

#ifdef PROF
/*
 * These words are to insure that times reported for any following routine do
 * not include those spent while in idle mode when statistics are gathered
 * for system profiling.
 */
	rts	pc
	rts	pc
	rts	pc
#endif


/*
 * setjmp(env)
 *	label_t	*env;
 *
 * longjmp(u, env)
 * resume(u, env)
 *	memaddr	u;
 *	label_t	*env;
 *
 * Setjmp(env) will save the process' current register variable, stack,
 * overlay and program counter context and return a zero.
 *
 * Longjmp(u, env) (and resume) will will generate a "return(1)" from the last
 * call to setjmp(env) by mapping in the user structure pointed to by u,
 * restoring the context saved by setjmp in env and returning a one.  Note that
 * registers are recovered statically from the env buffer rather than
 * dynamically from the stack ...
 *
 * This longjmp differs from the longjmp found in the standard library and the
 * VAX 4.3 kernel - it's actually closer to the resume routine of the 4.3
 * kernel and, indeed, even used to be called resume in the 2.9 kernel.
 * We've given it both names to promote some degree of compatibility between
 * the 4.3 and 2.10 C kernel source ...
 */
ENTRY(setjmp)
	mov	(sp)+,r1		/ save return address
	mov	(sp),r0			/ r0 = env
	mov	r2,(r0)+		/ save register variables r2 - r4
	mov	r3,(r0)+		/   in env ...
	mov	r4,(r0)+
	mov	r5,(r0)+		/   frame pointer,
	mov	sp,(r0)+		/   stack pointer,
#ifdef INET
	mov	PS,-(sp)		/   network stack pointer,
	mov	$010340,PS
	mfpd	sp
#ifdef CHECKSTACK
	cmp	(sp),$NET_STOP		/   (check network stack pointer to
	bhi	1f			/     make sure it's in the network
	cmp	(sp),$NET_SBASE		/     stack ...)
	bhi	2f
1:
	halt
2:
#endif
	mov	(sp)+,(r0)+
	mov	(sp)+,PS
#endif
	mov	__ovno,(r0)+		/   overlay number,
	mov	r1,(r0)+		/   and return address
	clr	r0			/ return a zero for the setjmp call
	jmp	(r1)

ENTRY(longjmp)
ENTRY(resume)
	mov	2(sp),r0		/ r0 = u
	mov	4(sp),r1		/ r1 = env
	SPL7				/ can't let anything in till we
					/   (at least) get a valid stack ...
	mov	r0,KDSA6		/ map new process' u structure in
#ifdef INET
	mov	r0,SDSA6		/ map supervisor stack area to same
#endif
	mov	(r1)+,r2		/ restore register variables
	mov	(r1)+,r3		/   from env ...
	mov	(r1)+,r4
	mov	(r1)+,r5		/   frame pointer,
	mov	(r1)+,sp		/   stack pointer,
#ifdef INET
	mov	PS,-(sp)		/   network stack pointer,
	mov	$010340,PS
	mov	(r1)+,-(sp)
	mtpd	sp
	mov	(sp)+,PS
#endif
	mov	(r1)+,r0		/ grab return overlay number ...
	cmp	r0,__ovno		/ old overlay currently mapped in?
	beq	1f
	mov	r0,__ovno		/ nope, set new overlay number
	asl	r0			/ compute descriptor index and map
	mov	ova(r0),OVLY_PAR	/   the old overlay back in ...
	mov	ovd(r0),OVLY_PDR
1:
	mov	$1001,SSR0		/ J-11 bug, force MMU registers to start
					/   tracking again between processes
	SPLLOW				/ release interrupts and transfer back
	mov	$1,r0			/   to setjmp return with a return
	jmp	*(r1)+			/   value of 1

/*
 * struct uprof {			/ profile arguments
 *	short	*pr_base;		/ buffer base
 *	unsigned pr_size;		/ buffer size
 *	unsigned pr_off;		/ pc offset
 *	unsigned pr_scale;		/ pc scaling
 * } u_prof;
 *
 * addupc(pc, pbuf, ticks)
 *	caddr_t		pc;
 *	struct uprof	*pbuf;
 *	int		ticks;
 *
 * Addupc implements the profil(2) facility:
 *
 *	b = (pc - pbuf->pr_off)>>1;
 *	b *= pbuf->pr_scale>>1;
 *	b >>= 14; { 2^14 = 2^16/2/2 - because of the two `>>'s above }
 *	if (b < pbuf->pr_size) {
 *		b += pbuf->pr_base;
 *		if (fuword(b, &w) < 0  ||  suword(b, w) < 0)
 *			pbuf->pr_scale = 0;	{ turn off profiling }
 *	}
 */
ENTRY(addupc)
	mov	r2,-(sp)		/ save register so we can use it
	mov	6(sp),r2		/ r2 = pbuf
	mov	4(sp),r0		/ r0 = pc
	sub	4(r2),r0		/ r0 -= pbuf->pr_off
	clc				/ r0 >>= 1 { ensure high bit 0 }
	ror	r0
	mov	6(r2),r1		/ r1 = pbuf->pr_scale
	clc				/ r1 >>= 1 { ensure high bit 0 }
	ror	r1
	mul	r1,r0			/ r0:r1 = r0 * (pbuf->pr_scale>>1)
	ashc	$-14.,r0		/ r0:r1 >>= 14
	inc	r1			/ *round* r1 to a word offset
	bic	$1,r1
	cmp	r1,2(r2)		/ if r1 > pbuf->pr_size
	bhis	3f			/   bug out ...
	add	(r2),r1			/ r1 += pbuf->pr_base
	mov	nofault,-(sp)		/ set up for possible memory fault when
	mov	$1f,nofault		/   access pbuf->pr_base[r1] : branch
					/   to 1f on fault
	mfpd	(r1)			/ pbuf->pr_base[r1] += ticks
	add	12.(sp),(sp)
	mtpd	(r1)
	br	2f			/ (branch around fault code)
1:					/ on fault: disable profiling
	clr	6(r2)			/   (pbuf->pr_scale = 0)
2:
	mov	(sp)+,nofault		/ reset fault branch
3:
	mov	(sp)+,r2		/ restore saved registers
	rts	pc			/   and return

#ifndef ENABLE34
/*
 * fioword(addr)
 *	caddr_t addr;
 *
 * Fetch a word from an address on the I/O page,
 * returning -1 if address does not exist.
 */
ENTRY(fioword)
	mov	nofault,-(sp)
	mov	$2f,nofault
	mov	*4(sp),r0
1:
	mov	(sp)+,nofault
	rts	pc
2:
	mov	$-1,r0
	br	1b
#endif

/*
 * error = copystr(fromaddr, toaddr, maxlength, lencopied)
 *	int	error;
 *	caddr_t	fromaddr, toaddr;
 *	u_int	maxlength, *lencopied; 
 *
 * Copy a null terminated string from one point to another in the kernel
 * address space.  Returns zero on success, ENOENT if maxlength exceeded.  If
 * lencopied is non-zero, *lencopied gets the length of the copy (including
 * the null terminating byte).
 */
ENTRY(copystr)
	mov	r2,-(sp)		/ need an extra register
	mov	4.(sp),r0		/ r0 = fromaddr
	mov	6.(sp),r1		/ r1 = toaddr
	mov	8.(sp),r2		/ r2 = maxlength (remaining space)
	beq	2f			/ (exit early with ENOENT if 0)
1:
	movb	(r0)+,(r1)+		/ move a byte
	beq	3f			/ (done when we cross the null)
	sob	r2,1b			/ and loop as long as there's room
2:
	mov	$ENOENT,r0		/ ran out of room - indicate failure
	br	4f			/   and exit ...
3:
	clr	r0			/ success!
4:
	tst	10.(sp)			/ does the caller want the copy length?
	beq	5f
	sub	6.(sp),r1		/ yes, figure out how much we copied:
	mov	r1,*10.(sp)		/ *lencopied = r1 {toaddr'} - toaddr
5:
	mov	(sp)+,r2		/ restore registers
	rts	pc			/   and return


/*
 * Zero the core associated with a buffer.  Since this routine calls mapin
 * without saving the current map, it cannot be called from interrupt routines.
 */
ENTRY(clrbuf)
	mov	2(sp),-(sp)		/ pass bp to mapin
	jsr	pc,_mapin		/ r0 = buffer pointer
	tst	(sp)+

	tst	_fpp			/ do we have floating point hardware?
	beq	2f			/ nope, use regular clr instructions

	stfps	-(sp)			/ save old floating point status
	setd				/ use double precision
	mov	$MAXBSIZE\/32.,r1	/ clear 32 bytes per loop
1:
	clrf	(r0)+
	clrf	(r0)+
	clrf	(r0)+
	clrf	(r0)+
	sob	r1,1b

	ldfps	(sp)+			/ restore floating point status
	br	4f
2:
	mov	$MAXBSIZE\/8.,r1	/ clear 8 bytes per loop
3:
	clr	(r0)+
	clr	(r0)+
	clr	(r0)+
	clr	(r0)+
	sob	r1,1b
4:
#ifdef DIAGNOSTIC
	jmp	_mapout			/ map out buffer

#else

	mov	_seg5+SE_DESC,KDSD5	/ normalseg5();
	mov	_seg5+SE_ADDR,KDSA5
	rts	pc
#endif


#ifdef DIAGNOSTIC
SPACE(GLOBAL, _hasmap, 2)		/ (struct bp *): SEG5 mapped
#endif

/*
 * caddr_t
 * mapin(bp)
 *	struct buf *bp;
 *
 * Map in an out-of-address space buffer.  If this is done
 * from interrupt level, the previous map must be saved before
 * mapin, and restored after mapout; e.g.
 *	segm save;
 *	saveseg5(save);
 *	mapin(bp);
 *	...
 *	mapout(bp);
 *	restorseg5(save);
 *
 *	caddr_t
 *	mapin(bp)
 *		register struct buf *bp;
 *	{
 *		register u_int paddr;
 *		register u_int offset;
 *
 *	#ifdef DIAGNOSTIC
 *		if (hasmap) {
 *			printf("mapping %o over %o\n", bp, hasmap);
 *			panic("mapin");
 *		}
 *		hasmap = bp;
 *	#endif
 *		offset = (u_int)bp->b_un.b_addr & 077;
 *		paddr = bftopaddr(bp);
 *		mapseg5((u_short)paddr,
 *		    (u_short)(((u_int)DEV_BSIZE << 2) | (u_int)RW));
 *		return(SEG5 + offset);
 *	}
 */
ENTRY(mapin)
	mov	2(sp),r0		/ r0 = bp
#ifdef DIAGNOSTIC
	tst	_hasmap			/ is buffer already mapped in??
	beq	9f
	mov	_hasmap,-(sp)		/ oops ... print out a message and die
	mov	r0,-(sp)		/   with a panic
	mov	$1f,-(sp)
	STRING(LOCAL, 1, <mapping %o over %o\n\0>)
	jsr	pc,_printf
	cmp	(sp)+,(sp)+
	mov	$1f,(sp)
	STRING(LOCAL, 1, <mapin\0>)
	jsr	pc,_panic
	/*NOTREACHED*/
9:
	mov	r0,_hasmap		/ save mapin buffer address
#endif
	mov	B_ADDR(r0),r1		/ r1 = bp->b_addr
	movb	B_XMEM(r0),r0		/ r0 = bp->b_xmem
	mov	r1,-(sp)		/ for later...
	ashc	$-6,r0			/ r0:r1 = bftopaddr(bp)
	mov	r1,KDSA5		/ mapseg5((u_short)r0:r1,
	mov	$DEV_BSIZE\<2|RW,KDSD5	/   (DEV_BSIZE << 2) | RW)
	mov	(sp)+,r0
	bic	$!077,r0		/ return(SEG5 + (bp->b_un.b_addr&077))
	add	$0120000,r0
	rts	pc


#ifdef DIAGNOSTIC
/*
 * Map out buffer pointed to by bp and restore previous mapping.
 * Mapout is handled by a macro in seg.h if DIAGNOSTIC isn't defined.
 *
 *	#ifdef DIAGNOSTIC
 *	void
 *	mapout(bp)
 *		struct buf *bp;
 *	{
 *		if (bp != hasmap) {
 *			printf("unmapping %o, not %o\n", bp, hasmap);
 *			panic("mapout");
 *		}
 *		hasmap = NULL;
 *	
 *	normalseg5();
 *	}
 *	#endif
 */
ENTRY(mapout)
	cmp	2(sp),_hasmap		/ mapping out same buffer that was
	beq	9f			/   mapped in?
	mov	_hasmap,-(sp)		/ not good ... print out a message
	mov	4(sp),-(sp)		/   and die
	mov	$1f,-(sp)
	STRING(LOCAL, 1, <unmapping %o; not %o\n\0>)
	jsr	pc,_printf
	cmp	(sp)+,(sp)+
	mov	$1f,(sp)
	STRING(LOCAL, 1, <mapout\0>)
	jsr	pc,_panic
	/*NOTREACHED*/
9:
	clr	_hasmap			/ indicate mapping clear
	mov	_seg5+SE_DESC,KDSD5	/ normalseg5();
	mov	_seg5+SE_ADDR,KDSA5
	rts	pc
#endif


/*
 * Save current SEG5 and SEG6 mapping in map and setup normal mapping.
 *
 * 	#define	KD6	(((USIZE-1)<<8) | RW)	/ proto descriptor for u.
 *	savemap(map)
 *		register mapinfo map;
 *	{
 *		map[0].se_desc = *KDSD5;
 *		map[0].se_addr = *KDSA5;
 *		map[1].se_desc = *KDSD6;
 *		map[1].se_addr = *KDSA6;
 *		if (kdsa6) {
 *			*KDSD6 = KD6;
 *			*KDSA6 = kdsa6;
 *		}
 *		normalseg5(seg5);
 *	}
 */
ENTRY(savemap)
	mov	2(sp),r0		/ r0 = map
	mov	KDSD5,(r0)+		/ map[0].se_desc = *KDSD5
	mov	KDSA5,(r0)+		/ map[0].se_addr = *KDSA5
	mov	KDSD6,(r0)+		/ map[1].se_desc = *KDSD6
	mov	KDSA6,(r0)		/ map[1].se_addr = *KDSA6
	tst	_kdsa6			/ SEG mapped out??
	beq	9f
	mov	$USIZE-1\<8|RW,KDSD6	/ yep, map it in, *KDSD6 = (USIZE, RW)
	mov	_kdsa6,KDSA6		/   *KDSA6 = kdsa6
9:
	mov	_seg5+SE_DESC,KDSD5	/ normalseg5();
	mov	_seg5+SE_ADDR,KDSA5
	rts	pc

/*
 * Restore SEG5 and SEG6 mapping from map.
 *
 *	restormap(map)
 *		register mapinfo map;
 *	{
 *		*KDSD5 = map[0].se_desc;
 *		*KDSA5 = map[0].se_addr;
 *		*KDSD6 = map[1].se_desc;
 *		*KDSA6 = map[1].se_addr;
 *	}
 */
ENTRY(restormap)
	mov	2(sp),r0		/ r0 = map
	mov	(r0)+,KDSD5		/ *KDSD5 = map[0].se_desc
	mov	(r0)+,KDSA5		/ *KDSA5 = map[0].se_addr
	mov	(r0)+,KDSD6		/ *KDSD6 = map[1].se_desc
	mov	(r0),KDSA6		/ *KDSA6 = map[1].se_addr
	rts	pc

/*
 * savfp(fps)
 *	struct fps	*fps;
 *
 * Save current floating point processor state: floating point status register,
 * and all six floating point registers.
 */
ENTRY(savfp)
	tst	_fpp			/ do we really have floating point
	beq	1f			/   hardware??

	mov	2(sp),r1		/ r1 = fps
	stfps	(r1)+			/ save floating point status register
	setd				/ (always save registers as double)
	movf	fr0,(r1)+		/ and save floating point registers
	movf	fr1,(r1)+
	movf	fr2,(r1)+
	movf	fr3,(r1)+
	movf	fr4,fr0
	movf	fr0,(r1)+
	movf	fr5,fr0
	movf	fr0,(r1)+
1:
	rts	pc

/*
 * restfp(fps)
 *	struct fps	*fps;
 *
 * Restore floating point processor state.
 */
ENTRY(restfp)
	tst	_fpp			/ do we really have floating point
	beq	1f			/   hardware??

	mov	2(sp),r1		/ r0 = r1 = fps
	mov	r1,r0
	setd				/ (we saved the registers as double)
	add	$8.+2.,r1		/ skip fpsr and fr0 for now
	movf	(r1)+,fr1		/ restore fr1 thru fr3
	movf	(r1)+,fr2
	movf	(r1)+,fr3
	movf	(r1)+,fr0		/ grab and restore saved fr4 and fr5
	movf	fr0,fr4
	movf	(r1)+,fr0
	movf	fr0,fr5
	movf	2(r0),fr0		/ restore fr0
	ldfps	(r0)			/   and floating point status register
1:
	rts	pc

/*
 * stst(fperr)
 *	struct fperr	*fperr;
 *
 * Save floating point error registers.  The argument is a pointer to a two
 * word structure as defined in <pdp/fperr>.
 */
ENTRY(stst)
	tst	_fpp			/ do we really have floating point
	beq	1f			/   hardware??
	stst	*2(sp)			/ simple, no?
1:
	rts	pc

/*
 * scanc(size, str, table, mask)
 * 	u_int size;
 * 	u_char *str, table[];
 * 	u_char mask;
 *
 * Scan through str up to (but not including str[size]) stopping when a
 * character who's entry in table has mask bits set.  Return number of
 * characters left in str.
 */
ENTRY(scanc)
	mov	2(sp),r0		/ r0 = size
	beq	3f			/   exit early if zero
	mov	4(sp),r1		/ r1 = str
	mov	r2,-(sp)		/ r2 = table
	mov	6+2(sp),r2
	mov	r3,-(sp)		/ r3 = mask
	mov	10+4(sp),r3
	mov	r4,-(sp)		/ r4 is temporary
1:					/ do
	clr	r4			/   if (table[*str++] & mask)
	bisb	(r1)+,r4
	add	r2,r4
	bitb	r3,(r4)
	bne	2f			/     break;
	sob	r0,1b			/ while (--size != 0)
2:
	mov	(sp)+,r4		/ restore registers
	mov	(sp)+,r3
	mov	(sp)+,r2
3:
	rts	pc			/ and return size


/*
 * locc(mask, size, str)
 * 	u_char mask;
 * 	u_int size;
 * 	u_char *str;
 *
 * Scan through str up to (but not including str[size]) stopping when a
 * character equals mask.  Return number of characters left in str.
 */
ENTRY(locc)
	mov	4(sp),r0		/ r0 = size
	beq	3f			/   exit early if zero
	mov	6(sp),r1		/ r1 = str
	mov	r2,-(sp)		/ r2 = mask
	mov	2+2(sp),r2
1:					/ do
	cmpb	(r1)+,r2		/   if (*str++ == mask)
	beq	2f
	sob	r0,1b			/ while (--size != 0)
2:
	mov	(sp)+,r2		/ restore registers
3:
	rts	pc			/ and return size

/*
 * nextiv()
 *
 * Decrement _lastiv by size of a vector (4) and return the new value.
 * Placed here for centralized access and easy calling from the networking
 * (via SKcall) and 'autoconfig' (via ucall).
*/
ENTRY(nextiv)
	sub	$4,_lastiv		/ adjust last interrupt vector
	mov	_lastiv,r0		/ put in right place for return value
	rts	pc			/ return assigned vector

/*
 * vattr_null()
 *
 * Initialize a inode attribute structure.  See the comments in h/inode.h
 * for more details.  If the vnode/inode attribute structure (which is a
 * subset of 4.4's) changes then this routine must change also.  The 'sxt'
 * sequences below are shorter/faster than "mov $VNOVAL,..." since VNOVAL
 * is -1.
*/
ENTRY(vattr_null)
	mov	2(sp),r0		/ get address of vattr structure
	mov	$-1,(r0)+		/ va_mode = VNOVAL
	sxt	(r0)+			/ va_uid = VNOVAL
	sxt	(r0)+			/ va_gid = VNOVAL
	sxt	(r0)+			/ va_size - hi = VNOVAL
	sxt	(r0)+			/ va_size - lo = VNOVAL
	sxt	(r0)+			/ va_atime - hi = VNOVAL
	sxt	(r0)+			/ va_atime - lo = VNOVAL
	sxt	(r0)+			/ va_mtime - hi = VNOVAL
	sxt	(r0)+			/ va_mtime - lo = VNOVAL
	sxt	(r0)+			/ va_flags = VNOVAL
	clr	(r0)+			/ va_vaflags = 0
	rts	pc
