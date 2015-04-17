#ifndef lint
static char	*sccsid = "@(#)profile.c	1.1	(Berkeley) 12/18/87";
#endif

#include <sys/types.h>
#include <sys/stat.h>

#define	MON	"gmon.out"
#define	DIR	"/usr/tmp/nntpd.prof"

profile()
{
	static char	tmp[] = "gmon.XXXXXX";
	struct stat	statbuf;

	if (chdir(DIR) < 0)
		return;

	if (stat(MON, statbuf) < 0)
		return;

	(void) mktemp(tmp);

	(void) rename(MON, tmp);
}
