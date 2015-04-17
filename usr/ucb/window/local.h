/*
 * @(#)local.h	3.5 1/1/94
 */

/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */

/*
 * Things of local interest.
 */

#define RUNCOM		".windowrc"
#define ESCAPEC		ctrl(p)
#ifdef pdp11
#define NLINE		3			/* default text buffer size */
#else
#define NLINE		48			/* default text buffer size */
#endif
#define SHELL		"/bin/csh"		/* if no environment SHELL */
