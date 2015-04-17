
/*
 * Rx device register definitions
 */
struct rxdevice {
	int rxcs;	/* Command & Status Register */
	int rxdb;	/* Data Buffer Register */
};

/*
 * Rxcs definitions
 */
#define	GO	01
#define FILL	00
#define EMPTY	02
#define WRITE	04
#define READ	06
#define NOP	010
#define	RSTATUS	012
#define RERROR	016
#define UNIT	020
#define DONE	040
#define IENABLE	0100
#define TR	0200
#define INIT	040000
#define ERROR	0100000

/*
 * Rxes definitions
 */
#define CRC	01	/* CRC error */
#define	PARITY	02	/* Parity error */
#define IDONE	04	/* Initialize done */
#define DRVRDY 	0200	/* Drive 0 ready */
