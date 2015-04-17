/*
 * Simple but effective hack to test the kernel.o file.
 * It takes on stdin the output from "netstat -f inet -n | grep ESTAB"
 * in either Solaris 2.x (non-standard formats can easily be converted
 * to this)
 *	laddr.lport faddr.fport .....
 * or BSD 4.x (the defacto standard netstat output):
 *	tcp <num> <num>  laddr.lport faddr.fport
 * format.
 *
 * The output must be numeric, as non-numeric output is truncated when
 * hostnames get too long and ambiguous.  And we don't want netstat to
 * first convert numbers to names and then this program to convert
 * names back to numbers.
 *
 * Casper Dik (casper@fwi.uva.nl)
 */
#include <stdio.h>
#include <ctype.h>
#ifdef sequent
#include <strings.h>
#define strrchr rindex
#else
#include <string.h>
#include <stdlib.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>

#include "identd.h"
#include "error.h"


#ifdef LOG_DAEMON
static int syslog_facility = LOG_DAEMON;
#endif

/*
 * To resolve external references that are usually resolved
 * from identd.c
 */
char *path_unix = NULL;
char *path_kmem = NULL;
int lport = 0;
int fport = 0;

int debug_flag = 1;
int syslog_flag = 1;


#ifdef sequent
char *strtok(str, dels)
    char *str;
    char *dels;
{
  static char *bufp;
  

  if (str)
    bufp = str;

  if (!bufp || !*bufp)
    return (char *) 0;;

  while (*bufp && index(dels, *bufp) != (char *) 0)
    ++bufp;

  str = bufp;
  
  while (*bufp && index(dels, *bufp) == (char *) 0)
    ++bufp;
  
  if (*bufp)
    *bufp++ = '\0';

  return str;
}
#endif

int
main()
{
    char buf[500];

    /*
    ** Open the connection to the Syslog daemon if requested
    */
    if (syslog_flag)
    {
#ifdef LOG_DAEMON
	openlog("identd_test", LOG_PID, syslog_facility);
#else
	openlog("identd_test", LOG_PID);
#endif
    }


    if (k_open() < 0) {
	fprintf(stderr,"k_open failed\n");
	exit(1);
    }
    while (fgets(buf,sizeof(buf),stdin)) {
	char *loc, *rem, *tmp;
	unsigned short lport, fport;
	struct in_addr faddr, laddr;
	int uid;
#ifdef ALLOW_FORMAT
	int pid;
	char *cmd, *cmd_and_args;
#endif
	struct passwd *pwd;
	char buf2[sizeof(buf)];

	strcpy(buf2,buf);

	loc = strtok(buf, " \t");
	if (strcmp(loc,"tcp") == 0) {
	    int i;
	    for (i = 0; i < 3; i++)
		loc = strtok(NULL, " \t");
	}
	rem = strtok(NULL, " \t");
	if (loc == NULL || rem == NULL) {
	    fprintf(stderr,"Malformed line: %s\n", buf2);
	    continue;
	}
	/* parse remote, local address */
	tmp = strrchr(loc,'.');
	if (tmp == NULL) {
	    fprintf(stderr,"Malformed line: %s\n", buf2);
	    continue;
	}
	*tmp++ ='\0';
	laddr.s_addr = inet_addr(loc);
	lport = atoi(tmp);

	tmp = strrchr(rem,'.');
	if (tmp == NULL) {
	    fprintf(stderr,"Malformed line: %s\n", buf2);
	    continue;
	}
	*tmp++ ='\0';
	fport = atoi(tmp);
	faddr.s_addr = inet_addr(rem);

	uid = -1;
#ifdef ALLOW_FORMAT
	pid = 0;
	cmd = "unknown";
	cmd_and_args = "unknown";
#endif

	if (k_getuid(&faddr, htons(fport), &laddr, htons(lport), &uid
#ifdef ALLOW_FORMAT
		, &pid
		, &cmd
		, &cmd_and_args
#endif
		) != 0) {
	    fprintf(stderr,"*unknown*\t%s\t%d\t\t%s\t%d\n", loc, lport, rem, fport);
	    continue;
	}

	pwd = getpwuid(uid);
	if (pwd)
	    printf("%-8.8s", pwd->pw_name);
	else
	    printf("%-8.8d", uid);
#ifdef ALLOW_FORMAT
	printf (" \t%-13s\t%-4d\t%-13s\t%-4d\tPID=%d\tCMD=%s\tCMD+ARG=%s\n", loc, lport, rem, fport, pid, cmd, cmd_and_args);
#else
	printf(" \t%s\t%d\t\t%s\t%d\n", loc, lport, rem, fport);
#endif
    }

    return 0;
}
