/*
 * badsect
 *
 * Badsect takes a list of file-system relative sector numbers
 * and makes files containing the blocks of which these sectors are a part.
 * It can be used to contain sectors which have problems if these sectors
 * are not part of the bad file for the pack (see bad144).  For instance,
 * this program can be used if the driver for the file system in question
 * does not support bad block forwarding.
 *
 * Bugfix 930802 by Johnny Billquist
 * Sanity check on sector number August 10,1993 by Steven Schultz
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/inode.h>

main(argc, argv)
	int argc;
	char **argv;
{
	char nambuf[32];
	long sector;
	int errs = 0;

	--argc, argv++;
	while (argc > 0) {
		sector = atol(*argv);
		if (sector <= 0 || sector > 131071L) {
			fprintf(stderr, "Sector %s <= 0 or > 131071\n",
				*argv);
			continue;
		}
		if (mknod(*argv, IFMT, (u_short)(sector / CLSIZE)))
			perror("mknod"), errs++;
		argc--, argv++;
	}
	exit(errs);
}
