/*
** kernel/linux.c                   Linux 0.99.13q or later access functions
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 Nov 1993
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int k_open()
{
    return 0;
}


int k_getuid(struct in_addr *faddr, int fport,
	     struct in_addr *laddr, int lport,
	     int *uid)
{
    FILE *fp;
    long dummy;
    char buf[512];
    struct in_addr myladdr, myraddr;
    int mylport, myrport;

    fport = ntohs(fport);
    lport = ntohs(lport);
    
    /*
     ** Open the kernel memory device
     */
    if ((fp = fopen("/proc/net/tcp", "r"))==NULL)
    {
	return -1;
    }
    
    /* eat header */
    fgets(buf,sizeof(buf)-1,fp);
    while (fgets(buf, sizeof(buf)-1, fp))
    {
	if (sscanf(buf,"%d: %lX:%x %lX:%x %x %lX:%lX %x:%lX %lx %d",
		   &dummy, &myladdr, &mylport, &myraddr, &myrport,
		   &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
		   uid) == 12)
	{
	    if (myladdr.s_addr==laddr->s_addr && mylport==lport &&
		myraddr.s_addr==faddr->s_addr && myrport==fport)
	    {
		fclose(fp);
		return 0;
	    }
	}
    }

    fclose(fp);
    return -1;
}
