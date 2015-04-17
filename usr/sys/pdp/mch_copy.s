/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_copy.s	1.3 (2.11BSD GTE) 1/9/94
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"


/*
 * Fetch and set user byte routines:
 *	fubyte(addr):		fetch user data space byte
 *	fuibyte(addr):		fetch user instruction space byte
 *	subyte(addr, byte):	set user data space byte
 *	suibyte(addr, byte):	set user instruction space byte
 *		caddr_t addr;
 *		u_char byte;
 *
 * The fetch routines return the requested byte or -1 on fault.  The set
 * routines return 0 on success, -1 on failure.  The data space routines are
 * really the corresponding instruction space routines if NONSEPARATE is
 * defined.
 */
ENTRY(fubyte)
#ifndef NONSEPARATE
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1
	bic	$1,r1			/ r1 = addr&~1
	mfpd	(r1)			/ tmp = user data word at (addr&~1)
	mov	(sp)+,r0
	cmp	r1,4(sp)		/ if (addr&1)
	beq	1f			/   tmp >>= 8
	swab	r0
1:
	bic	$!377,r0		/ return((u_char)tmp)
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc
#endif !NONSEPARATE

ENTRY(fuibyte)
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1
	bic	$1,r1			/ r1 = addr&~1
	mfpi	(r1)			/ tmp = user instruction word at
	mov	(sp)+,r0		/   (addr&~1)
	cmp	r1,4(sp)		/ if (addr&1)
	beq	1f			/   tmp >>= 8
	swab	r0
1:
	bic	$!377,r0		/ return((u_char)tmp)
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc

ENTRY(subyte)
#ifndef NONSEPARATE
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1
	bic	$1,r1			/ r1 = addr&~1
	mfpd	(r1)			/ tmp = user data word at (addr&~1)
	cmp	r1,6(sp)		/ if (addr&1)
	beq	1f
	movb	10(sp),1(sp)		/   *((char *)tmp + 1) = byte
	br	2f
1:					/ else
	movb	10(sp),(sp)		/   *((char *)tmp) = byte
2:
	mtpd	(r1)			/ user data word (addr&~1) = tmp
	clr	r0			/ return success
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc
#endif !NONSEPARATE

ENTRY(suibyte)
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1
	bic	$1,r1			/ r1 = addr&~1
	mfpi	(r1)			/ tmp = user instruction word at
	cmp	r1,6(sp)		/   (addr&~1)
	beq	1f			/ if (addr&1)
	movb	10(sp),1(sp)		/   *((char *)tmp + 1) = byte
	br	2f
1:					/ else
	movb	10(sp),(sp)		/   *((char *)tmp) = byte
2:
	mtpi	(r1)			/ user instruction word (addr&~1) = tmp
	clr	r0			/ return success
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc


/*
 * Fetch and set user word routines:
 *	fuiword(addr):		fetch user instruction space word
 *	fuword(addr):		fetch user data space word
 *	suiword(addr, word):	set user instruction space word
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
#ifndef NONSEPARATE
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1		/ r1 = addr
	mfpd	(r1)			/ r0 = user data word at addr
	mov	(sp)+,r0
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc
#endif !NONSEPARATE

ENTRY(fuiword)
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1		/ r1 = addr
	mfpi	(r1)			/ r0 = user instruction word at addr
	mov	(sp)+,r0
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc

ENTRY(suword)
#ifndef NONSEPARATE
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1		/ r1 = addr
	mov	6(sp),-(sp)		/ user data word at addr = word
	mtpd	(r1)
	clr	r0			/ resturn success
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc
#endif !NONSEPARATE

ENTRY(suiword)
	mov	nofault,-(sp)		/ set fault trap
	mov	$fsfault,nofault
	mov	4(sp),r1		/ r1 = adddr
	mov	6(sp),-(sp)		/ user instruction word at addr = word
	mtpi	(r1)
	clr	r0			/ return success
	mov	(sp)+,nofault		/ restore fault trap, and return
	rts	pc


/*
 * Common fault trap for fetch/set user byte/word routines.  Returns -1 to
 * indicate fault.  Stack contains saved fault trap followed by return
 * address.
 */
fsfault:
	mov	(sp)+,nofault		/ restore fault trap,
	mov	$-1,r0			/ return failure (-1)
	rts	pc


/*
 * copyin(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * copyiin(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * Copy length/2 words from user space fromaddr to kernel space address
 * toaddr.  Fromaddr and toaddr must be even.  Returns zero on success,
 * EFAULT on failure.  Copyin copies from data space, copyiin from
 * instruction space.
 */
ENTRY(copyin)
#ifndef NONSEPARATE
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mfpd	(r1)+			/ do
	mov	(sp)+,(r2)+		/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup
#endif !NONSEPARATE

ENTRY(copyiin)
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mfpi	(r1)+			/ do
	mov	(sp)+,(r2)+		/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup


/*
 * copyout(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * copyiout(fromaddr, toaddr, length)
 *	caddr_t fromaddr, toaddr;
 *	u_int length;
 *
 * Copy length/2 words from kernel space fromaddr to user space address
 * toaddr.  Fromaddr and toaddr must be even.  Returns zero on success,
 * EFAULT on failure.  Copyout copies to data space, copyiout to
 * instruction space.
 */
ENTRY(copyout)
#ifndef NONSEPARATE
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mov	(r1)+,-(sp)		/ do
	mtpd	(r2)+			/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup
#endif !NONSEPARATE

ENTRY(copyiout)
	jsr	pc,copysetup		/ r1 = fromaddr, r2 = toaddr,
1:					/ r0 = length/2
	mov	(r1)+,-(sp)		/ do
	mtpi	(r2)+			/   *toaddr++ = *fromaddr++
	sob	r0,1b			/ while (--length)
	br	copycleanup


/*
 * Common set up code for the copy(in|out) routines.  Performs zero length
 * check, set up fault trap, and loads fromaddr, toaddr and length into the
 * registers r1, r2 and r0 respectively.  Leaves old values of r2 and
 * nofault on stack.
 */
copysetup:
	mov	(sp)+,r0		/ snag return address
	mov	r2,-(sp)		/ reserve r2 for our use,
	mov	nofault,-(sp)		/   save nofault so we can set our own
	mov	r0,-(sp)		/   trap and push return address back
	mov	$copyfault,nofault
	mov	14(sp),r0		/ r0 = (unsigned)length/2
	beq	1f			/   (exit early if length equals zero)
	asr	r0
	bic	$100000,r0
	mov	10(sp),r1		/ r1 = fromaddr
	mov	12(sp),r2		/ r2 = toaddr
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
	mov	(sp)+,nofault		/ restore fault trap,
	mov	(sp)+,r2		/   and reserved registers
	rts	pc


#ifdef INET
/*
 * Kernel/Network copying routines.
 *
 * NOTE:
 *	The m[ft]sd functions operate at high ipl.  This is done mostly
 *	because it's simpler to do a ``mov $10340,PS'' than ``bic $30000,PS;
 *	bis $10000,PS''.  But these functions will never take up enough time
 *	to cause anyone any problems.
 *
 * WARNING:
 *	All functions assume that the segments in supervisor space
 *	containing the source or target variables are never remapped.
 *
 * void
 * mtsd(addr, word)
 *	caddr_t addr;		destination address in supervisor space
 *	int word		word to store
 *
 * Move To Supervisor Data, simplified interface for the kernel to store
 * single words in the supervisor data space.
 */
ENTRY(mtsd)
	mov	2(sp),r0		/ get the destination address
	mov	PS,-(sp)		/ save psw
	mov	$10340,PS		/ previous supervisor
	mov	6(sp),-(sp)		/ grab word
	mtpd	(r0)			/   and store it in supervisor space
	mov	(sp)+,PS		/ restore psw
	rts	pc			/ return

/*
 * int
 * mfsd(addr)
 *	caddr_t addr;		source address in supervisor space
 *
 * Move From Supervisor Data, simplified interface for the kernel to get
 * single words from the supervisor data space.
 */
ENTRY(mfsd)
	mov	2(sp),r0		/ get the address of the data
	mov	PS,-(sp)		/ save psw
	mov	$10340,PS		/ previous supervisor
	mfpd	(r0)			/ get the word
	mov	(sp)+,r0		/ return value
	mov	(sp)+,PS		/ restore psw
	rts	pc			/ return
#endif


/*
 * error = vcopyin(fromaddr, toaddr, length)
 *	int	error;
 *	caddr_t	fromaddr, toaddr;
 *	u_int	length;
 *
 * Copy length bytes from user address space fromaddr to kernel space toaddr.
 * Returns zero on success, EFAULT on failure.  Vcopyin is only called when
 * fromaddr, toaddr or length is odd and the length doesn't justify an
 * fmove.
 */
ENTRY(vcopyin)
	mov	r2,-(sp)		/ allocate a couple registers
	mov	r3,-(sp)
	mov	nofault,-(sp)
	mov	$5f,nofault		/ set up error trap
	mov	10(sp),r1		/ r1 = fromaddr (user address)
	mov	12(sp),r2		/ r2 = toaddr (kernel address)
	mov	14(sp),r0		/ r0 = length
	beq	4f			/ (exit early if 0)
	bit	$1,r1			/ fromaddr odd?
	beq	1f
	dec	r1			/ yes, grab the even word and snarf
	mfpd	(r1)+			/   the high byte to start us off
	swab	(sp)
	movb	(sp)+,(r2)+
	dec	r0
1:
	mov	r0,r3			/ save trailing byte indicator and
	asr	r0			/ convert length remaining to units of
	beq	3f			/   words
2:
	mfpd	(r1)+			/ grab next word from user space
	movb	(sp),(r2)+		/ move the first byte
	swab	(sp)			/   and the second ...
	movb	(sp)+,(r2)+
	sob	r0,2b
3:					/ r0 = 0
	asr	r3			/ need to copy in trailing byte?
	bcc	4f			/   nope, all done
	mfpd	(r1)			/ grab last word and take the low
	movb	(sp)+,(r2)		/   byte
4:
	mov	(sp)+,nofault		/ restore error trap
	mov	(sp)+,r3		/ restore registers
	mov	(sp)+,r2
	rts	pc			/   and return
5:
	mov	$EFAULT,r0		/ we got a memory fault give them the
	br	4b			/   error


/*
 * error = vcopyout(fromaddr, toaddr, length)
 *	int	error;
 *	caddr_t	fromaddr, toaddr;
 *	u_int	length;
 *
 * Copy length bytes from kernel address space fromaddr to user space toaddr.
 * Returns zero on success, EFAULT on failure.  Vcopyout is only called when
 * fromaddr, toaddr or length is odd and the length doesn't justify an fmove.
 */
ENTRY(vcopyout)
	mov	r2,-(sp)		/ allocate a couple extra registers
	mov	r3,-(sp)
	mov	nofault,-(sp)
	mov	$5f,nofault		/ set up error trap
	mov	10(sp),r1		/ r1 = fromaddr (kernel space)
	mov	12(sp),r2		/ r2 = toaddr (user address)
	mov	14(sp),r0		/ r0 = length
	beq	4f			/ (exit early if 0)
	bit	$1,r2			/ toaddr odd?
	beq	1f
	dec	r2			/ yes, grab even word so we can stuff
	mfpd	(r2)			/   our first byte into the high byte
	movb	(r1)+,1(sp)		/   of that word
	mtpd	(r2)+
	dec	r0
1:
	mov	r0,r3			/ save trailing byte indicator and
	asr	r0			/ convert length remaining to units of
	beq	3f			/   words
2:
	movb	(r1)+,-(sp)		/ form word to copy out on the stack
	movb	(r1)+,1(sp)
	mtpd	(r2)+			/   and send it on its way
	sob	r0,2b
3:					/ r0 = 0
	asr	r3			/ need to copy out trailing byte?
	bcc	4f			/   nope, all done
	mfpd	(r2)			/ have to stuff our last byte out so
	movb	(r1),(sp)		/   stick it into the lower byte and
	mtpd	(r2)			/   rewrite it
4:
	mov	(sp)+,nofault		/ restore previous error trap
	mov	(sp)+,r3		/ restore registers
	mov	(sp)+,r2
	rts	pc			/   and return
5:
	mov	$EFAULT,r0		/ user memory fault ...  return
	br	4b			/   EFAULT


/*
 * error = copyinstr(fromaddr, toaddr, maxlength, &lencopied)
 *	int	error;
 *	caddr_t	fromaddr, toaddr;
 *	u_int	maxlength, *lencopied;
 *
 * Copy a null terminated string from the user address space into the kernel
 * address space.  Returns zero on success, EFAULT on user memory management
 * trap, ENOENT if maxlength exceeded.  If lencopied is non-zero, *lencopied
 * gets the length of the copy (including the null terminating byte).
 */
ENTRY(copyinstr)
	mov	r2,-(sp)		/ allocate a couple extra registers
	mov	r3,-(sp)
	mov	nofault,-(sp)
	mov	$7f,nofault		/ set up error trap
	mov	10(sp),r1		/ r1 = fromaddr (user address)
	mov	12(sp),r2		/ r2 = toaddr (kernel address)
	mov	14(sp),r0		/ r0 = maxlength (remaining space)
	beq	3f			/ (exit early with ENOENT if 0)
	bit	$1,r1			/ fromaddr odd?
	beq	1f
	dec	r1			/ yes, grab the even word to start
	mfpd	(r1)+			/   us off
	mov	(sp)+,r3
	br	2f			/ and enter the loop halfway in ...
1:
	mfpd	(r1)+			/ grab next word from user space
	mov	(sp)+,r3
	movb	r3,(r2)+		/ move the first byte
	beq	4f
	dec	r0
	beq	3f
2:
	swab	r3			/   and the second ...
	movb	r3,(r2)+
	beq	4f
	sob	r0,1b
3:
	mov	$ENOENT,r0		/ ran out of room - indicate failure
	br	5f			/   and exit ...
4:
	clr	r0			/ success!
5:
	tst	16(sp)			/ does the caller want the copy length?
	beq	6f
	sub	12(sp),r2		/ yes, figure out how much we copied:
	mov	r2,*16(sp)		/ *lencopied = r2 {toaddr'} - toaddr
6:
	mov	(sp)+,nofault		/ restore error trap
	mov	(sp)+,r3		/ restore registers
	mov	(sp)+,r2		/   and return
	rts	pc
7:
	mov	$EFAULT,r0		/ we got a memory fault give them the
	br	5b			/   error


/*
 * error = copyoutstr(fromaddr, toaddr, maxlength, lencopied)
 *	int	error;
 *	caddr_t	fromaddr, toaddr;
 *	u_int	maxlength, *lencopied;
 *
 * Copy a null terminated string from the kernel address space to the user
 * address space.  Returns zero on success, EFAULT on user memory management
 * trap, ENOENT if maxlength exceeded.  If lencopied is non-zero, *lencopied
 * gets the length of the copy (including the null terminating byte).  Note
 * that *lencopied will not by valid on EFAULT.
 */

ENTRY(copyoutstr)
	mov	r2,-(sp)		/ allocate a couple extra registers
	mov	r3,-(sp)
	mov	nofault,-(sp)
	mov	$7f,nofault		/ set up error trap

	/*
	 * First find out how much we're going to be copying:
	 *	min(strlen(fromaddr), maxlength).
	 */
	mov	10(sp),r0		/ r0 = fromaddr (kernel address)
	mov	14(sp),r1		/ r1 = maxlength (remaining space)
	beq	6f			/ (exit early with ENOENT if 0)
1:
	tstb	(r0)+			/ found null?
	beq	2f
	sob	r1,1b			/ run out of room?
	mov	14(sp),r0		/ ran out of room: r0 = maxlength
	br	3f
2:
	sub	10(sp),r0		/ found null: r0 = strlen(fromaddr)
3:
	tst	16(sp)			/ does the caller want the copy length?
	beq	4f			/ yes,
	mov	r0,*16(sp)		/   lencopied = r0 (invalid on EFAULT)
4:
	mov	10(sp),r1		/ r1 = fromaddr (kernel space)
	mov	12(sp),r2		/ r2 = toaddr (user address)
	bit	$1,r2			/ toaddr odd?
	beq	5f
	dec	r2			/ yes, grab even word so we can stuff
	mfpd	(r2)			/   our first byte into the high byte
	movb	(r1)+,1(sp)		/   of that word
	mtpd	(r2)+
	dec	r0
5:
	mov	r0,r3			/ save trailing byte indicator and
	asr	r0			/ convert space remaining to units of
	beq	2f			/   words
1:
	movb	(r1)+,-(sp)		/ form word to copy out on the stack
	movb	(r1)+,1(sp)
	mtpd	(r2)+			/   and send it on its way
	sob	r0,1b
2:
	asr	r3			/ need to copy out trailing byte?
	bcc	3f			/   nope, all done
	mfpd	(r2)			/ have to stuff our last byte out so
	movb	(r1)+,(sp)		/   stick it into the lower byte and
	mtpd	(r2)			/   rewrite it
3:
	movb	-(r1),r0		/ did we copy the null out?
	beq	5f
4:
	mov	$ENOENT,r0		/ no, so indicate ENOENT
5:
	mov	(sp)+,nofault		/ restore previous error trap
	mov	(sp)+,r3		/ restore registers
	mov	(sp)+,r2
	rts	pc			/   and return

	/*
	 * Rapacious silliness here - someone has passed us maxlength == 0 ...
	 */
6:
	tst	16(sp)			/ do they want to know about it?
	beq	4b			/ (guess not ...)
	clr	*16(sp)		/ *lencopied = 0
	br	4b			/ return ENOENT
7:
	mov	$EFAULT,r0		/ user memory fault ...  return
	br	5b			/   EFAULT
