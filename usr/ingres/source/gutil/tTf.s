/
/  test trace flag routine.
/
/  this is a highly tooled routine for efficiency reasons.
/  The equivalent C code is:
/	tTf(m, n)
/	{
/		extern char	tTany;
/		extern int	tT[];
/
/		if (n < 0)
/			return (tT[m]);
/		else
/			return ((tT[m] >> n) & 01);
/	}
/
/	Call:
/		if (tTf(m, n)) ...
/		Tests bit n of trace flag m (or any bit of m if n
/			< 0)
/
/	History:
/		12/13/78 (eric) -- written from C
/

.globl	_tTf
.globl	_tTany
.globl	_tT

_tTf:
	movb	_tTany,r0	/ test for any flags on
	beq	1f		/ no, exit
	mov	2(sp),r0	/ yes, get specific flag
	asl	r0
	mov	_tT(r0),r0
	mov	4(sp),r1	/ what bit?
	blt	1f		/ any: just return
	neg	r1		/ move to low-order bit
	ash	r1,r0
	bic	$177776,r0	/ clear all others
1:
	rts	pc
