/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)attach.c	2.1 (2.11BSD GTE) 1/10/94
 */

/*
 * Attach the passed device:
 *	Patch the interrupt vector
 *	Call the device attach routine
 *
 *	Call (if present) the vector setting routine, passing the
 *	vector from /etc/dtab to the driver.  At present only the
 *	MSCP and TMSCP drivers have routines to do this.  The detach()
 *	routine was removed because it was superfluous.  The kernel no
 *	longer calls every driver's XXroot() entry point at start up time,
 *	thus there is nothing to detach any more.
 */

#include <machine/psl.h>
#include <machine/autoconfig.h>
#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>
#include "dtab.h"
#include "ivec.h"

extern	int	debug;
int	vec_set[NVECTOR], num_vec;

attach(dp)
DTAB	*dp;
{
	extern int	errno;
	int	unit,
		addr,
		ret;
	register HAND	*sp;

	if ((unit = find_unit(dp)) == -1) {
		prdev(dp);
		printf(" unit already in use\n");
		return;
	}				/* first attach the device */
	if (debug)
		printf("attach: ucall %o(PSL_BR0, %o, %o)\n",dp->dt_attach->n_value,dp->dt_addr,unit);
	else {
		errno = 0;
		if (!(ret = ucall(PSL_BR0, dp->dt_attach->n_value, dp->dt_addr, unit)) || ret == -1) {
			prdev(dp);
			if (ret == -1 && errno) {
				perror("ucall");
				exit(AC_SINGLE);
			}
			printf(" attach failed\n");
			return;
		}
	}				/* then fill the interrupt vector */
	addr = dp->dt_vector;
	for (sp = (HAND *)dp->dt_handlers;sp;sp = sp->s_next) {
		if (!sp->s_nl->n_value) {
			prdev(dp);
			printf(" no address found for %s\n", sp->s_str);
			exit(AC_SINGLE);
		}
		write_vector(addr, sp->s_nl->n_value, pry(dp) + unit);
		if (num_vec == NVECTOR - 1) {
			printf("Too many vectors to configure\n");
			exit(AC_SINGLE);
		}
		else
			vec_set[num_vec++] = addr;
		addr += IVSIZE;
	}
	prdev(dp);
	if (dp->dt_setvec && dp->dt_setvec->n_value) {
		ret = ucall(PSL_BR0,dp->dt_setvec->n_value,unit,dp->dt_vector);
		if (ret == -1) {
			printf(" vectorset failed\n");
			exit(AC_SINGLE);
		}
	printf(" vectorset");
	}
	printf(" attached\n");
}

have_set(vec)
{
	int	cnt_vec;

	for (cnt_vec = 0;cnt_vec < num_vec;++cnt_vec)
		if (vec_set[cnt_vec] == vec)
			return(1);
	return(0);
}

pry(dp)
DTAB	*dp;
{
	switch(dp->dt_br) {
		case 4:
			return(PSL_BR4);
		case 5:
			return(PSL_BR5);
		case 6:
			return(PSL_BR6);
		default:
			prdev(dp);
			printf(": br%d is not supported.  Assuming 7\n", dp->dt_br);
		case 7:
			return(PSL_BR7);
	}
}

write_vector(addr,value,pri)
int	addr,
	value,
	pri;
{
	stuff(value,addr);
	stuff(pri,addr + sizeof(int));
}

/*
 * find_unit -- Add this device to the list of devices if it isn't already
 * in the list somewhere.  If it has an explicit unit number then use that,
 * else fill in the wildcard with next biggest number.
 */

find_unit(dp)
DTAB	*dp;
{
	typedef struct done_s	{
		char	*d_name;	/* device name */
		int	d_unit;		/* current unit for wildcarding */
		struct	done_s *d_next;
	} DONE;
	static DONE	*done,
			*dn;
	int	want_unit;
	char	*malloc();

	if (!done) {				/* initialize list */
		done = dn = (DONE *)malloc((u_int)(sizeof(DONE)));
		dn->d_name = dp->dt_name;
		dn->d_next = NULL;
		if ((dn->d_unit = dp->dt_unit - 1) < -1)
			dn->d_unit = -1;
	}
	else for (dn = done;;dn = dn->d_next)	/* search list */
		if (!strcmp(dn->d_name,dp->dt_name))
			break;
		else if (!dn->d_next) {
			dn->d_next = (DONE *)malloc((u_int)sizeof(DONE));
			dn = dn->d_next;
			dn->d_next = NULL;
			dn->d_name = dp->dt_name;
			if ((dn->d_unit = dp->dt_unit - 1) < -1)
				dn->d_unit = -1;
			break;
		}				/* fill in wildcards */
	if ((want_unit = dp->dt_unit) == -1)
		want_unit = dn->d_unit + 1;
	else if (want_unit <= dn->d_unit)
		return(ERR);
	return(dn->d_unit = dp->dt_unit = want_unit);
}
