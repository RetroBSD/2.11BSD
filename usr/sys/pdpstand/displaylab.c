/*
 * 1995/06/08 - Borrowed from disklabel.c
 *
 * Many of the unused or invariant fields are omitted from the display.  This
 * has the benefit of having everything fit on one screen - important since
 * the standalone console driver does not do xon/xoff.
 *
 *  The partition display is jagged because the minimal printf() function
 * does not do field padding or justification.
*/

#define	DKTYPENAMES
#include <sys/param.h>
#include "saio.h"

displaylabel(lp)
	register struct disklabel *lp;
	{
	register int i;
	register struct partition *pp;
	char	junk[32];

	putchar('\n');
	if	(lp->d_type < DKMAXTYPES)
		printf("type: %s\n", dktypenames[lp->d_type]);
	else
		printf("type: %d\n", lp->d_type);
	strncpy(junk, lp->d_typename, sizeof (lp->d_typename));
	junk[sizeof(lp->d_typename)] = '\0';
	printf("disk: %s\n", junk);

	strncpy(junk, lp->d_packname, sizeof (lp->d_packname));
	junk[sizeof(lp->d_packname)] = '\0';
	printf("label: %s\n", junk);

	printf("flags:");
	if	(lp->d_flags & D_REMOVABLE)
		printf(" removeable");
	if	(lp->d_flags & D_ECC)
		printf(" ecc");
	if	(lp->d_flags & D_BADSECT)
		printf(" badsect");
	printf("\n");
	printf("bytes/sector: %d\n", lp->d_secsize);
	printf("sectors/track: %d\n", lp->d_nsectors);
	printf("tracks/cylinder: %d\n", lp->d_ntracks);
	printf("sectors/cylinder: %d\n", lp->d_secpercyl);
	printf("cylinders: %d\n", lp->d_ncylinders);
	printf("rpm: %d\n", lp->d_rpm);
	printf("drivedata: ");
	for	(i = 0; i < NDDATA; i++)
		printf("%D ", lp->d_drivedata[i]);
	printf("\n\n%d partitions:\n", lp->d_npartitions);
	printf("#        size   offset    fstype   [fsize bsize]\n");
	pp = lp->d_partitions;
	for	(i = 0; i < lp->d_npartitions; i++, pp++)
		{
		if	(pp->p_size == 0)
			continue;
		printf("  %c: %D %D  ", 'a' + i,
			pp->p_size, pp->p_offset);
		if	((unsigned) pp->p_fstype < FSMAXTYPES)
			printf("%s", fstypenames[pp->p_fstype]);
		else
			printf("%d", pp->p_fstype);
		switch	(pp->p_fstype)
			{
			case FS_V71K:
			case FS_UNUSED:
				printf("    %d %d ", pp->p_fsize,
					pp->p_fsize * pp->p_frag);
				break;
			default:
				printf("               ");
				break;
			}
		printf("\t# (Cyl. %D", pp->p_offset / lp->d_secpercyl);
		if	(pp->p_offset % lp->d_secpercyl)
			putchar('*');
		else
			putchar(' ');
		printf("- %D",
			(pp->p_offset + 
			pp->p_size + lp->d_secpercyl - 1) /
			lp->d_secpercyl - 1);
		if	(pp->p_size % lp->d_secpercyl)
			putchar('*');
		printf(")\n");
		}
	putchar('\n');
	}
