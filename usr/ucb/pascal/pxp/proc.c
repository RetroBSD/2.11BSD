#
/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

proc(r)
	int *r;
{

	call(r[2], r[3]);
}
