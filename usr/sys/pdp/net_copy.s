/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)net_copy.s	1.1 (2.10BSD Berkeley) 4/10/88
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"


/*
 * Fetch and set user byte routines:
 *	fubyte(addr):		fetch user data space byte
 *	subyte(addr, byte):	set user data space byte
 *		caddr_t addr;
 *		u_char byte;
 *
 * The fetch routines return the requested byte or -1 on fault.  The set
 * routines return 0 on success, -1 on failure.
 */
ENTRY(fubyte)
	jsr	pc,fssetup		/ r1 = addr, previous mode = user
	bic	$1,r1			/ r1 = addr&~1
	mfpd	(r1)			/ tmp = user data word at (addr&~1)
	mov	(sp)+,r0
	cmp	r1,6(sp)		/ if (addr&1)
	beq	1f			/   tmp >>= 8
	swab	r0
1:
	bic	$!377,r0		/ return((u_char)tmp)
	br	fscleanup		/ restore fault trap and PS, and return

ENTRY(subyte)
	jsr	pc,fssetup		/ r1 = addr, previous mode = user
	bic	$1,r1			/ r1 = addr&~1
	mfpd	(r1)			/ tmp = user data word at (addr&~1)
	cmp	r1,10(sp)		/ if (addr&1)
	beq	1f
	movb	12(sp),1(sp)		/   *((char *)tmp + 1) = byte
	br	2f
1:					/ else
	movb	12(sp),(sp)		/   *((char *)tmp) = byte
2:
	mtpd	(r1)			/ user data word (addr&~1) = tmp
	clr	r0			/ return success
	br	fscleanup		/ restore fault trap and PS, and return


/*
 * Fetch and set user word routines:
 *	fuword(addr):		fetch user data space word
 *	suword(addr, word):	set user data space word
 *		caddr_t addr;
 *		u_short word;
 *
 * The fetch routines return the requested word or -1 on fault.  The set
 * routines return 0 on success, -1 on failure.  Addr must be even.  The data
 * space routines are really the corresponding instruction space routines if
 * NONSEPARATE is defined.
 */
ENTRY(fuword)
	jsr	pc,fssetup		/ r1 = addr, previous mode = user
	mfpd	(r1)			/ r0 = user data word at addr
	mov	(sp)+,r0
	br	fscleanup		/ restore fault trap and PS, and return

ENTRY(suword)
	jsr	pc,fssetup		/ r1 = addr, previous mode = user
	mov	10(sp),-(sp)		/ user data word at addr = word
	mtpd	(r1)
	clr	r0			/ resturn success
	br	fscleanup		/ restore fault trap and PS, and return


/*
 * Common set up code for the [fs]u(byte|word) routines.  Sets up fault
 * trap, sets previous mode to user, and loads addr into r1.  Leaves old
 * values of PS and nofault on stack.
 */
fssetup:
	mov	(sp)+,r0		/ snag return address
	mov	PS,-(sp)		/ save current PS,
	bic	$30000,PS		/   set previous mode kernel
	mfpd	*$nofault		/ grab current value of nofault
	mov	$fsfault,-(sp)		/ and set new fault trap
	mtpd	*$nofault
	bis	$30000,PS		/ leave previous mode set to user
	mov	6(sp),r1		/ r1 = addr
	jmp	(r0)			/ return to f/s routine

/*
 * Common fault trap for fetch/set user byte/word routines.  Returns -1 to
 * indicate fault.  Stack contains saved fault trap followed by return
 * address.
 */
fsfault:
	mov	$-1,r0			/ return failure (-1)
	/*FALLTHROUGH*/

/*
 * Common clean up code for the [fs]u(byte|word) routines.
 */
fscleanup:
	bic	$30000,PS		/ set previous more to kernel
	mtpd	*$nofault		/ restore fault trap,
	mov	(sp)+,PS		/   PS,
	rts	pc			/   and return


/*
 * copyin(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * Copy length/2 words from user space fromaddr to kernel space address
 * toaddr.  Fromaddr and toaddr must be even.  Returns zero on success,
 * EFAULT on failure.
 */
ENTRY(copyin)
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mfpd	(r1)+			/ do
	mov	(sp)+,(r2)+		/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup


/*
 * copyout(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * Copy length/2 words from kernel space fromaddr to user space address
 * toaddr.  Fromaddr and toaddr must be even.  Returns zero on success,
 * EFAULT on failure.
 */
ENTRY(copyout)
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mov	(r1)+,-(sp)		/ do
	mtpd	(r2)+			/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup


/*
 * Common set up code for the copy(in|out) routines.  Performs zero length
 * check, sets up fault trap, sets previous mode to user, and loads
 * fromaddr, toaddr and length into the registers r1, r2 and r0
 * respectively.  Leaves old values of r2, PS, and nofault on stack.
 */
copysetup:
	mov	(sp)+,r0		/ snag return address
	mov	r2,-(sp)		/ reserve r2 for our use,
	mov	PS,-(sp)		/ save current PS,
	bic	$30000,PS		/   set previous mode kernel
	mfpd	*$nofault		/ grab current value of nofault
	mov	r0,-(sp)		/ push return address back
	mov	$copyfault,-(sp)	/ and set new fault trap
	mtpd	*$nofault
	bis	$30000,PS		/ leave previous mode set to user
	mov	16(sp),r0		/ r0 = (unsigned)length/2
	beq	1f			/   (exit early if length equals zero)
	asr	r0
	bic	$100000,r0
	mov	12(sp),r1		/ r1 = fromaddr
	mov	14(sp),r2		/ r2 = toaddr
	rts	pc

1:
	tst	(sp)+			/ short circuit the copy for zero
	br	copycleanup		/   length returning "success" ...

copyfault:
	mov	$EFAULT,r0		/ we faulted out, return EFAULT
	/*FALLTHROUGH*/

/*
 * Common clean up code for the copy(in|out) routines.  When copy routines
 * finish successfully r0 has already been decremented to zero which is
 * exactly what we want to return for success ...  Tricky, hhmmm?
 */
copycleanup:
	bic	$30000,PS		/ set previous mode to kernel
	mtpd	*$nofault		/ restore fault trap,
	mov	(sp)+,PS		/   PS,
	mov	(sp)+,r2		/   and reserved registers
	rts	pc


/*
 * Kernel/Network copying routines.
 *
 * NOTE:
 *	The cp(to|from)kern functions operate atomically, at high ipl.
 *	This is done mostly out of paranoia.  If the cp(to|from)kern
 *	routines start taking up too much time at high IPL, then this
 *	parnoia should probably be reconsidered.
 *
 *
 * WARNING:
 *	All functions assume that the segments in supervisor space
 *	containing the source or target variables are never remapped.
 */

#ifdef notdef				/* not currently used */
/*
 * void
 * cptokern(nfrom, kto, len)
 *	caddr_t nfrom;		source address in network space
 *	caddr_t kto;		destination address in kernel space
 *	int len;		number of bytes to copy
 *
 * Copy words from the network to the kernel.  Len must be even and both
 * nfrom and kto must begin on an even word boundary.
 */
ENTRY(cptokern)
	mov	r2,-(sp)
	mov	PS,-(sp)
	mov	$40340,PS		/ set previous mode to kernel
	mov	6(sp),r0		/ nfrom
	mov	10(sp),r1		/ kto
	mov	12(sp),r2		/ len
	asr	r2			/ len/2
1:
	mov	(r0)+,-(sp)
	mtpd	(r1)+
	sob	r2,1b

	mov	(sp)+,PS
	mov	(sp)+,r2
	rts	pc
#endif /* notdef */

/*
 * void
 * cpfromkern(kfrom, nto, len)
 *	caddr_t kfrom;		source address in kernel space
 *	caddr_t nto;		destination address in network space
 *	int len;		number of bytes to copy
 *
 * Copy words from the kernel to the network.  Len must be even and both
 * kfrom and nto must begin on an even word boundary.
 */
ENTRY(cpfromkern)
	mov	r2,-(sp)
	mov	PS,-(sp)
	mov	$40340,PS		/ set previous mode to kernel
	mov	6(sp),r0		/ kfrom
	mov	10(sp),r1		/ nto
	mov	12(sp),r2		/ len
	asr	r2			/ len/2
1:
	mfpd	(r0)+
	mov	(sp)+,(r1)+
	sob	r2,1b

	mov	(sp)+,PS
	mov	(sp)+,r2
	rts	pc

/*
 * void
 * mtkd(addr, word)
 *	caddr addr;		destination address in kernel space
 *	int word;		word to store
 *
 * Move To Kernel Data, simplified interface for the network to store
 * single words in the kernel data space.
 */
ENTRY(mtkd)
	mov	2(sp),r0		/ get the destination address
	mov	PS,-(sp)		/ save psw
	bic	$30000,PS		/ previous kernel
	mov	6(sp),-(sp)		/ grab word
	mtpd	(r0)			/   and store it in kernel space
	mov	(sp)+,PS		/ restore psw
	rts	pc			/ return

/*
 * int
 * mfkd(addr)
 *	caddr_t addr;		source address in kernel space
 *
 * Move From Kernel Data, simplified interface for the network to get
 * single words from the kernel data space.
 */
ENTRY(mfkd)
	mov	2(sp),r0		/ get the address of the data
	mov	PS,-(sp)		/ save psw
	bic	$30000,PS		/ previous kernel
	mfpd	(r0)			/ get the word
	mov	(sp)+,r0		/ return value
	mov	(sp)+,PS		/ restore psw
	rts	pc			/ return
