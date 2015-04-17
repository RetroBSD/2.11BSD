
#include <stdio.h>
#include <netdb.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/socket.h>

main()
	{
	register int	net;

	net = socket(PF_INET, SOCK_DGRAM, 0);
	if	(net < 0)
		{
		puts("NO");
		exit(EX_OSERR);
		}
	puts("YES");
	exit(EX_OK);
	}
