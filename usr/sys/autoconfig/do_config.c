/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)do_config.c	2.1 (2.11BSD GTE) 12/30/92
 */

/*
 * Now with all our information, make a configuration
 * Devices without both attach and probe routines will
 * not be configured into the system
 */

#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/autoconfig.h>
#include <sys/types.h>
#include <a.out.h>
#include <errno.h>
#include <stdio.h>
#include "dtab.h"
#include "ivec.h"

extern	int	kmem, verbose, debug, errno, complain;
extern	NLIST	*bad_nl, *good_nl, *int_nl, *end_vector, *trap_nl, *sep_nl;
extern	NLIST	*next_nl;

grab(where)
u_int	where;
{
	int	var;

	lseek(kmem,((long) where) & 0xffffL,0);
	read(kmem,&var,sizeof(var));
	if (debug)
		printf("Grab %o = %o",where, var);
	return(var);
}

stuff(var, where)
unsigned int where;
{
	char s[20];

	if (debug) {
		printf("Stuff %o @ %o\n", var, where);
		return;
	}
	lseek(kmem, ((long) where) & 0xffffL, 0);
	if (write(kmem, &var, sizeof var) < sizeof var) {
		sprintf(s, "stuff 0%o", where);
		perror(s);
	}
}

prdev(dp)
DTAB	*dp;
{
	printf("%s ", dp->dt_name);
	if (dp->dt_unit == -1)
		putchar('?');
	else
		printf("%d", dp->dt_unit);
	printf(" csr %o vector %o", dp->dt_addr, dp->dt_vector);
}

/*
 * Go through the configuration table and probe all the devices.  Call
 * attach for ones which exist.  Probe routines should return
 *	ACP_NXDEV	No such device
 *	ACP_IFINTR	Device exists if interrupts ok
 *	ACP_EXISTS	Device exists
 * All interrupt vectors are poked to either point to conf_goodvec or
 * conf_badvec which change the value of conf_int.  Values of this
 * variable are:
 *	ACI_BADINTR	Device interrupted through wrong vector
 *	ACI_NOINTR	Device didn't interrupt
 *	ACI_GOODINTR	Interrupt ok
 */

auto_config()
{
	register DTAB	*dp;
	int ret;

	if (intval() != CONF_MAGIC) {
		fputs(myname,stderr);
		fputs(": namelist doesn't match running kernel.\n",stderr);
		exit(AC_SETUP);
	}

	init_lowcore(bad_nl->n_value);

	for (dp = devs; dp != NULL; dp = dp->dt_next) {
		/*
		 * Make sure we have both a probe and attach routine
		 */
		if (!((dp->dt_uprobe || (dp->dt_probe && dp->dt_probe->n_value))
		    && (dp->dt_attach && dp->dt_attach->n_value))) {
			if (debug || verbose) {
				prdev(dp);
				puts(" skipped:  No autoconfig routines.");
			}
			continue;
		}			/* Make sure the CSR is there */
		errno = 0;
		grab(dp->dt_addr);
		if (errno) {
			if (errno != EFAULT && errno != ENXIO)
				perror("Reading CSR");
			if (debug || verbose) {
				prdev(dp);
				puts(" skipped:  No CSR.");
			}
			continue;
		}			/* Ok, try a probe now */
		if (expect_intr(dp)) {
			if (complain) {
				prdev(dp);
				puts(" interrupt vector already in use.");
			}
			continue;
		}
		ret = do_probe(dp);
		clear_vec(dp);
		switch (ret) {
			case ACP_NXDEV:
				if (debug || verbose) {
					prdev(dp);
					puts(" does not exist.");
				}
				break;
			case ACP_IFINTR:
				switch (intval()) {
					case ACI_BADINTR:
						if (debug || verbose || complain) {
							prdev(dp);
							puts(" interrupt vector wrong.");
						}
						break;
					case ACI_NOINTR:
						if (complain) {
							prdev(dp);
							puts(" didn't interrupt.");
						}
						break;
					case ACI_GOODINTR:
						attach(dp);
						break;
					default:
						prdev(dp);
						printf(" bad interrupt value %d.\n", intval());
						break;
				}
				break;
			case ACP_EXISTS:
				attach(dp);
				break;
			default:
				prdev(dp);
				printf(" bad probe value %d.\n", ret);
				break;
		}
	}
	set_unused();
}

/*
 * Return the current value of the interrupt return flag.
 * Initial value is the magic number
 */

static int conf_int = CONF_MAGIC;

intval()
{
	if (debug)
		return conf_int;
	else
		return grab(int_nl->n_value);
}

static int save_vec[9][2], save_p;

/*
 * Fill all interrupt vectors of this device with pointers to
 * the good interrupt vector routine.  Also save values to
 * later restore them.  Since we init_lowcore() everything to
 * conf_badint, anything not equalling that indicates a vector
 * which is already in use; unless the vector was initialized in l.s,
 * this device should be skipped.
 */

expect_intr(dp)
DTAB	*dp;
{
	HAND	*hp;
	register int	addr = dp->dt_vector;

/*
 * A vector of 0 has special meaning for devices which support programmable
 * (settable) vectors.  If a xxVec() entry point is present in the driver and
 * /etc/dtab has a value of 0 for the vector then 'autoconfig' will allocate
 * one by calling the kernel routine 'nextiv'.
 *
 * If multiple handlers are declared for a device (at present there are no 
 * progammable vector devices with more than 1 handler) the vector passed 
 * to the driver will be the lowest one (the first handle corresponds to 
 * the lowest vector).
*/
	if (!addr) {
		if (dp->dt_setvec == 0) {
			prdev(dp);
			printf(" vector = 0, %sVec undefined\n", dp->dt_name);
			return(1);
		}
/*
 * Now count the number of vectors needed.  This has the side effect of
 * allocating the vectors even if an error occurs later.  At the end of
 * the scan the last vector assigned will be the lowest one.  In order to
 * assure adjacency of multiple vectors BR7 is used in the call to the 
 * kernel and it is assumed that at this point in the system's life 
 * nothing else is allocating vectors (the networking has already grabbed the
 * ones it needs by the time autoconfig is run).
*/
		for (hp = dp->dt_handlers; hp; hp = hp->s_next) {
			addr = ucall(PSL_BR7, next_nl->n_value, 0, 0);
			if (addr <= 0) {
				printf("'nextiv' error for %s\n",
					dp->dt_name);
				return(1);
			}
		}
/*
 * Now set the lowest vector allocated into the device entry for further
 * processing.  From this point on the vector will behave just as if it
 * had been read from /etc/dtab.
*/
		dp->dt_vector = addr;
	}

	for (save_p = 0, hp = (HAND *)dp->dt_handlers;hp;hp = hp->s_next) {
		save_vec[save_p][1] = grab(addr + sizeof(int));
		if (((save_vec[save_p][0] = grab(addr)) != bad_nl->n_value)
		    && ((save_vec[save_p][0] != hp->s_nl->n_value)
		    || have_set(addr))) {
			clear_vec(dp);
			return 1;
		}
		save_p++;
		write_vector(addr, good_nl->n_value, PSL_BR7);
		addr += IVSIZE;
	}
	return 0;
}

clear_vec(dp)
register DTAB	*dp;
{
	register int addr = dp->dt_vector, n;

	for (n = 0; n < save_p; n++) {
		write_vector(addr, save_vec[n][0], save_vec[n][1]);
		addr += IVSIZE;
	}
}

init_lowcore(val)
{
	int addr;

	if (debug)
		return;
	for (addr = 0; addr < end_vector->n_value; addr += IVSIZE) {
		if (grab(addr) || grab(addr + 2))
			continue;
		write_vector(addr, val, PSL_BR7);
	}
}

do_probe(dp)
register DTAB	*dp;
{
	int func;
	int ret;

	func = dp->dt_probe->n_value;
	if (debug) {
		char line[80];

		if (func)
			printf("ucall %o(PSL_BR0, %o, 0):", func, dp->dt_addr);
		else
			printf("probe %s:", dp->dt_name);
		fputs(" return conf_int:",stdout);
		gets(line);
		sscanf(line, "%o%o", &ret, &conf_int);
		return ret;
	}
	stuff(0, int_nl->n_value);	/* Clear conf_int */
	/*
	 * use the kernel's probe routine if it exists,
	 * otherwise use our internal probe.  Pass it the first (lowest)
	 * vector assigned to the device.
	 */
	if (func) {
		errno = 0;
		ret = ucall(PSL_BR0, func, dp->dt_addr, dp->dt_vector);
		if (errno)
			perror("ucall");
		return(ret);
	}
	return((*(dp->dt_uprobe))(dp->dt_addr,  dp->dt_vector));
}

set_unused()
{
	int addr;

	if (debug)
		return;
	if (sep_nl->n_value) {
		/*
		 * On non-separate I/D kernel, attempt to set up catcher
		 * at 0 for both jumps and traps to 0.
		 * On traps, set PS for randomtrap, with the catcher at 0444.
		 * On jumps, branch to 0112, then to 050 (where there's space).
		 * The handler at 50 is already in place.
		 * The funny numbers are due to attempts to find vectors
		 * that are unlikely to be used, and the need for the value
		 * at 0 to serve as both a vector and an instruction
		 * (br 112 is octal 444).
		 */
		if ((grab(0110) == bad_nl->n_value) && (grab(0112) == PSL_BR7)
		    && (grab(0444) == bad_nl->n_value) && (grab(0446) == PSL_BR7)) {
			stuff(0444, 0);			/* br 0112 */
			stuff(PSL_BR7 + T_ZEROTRAP, 2);
			stuff(trap_nl->n_value, 0110);	/* trap; 756 (br7+14.)*/
			stuff(0756, 0112);		/* br 050 */
			stuff(0137, 0444);		/* jmp $*trap */
			stuff(trap_nl->n_value, 0446);
		}
	}
	for (addr = 0; addr < end_vector->n_value; addr += IVSIZE) {
		if (grab(addr) != bad_nl->n_value)
			continue;
		write_vector(addr, trap_nl->n_value, PSL_BR7+T_RANDOMTRAP);
	}
}
