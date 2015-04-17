struct	dndevice
{
	short	dnisr[4];
};

/* bits in dnisr */
#define	DN_PWI		0100000		/* power indicate */
#define DN_ACR		0040000		/* abandon call and retry */
/* bit 13 is unused */
#define DN_FDLO		0010000		/* data line occupied */
/* bits 11-8 are the digit bits */
#define DN_DONE		0000200		/* done */
#define DN_INTENB	0000100		/* interrupt enable */
#define DN_DSS		0000040		/* data set status */
#define DN_FPND		0000020		/* present next digit */
#define DN_MAINT	0000010		/* maintenance */
#define DN_MINAB	0000004		/* master enable */
#define DN_FDPR		0000002		/* digit present */
#define DN_FCRQ		0000001		/* call request */
#define DN_BITS		\
"\10\20PWI\17ACR\15FDLO\10DONE\7INTENB\6DSS\5FPND\4MAINT\3MINAB\2FDPR\1FCRQ"
