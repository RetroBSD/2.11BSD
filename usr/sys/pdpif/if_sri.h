/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_sri.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * SRI dr11c interface
 */

struct sridevice {
	u_short csr;            /* control/status */
	u_short obf;            /* out buffer */
	u_short ibf;            /* in buffer */
};

/*
 * control and status register.
 */
#define SRI_IREQ        0x8000          /* input request or error */
#define SRI_OREQ        0x0080          /* output request */
#define SRI_OINT        0x0040          /* output intr enable */
#define SRI_IINT        0x0020          /* input intr enable */
#define SRI_IENB        0x0002          /* input enable */
#define SRI_OENB        0x0001          /* output enable */

/*
 * input buffer register.
 */
#define IN_CHECK        0x8000          /* check input or error */
#define IN_HNRDY        0x2000          /* host not ready */
#define IN_INRDY        0x1000          /* imp not ready */
#define IN_LAST         0x0800          /* last bit */

/*
 * output buffer register.
 */
#define OUT_LAST        IN_LAST         /* last bit */
#define OUT_HNRDY       IN_HNRDY        /* host not ready */
#define OUT_HRDY        0x1000          /* host ready */


#define SRI_INBITS \
"\20\20CHECK\16HNRDY\15INRDY\14LAST"

#define SRI_BITS \
"\20\20IREQ\10OREQ\7OINT\6IINT\2IENB\1OENB"
