/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)net_scb.s	1.2 (2.11BSD GTE) 10/13/92
 */

#include "acc.h"
#include "css.h"
#include "de.h"
#include "ec.h"
#include "il.h"
#include "qe.h"
#include "qt.h"
#include "sri.h"
#include "vv.h"

/*
 * Entry points for interrupt vectors from kernel low-core
 */

#define	HANDLER(handler)	.globl handler, _/**/handler; \
				handler: jsr r0,call; jmp _/**/handler

#if NACC > 0
	HANDLER(accrint)
	HANDLER(accxint)
#endif

#if NCSS > 0
	HANDLER(cssrint)
	HANDLER(cssxint)
#endif

#if NDE > 0
	HANDLER(deintr)
#endif

#if NEC > 0
	HANDLER(ecrint)
	HANDLER(eccollide)
	HANDLER(ecxint)
#endif

#if NIL > 0
	HANDLER(ilrint)
	HANDLER(ilcint)
#endif

#if NQE > 0
	HANDLER(qeintr)
#endif

#if NQT > 0
	HANDLER(qtintr)
#endif

#if NSRI > 0
	HANDLER(srixint)
	HANDLER(srirint)
#endif

#if NVV > 0
	HANDLER(vvrint)
	HANDLER(vvxint)
#endif
