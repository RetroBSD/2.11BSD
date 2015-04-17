/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)drreg.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *	Registers of the DR11-W parallel DMA interface
 */
struct	drdevice {
	short	wcr;		/* word count register */
	short	bar;		/* bus address register */
	short	csr;		/* csr/eir register */
	short	dar;		/* input/output data register */
};

struct	sg1 {
	short	word;		/* pass one word of info in or out */
};

struct	sg2 {
	short	word1;		/* pass two words of info in or out */
	short	word2;
};

/* Bits of the csr */
#define	DR_GO	0000001		/* start transfer */
#define	DR_FN1	0000002		/* user defined function bits */
#define	DR_FN2	0000004
#define	DR_FN3	0000010
#define DR_XBA	0000060		/* Unibus extension bits */
#define	DR_IE	0000100		/* interrupt enable */
#define	DR_RDY	0000200		/* ready bit */
#define	DR_CYL	0000400		/* cycle start */
#define	DR_ST1	0001000		/* user defined status */
#define	DR_ST2	0002000
#define	DR_ST3	0004000
#define	DR_MANT	0010000		/* maintenance mode bit */
#define	DR_ATTN	0020000		/* attention bit from interface */
#define	DR_NEX	0040000		/* non-existant memory */
#define	DR_ERR	0100000		/* general error bit */

#define	DRCSR_BITS	\
"\10\20ERR\17NEX\16ATTN\15MAINT\14STA\13STB\12STC\
\11CYCL\10RDY\7IE\6XBA17\5XBA16\4FN3\3FN2\2FN1\1GO"

/* Bits of the EIR */
#define	DR_FLG	0000001		/* register flag 1=EIR, 0=CSR */
#define	DR_NCYL	0000400		/* N-cycle burst selected */
#define	DR_BDL	0001000		/* Burst data late */
#define	DR_PAR	0002000		/* parity error */
#define	DR_ACLO	0004000		/* powerfailure */
#define	DR_MRQ	0010000		/* multicycle request */

/* All remaining bits same as CSR */
#define	DREIR_BITS	\
	"\10\20ERR\17NEX\16ATTN\15MRQ\14ACLO\13PAR\12BDL\11NCYL\1REG"

/*
 *	Definitions for ioctl calls for DR11-W interface
 */
#define	DRGTTY		_IOR(d, 1, struct sg2)	/* get dr11 status */
#define	DRSTTY		_IOW(d, 2, struct sg2)	/* set flags & function */
#define	DRSFUN		_IOW(d, 3, struct sg1)	/* set function */
#define	DRSFLAG		_IOW(d, 4, struct sg1)	/* set flags */
#define	DRGCSR		_IOR(d, 5, struct sg2)	/* get csr and wcr */
#define	DRSSIG		_IOW(d, 6, struct sg1)	/* set sig for ATTN interrupt */
#define	DRESET		_IO(d, 7)		/* reset DR11-W interface */
#define	DRSTIME		_IOW(d, 8, struct sg1)	/* set timeout */
#define	DRCTIME		_IO(d, 9)		/* set timeout inactive */
#define	DROUTPUT	_IOW(d, 10, struct sg1)	/* word to output data reg */
#define	DRINPUT		_IOR(d, 11, struct sg1)	/* word from input data reg */
#define	DRITIME		_IO(d, 12)		/* no set error on timeout */

/*
 *	i_flags definition
 */
#define	DR_ALIVE	0000001	/* unit has attatched */
#define	DR_OPEN		0000002	/* unit has been opened */
#define	DR_TIMEOUT	0000004	/* unit needs timeout (set by user) */
#define	DR_TACTIVE	0000010	/* timeout active on unit */
#define	DR_IGNORE	0000020	/* ignore timeout error */
