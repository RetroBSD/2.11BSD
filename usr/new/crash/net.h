#define	CRASH
#define	MAD	HEX		/* format for octal/hex dumps */
#define	DUMP(x)	hexout((x))	/* format for octal/hex dumps */

/*
 * Arena (Mbuf) cross-reference table entry
 */
struct arenas {
	struct	arenas *as_next;	/* link */
	unsigned as_addr;		/* address in arena*/
	short   as_ref;			/* reference counter */
	short	as_size;
	short	as_flags;
};
#define	AS_SOCK		01
#define	AS_MBUF		02
#define	AS_ARCK		04
#define	AS_BDSZ		010
#define	AS_FREE		020
#define	AS_INPCB	040
#define	AS_TCPCB	0100
#define	AS_RTENT	0200
#define	AS_RAWCB	0400

struct arenas	*getars(),
		*putars();

char *arenap;		/* stuffed with addr of arena, an auto variable */
unsigned int	allocs,
		alloct;

#define klseek(fd,base,off)	lseek(fd, (off_t)((unsigned)base), off)
#define	VALADD(a,type)	((unsigned)(a) > allocs && \
			 (unsigned)(a) < alloct-sizeof (type))
#define	VALMBA(a)	VALADD(a,struct mbuf)
#define	VALMBXA(a)	((unsigned)(a) >= mbstat.m_mbxbase && \
			 (unsigned)(a) <= mbend-(sizeof (struct mbufx)/64))
#define	XLATE(a,type)	(type)(arenap + (unsigned)(a) - allocs)

