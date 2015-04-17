#define		_CNT
#define		_PTR	2.
#define		_BASE	4.
#define		_BUFSIZ	6.
#define		_FLAG	8.
#define		_FILE	10.
#define		IOBSIZ	12.

#define		_IOREAD 01
#define		_IOWRT  02
#define		_IONBF	04
#define		_IOMYBUF 010
#define		_IOEOF  020
#define		_IOLBF	0200
#define		_IORW   0400

#define		NULL	0
#define		EOF	-1
#define		NL	012

.globl	__iob
#define		STDIN	__iob
#define		STDOUT	[__iob+IOBSIZ]
#define		STDERR	[__iob+IOBSIZ+IOBSIZ]
