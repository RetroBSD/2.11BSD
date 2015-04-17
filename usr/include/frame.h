/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)frame.h	7.1 (Berkeley) 6/5/86
 */

/*
 * Definition of the pdp call frame.
 */
struct frame {
	int	fr_savr2;		/* saved register 2 */
	int	fr_savr3;		/* saved register 3 */
	int	fr_savr4;		/* saved register 4 */
	int	fr_savov;		/* saved overlay number */
	int	fr_savfp;		/* saved frame pointer */
	int	fr_savpc;		/* saved program counter */
};
