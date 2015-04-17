#include <sys/file.h>
#include "config.h"
#include "sh.h"

char *efilname = "/etc/tcsh6.00strgs" ;

void
mkprintf(a1,a2,a3,a4)
	int a1;
	int a2, a3, a4;
{
	char buf[256];
	int efil = -1;

	/* better not assume same file descriptor each time
	static int efil = -1;
	if (efil < 0) {
	    efil = open(efilname, O_RDONLY);
	    if (efil < 0) {
		    perror(efilname);
		    exit(2);
	    }
	}
	*/
	efil = open(efilname, O_RDONLY);
	if (efil < 0) {
		perror(efilname);
		exit(2);
	}
	if (lseek(efil, (long)a1,0) == -1) {
		printf("lseek of mkstr file failed\n"); 
		    perror(efilname);
		    exit(2);
	}
	if (read(efil, buf, 256) <= 0) {
		printf("read of mkstr file failed\n");
		    perror(efilname);
		    exit(2);
	}
	close(efil);
	xprintf(buf, a2, a3, a4);
}
