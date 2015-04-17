/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uprobe.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

typedef struct uprobe {
	char	*up_name;
	int	(*up_func)();
} UPROBE;
