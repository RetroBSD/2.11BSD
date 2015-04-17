#include <syscall.h>
rti=2
iot=4
nop=240

.data
. = . + 6000
/ this code is actually origined to 0146000

getovr:
	jmp	reado		/jmp to overlay reading routine
getdat:
	jmp	readd		/jmp to file reading routine
chrget:
	jmp	getchar		/jmp to RT-11 .TTYIN emulator
chrput:
	jmp	putchar		/jmp to RT-11 .TTYOUT emulator
printit:
	jmp	.print		/jmp to RT-11 .PRINT emulator

/  setup registers and start pgm where it was interrupted

start:
	mov	sp,r0
1:	tst	(r0)+			/calculate environment pointer
	bne	1b
	tst	(r0)+
	mov	r0,environ		/save environment pointer
	sys	SYS_getuid.		/setuid back to user's
	mov	r0,-(sp)
	clr	-(sp)
	sys	SYS_setuid.
	sys	SYS_getgid.
	mov	r0,2(sp)
	sys	SYS_setgid.
	cmp	(sp)+,(sp)+

	clr	(sp)
	mov	$emtvec,-(sp)
	mov	$7,-(sp)		/7 is EMT
	mov	$sigtramp,-(sp)
	tst	-(sp)
	sys	SYS_sigaction.		/intercept EMTs
	add	$10,sp

	mov	$timval,-(sp)
	tst	-(sp)
	sys	SYS_gettimeofday.	/get time-of-day
	cmp	(sp)+,(sp)+
	mov	hival,otime		/save high order
	mov	lowval,otime+2		/save low order

	clr	-(sp)			/We have to use sigstack
	mov	sp,sigstk		/because 2.10 won't use
	mov	$sigstk,-(sp)		/a signal stack on the
	tst	-(sp)			/normal process stack unless
	sys	SYS_sigstack.		/it's inside the allocated
	add	$6,sp			/stack area.

	mov	$4,r0
	mov	$140232,r1
	mov	$1,r2
	mov	$140442,r3		/from .SETTOP ??
	mov	$137724,r4
	mov	$20670,r5
	mov	$1102,sp

	jmp	*$17332

sigtramp:
	jsr	pc,(r0)			/Signal trampoline for sigaction
	mov	sp,r0
	add	$6,r0
	mov	r0,-(sp)
	tst	-(sp)
	sys	SYS_sigreturn.
	iot

sigstk:	0				/Structure for sigstack call
	0

emtvec: 
	emtrap				/interception routine
	0; 0				/Bit 6 is EMT (7)
	1				/Flag to run on our own signal stack

timval:					/Two longs in timeval
hival:	0
lowval:	0; 0; 0
otime:	0; 0				/time at pgm start-up

/  environment pointer
environ: 0

/  file discriptor numbers

ofd:	5	/doveraly open on fd5 (original RT-11 pgm)
ifd:	4	/dindex.dat open on fd4
tfd:	3	/dtext.dat open on fd3

/  disk i/o comes here

reado:
	mov	ofd,fd		/load fd with overaly chan file discrip
	br	readw		/branch to common read routine

readd:
	cmp	*(sp),$0	/check for chan 0 (dindex.dat)
	bne	1f		/skip if not
	mov	ifd,fd		/load index file discriptor
	br	readw

1:	cmp	*(sp),$1	/check for chan 1 (dtext.dat)
	bne	2f		/Oh oh!
	mov	tfd,fd		/load text file discriptor
	br	readw

2:	iot			/bomb-out with dump

/  read common routine

readw:
	mov	(sp)+,retaddr	/save return address
	mov	r1,-(sp)	/save r1 from mul
	mul	$512.,r0	/block offset to byte offset
	clr	-(sp)		/offset is from beginning of file
	mov	r1,-(sp)	/store low word of offset on stack
	mov	r0,-(sp)	/and high word at next word on stack

	mov	(pc)+,-(sp)	/put file discriptor on stack
fd:	-1			/file discriptor
	tst	-(sp)
	sys	SYS_lseek.	/seek to proper disk block
	add	$12,sp		/restore stack
	mov	(sp)+,r1	/restore r1
	mov	(sp)+,r0	/load address for read
	asl	(sp)		/convert word count to byte count
	mov	r0,-(sp)	/put address on stack
	mov	fd,-(sp)	/load file discriptor on stack
	tst	-(sp)
	sys	SYS_read.
	add	$12,sp		/clean-up
	jmp	*(pc)+		/return
retaddr: -1			/return address

/  character input routine (.TTYIN)

getchar:
	mov	$1,-(sp)	/read 1 character
	mov	$char,-(sp)	/into char
	clr	-(sp)		/read from standard input (0)
	tst	-(sp)
	sys	SYS_read.	/read 1 character
	add	$10,sp		/clean up

/  special command processing

	tst	r0		/EOF?
	bne	1f		/nope
	sys	SYS_exit.	/yup
1:
	mov	char,r0		/pick-up char
	cmp	lastch,$'\n	/last char a newline?
	bne	4f		/nope - we're done

	cmp	r0,$'>		/save game?
	bne	2f		/nope
	jsr	pc,save
	br	getchar
2:
	cmp	r0,$'<		/restore game?
	bne	3f		/nope
	jsr	pc,restor
	br	getchar
3:
	cmp	r0,$'!		/escape to UNIX sh?
	beq	6f		/yes! - go fork
4:
	mov	r0,(pc)+	/save last char
lastch:	'\n
	cmp	$12,r0		/char a linefeed (newline)?
	bne	5f		/no
	mov	$15,r0		/map LF to CR
5:
	rts	pc		/back to dungeon

6:
	sys	SYS_fork.	/fork!
	br	child		/daughter task returns here
				/parent returns here

	mov	r1,-(sp)	/protect r1 from wait

	clr	-(sp)		/ rusage
	clr	-(sp)		/ options
	clr	-(sp)		/ status
	mov	$-1,-(sp)	/ wpid
	tst	-(sp)		/ fake return address
	sys	SYS_wait4.	/ wait for daughter to complete
	add	$5*2,sp
	mov	(sp)+,r1	/restore r1

	mov	$3,-(sp)	/write prompt when through
	mov	$prom,-(sp)
	mov	$1,-(sp)
	tst	-(sp)
	sys	SYS_write.
	add	$10,sp
	br	getchar		/go get more input

prom:	<!\n\>>			/input prompt characters
	.even

/  the following code is run by the daughter task
child:
	clr	-(sp)
	mov	$sigdef,-(sp)
	mov	$2,-(sp)		/SIGQUIT
	clr	-(sp)			/no trampoline routine
	clr	-(sp)			/2.11 interface (return @)
	sys	SYS_sigaction.
	add	$12,sp

	mov	environ,-(sp)		/set environ pointer
	mov	$shargs,-(sp)
	mov	$shname,-(sp)
	tst	-(sp)
	sys	SYS_execve.		/process "!" command line
	add	$10,sp
	sys	SYS_exit.

sigdef: 
	0; 0; 0; 0

shname:	</bin/sh\0>
	.even
shargs:	sharg0; sharg1; 0
sharg0:	<-c\0>
sharg1:	<-t\0>
	.even

/  save game

save:
	tst	-(sp)
	mov	$600,-(sp)		/mode
	mov	$601,-(sp)		/O_CREAT|O_TRUNC|O_WRONLY
	mov	$savfil,-(sp)		/path
	tst	-(sp)			/2.11 syscall convention
	sys	SYS_open.		/create output file
	bcc	1f			/oops

/ this bit of nonsense is needed because 'serr' insists that all syscalls
/ take 10(8) bytes of stack.  SYS_open takes 12 so we advance sp by 2 if the
/ open failed.  Stuff a -1 in the return value so the check for failure can /
/ be made.  Isn't assembly fun?! <grin>.

	tst	(sp)+
	br	serr
1:
	add	$12,sp
	mov	r0,(pc)+		/save "save" file discriptor
sfd:	-1				/ "save" file discriptor
	mov	$17812.,-(sp)
	mov	$22410,-(sp)
	mov	r0,-(sp)
	tst	-(sp)
	sys	SYS_write.		/write out data
	bcs	rwerr			/branch on error
	add	$10,sp
	mov	sfd,-(sp)		/load "save" file discriptor
	tst	-(sp)
	sys	SYS_close.		/close "save" file
	cmp	(sp)+,(sp)+
	mov	$11.,-(sp)
	mov	$svmsg,-(sp)
	mov	$1,-(sp)		/load standard output fd
	tst	-(sp)
	sys	SYS_write.		/convey success
	add	$10,sp
	rts	pc			/return

svmsg:	<Game saved\n>			/success message
	.even

/  restore game

restor:
	tst	-(sp)
	clr	-(sp)
	mov	$savfil,-(sp)
	tst	-(sp)
	sys	SYS_open.		/open "save" file
	bcs	serr			/branch on error
	add	$10,sp
	mov	r0,sfd			/store "save" file fd
	mov	$17812.,-(sp)
	mov	$22410,-(sp)
	mov	r0,-(sp)
	tst	-(sp)
	sys	SYS_read.		/read data
	bcs	rwerr			/branch on failure
	add	$10,sp
	mov	sfd,-(sp)		/load "save" file fd
	tst	-(sp)
	sys	SYS_close.		/close the file
	cmp	(sp)+,(sp)+
	mov	$14.,-(sp)
	mov	$rsmsg,-(sp)
	mov	$1,-(sp)		/load char output fd
	sys	SYS_write.		/report success
	add	$6,sp
	rts	pc			/return

rsmsg:	<Game restored\n>

savfil:	<dungeon.dat\0>			/ "save" file name
	.even

/  open, read & write errors come here

rwerr:
	mov	sfd,-(sp)		/close file on read/write error
	tst	-(sp)
	sys	SYS_close.
	cmp	(sp)+,(sp)+
serr:
	add	$10,sp
	mov	$7.,-(sp)
	mov	$ermsg,-(sp)
	mov	$1,-(sp)		/using standard output
	tst	-(sp)
	sys	SYS_write.		/  write "ERROR" message
	add	$10,sp
	rts	pc			/return (exit for child)

ermsg:	<Error\n>
	.even

/  RT-11 .TTYOUT emulator

putchar:
	mov	r0,(pc)+	/save character
char:	0			/char to print
	mov	$1,-(sp)	/write one character
	mov	$char,-(sp)
	mov	$1,-(sp)	/load terminal output fd
	tst	-(sp)
	sys	SYS_write.	/write it out
	add	$10,sp
	rts	pc		/return

/  RT-11 .PRINT emulator

.print:
	mov	r1,-(sp)	/save r1 & r2
	mov	r2,-(sp)
	mov	r0,r1		/scan for terminator (right 7 bits 0)
1:
	movb	(r1)+,r2
	bic	$177600,r2
	bne	1b

/  terminator found

	movb	-(r1),r2	/save terminator
	sub	r0,r1		/size of string to output
	mov	r1,-(sp)	/save count
	mov	r0,-(sp)
	mov	$1,-(sp)	/write to standard output
	tst	-(sp)
	sys	SYS_write.
	add	$10,sp
	tstb	r2		/skip crlf if 200 bit set in terminator
	bmi	2f
	mov	$2,-(sp)
	mov	$crlf,-(sp)
	mov	$1,-(sp)
	tst	-(sp)
	sys	SYS_write.	/output crlf
	add	$10,sp
2:
	mov	(sp)+,r2	/restore registers
	mov	(sp)+,r1
	rts	pc

crlf:	<\r\n>
	.even

emtrap:
	mov	6(sp),r0		/Get sigcontext
	mov	14(r0),-(sp)		/Put old r0 on stack
	mov	16(r0),r0		/Pull old pc off stack
	cmp	$104350,-(r0)		/Check for EMT 350
	beq	.exit			/Exit if it is.
	cmp	$104375,(r0)		/if EMT not 375
	bne	3f			/ then abort with dump

	mov	(sp),r0			/check for .GTIM
	cmp	$10400,(r0)
	bne	1f
	tst	(r0)+			/bump r0
	mov	r1,-(sp)		/save r1
	mov	r2,-(sp)		/save r2
	mov	(r0),r2			/get pointer

	clr	-(sp)
	mov	$timval,-(sp)
	tst	-(sp)
	sys	SYS_gettimeofday.	/get current time
	add	$6,sp
	mov	lowval,r1
	mov	hival,r0

	sub	otime+2,r1		/subtract low order
	sbc	r0
	sub	otime,r0		/subtract high order
	mov	r1,-(sp)		/save low order
	mul	$60,r0			/convert seconds to ticks
	mov	r1,(r2)			/store high order
	mov	(sp)+,r0		/restore low order
	mul	$60,r0			/mult low order by 60
	mov	r1,2(r2)		/store low in low order
	add	r0,(r2)			/add high to high
	add	$6223,2(r2)		/add fudge factor
	adc	(r2)
	mov	(sp)+,r2		/restore r2 & r1
	mov	(sp)+,r1
	jbr	2f			/go return from interrupt
1:
	cmp	$16000,(r0)+		/check if .GVAL
	bne	3f
	cmp	$300,(r0)		/do .GVAL only of 300
	bne	3f
	mov	$121100,(sp)		/load fake RT-11 config word
2:
	mov	(sp)+,r0		/restore r0
	rts	pc			/Return From Interrupt
3:
	mov	(r0),r1			/Get emt number
	bic	$177077,r1		/Fix up error message
	ash	$-6,r1
	add	$60,r1
	movb	r1,emt
	mov	(r0),r1			/Get emt number
	bic	$177707,r1		/Fix up error message
	ash	$-3,r1
	add	$60,r1
	movb	r1,emt+1
	mov	(r0),r1			/Get emt number
	bic	$177770,r1		/Fix up error message
	add	$60,r1
	movb	r1,emt+2

	jsr	pc,save			/save game before blow-up
	mov	$core62,-(sp)
	tst	-(sp)
	sys	SYS_chdir.		/put core dump in special place
	cmp	(sp)+,(sp)+
	mov	$29.,-(sp)		/Write an explaination out
	mov	$emtmsg,-(sp)
	mov	$1,-(sp)
	tst	-(sp)
	sys	SYS_write.
	add	$10,sp
	mov	(sp)+,r0		/restore r0
	rts	pc			/return

emtmsg:
	<A vicious emt >
emt:	<xxx>
	< kills you.\n>

core62:
	</usr/games/lib/coredumps\0>
	.even

/.EXIT emulator with a minor cleanup
.exit:
	mov	$2,-(sp)
	mov	$crlf,-(sp)
	mov	$1,-(sp)
	tst	-(sp)
	sys	SYS_write.
	sys	SYS_exit.
