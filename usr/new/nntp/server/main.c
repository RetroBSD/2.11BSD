#ifdef EXCELAN
struct sockaddr_in current_peer = { AF_INET, IPPORT_NNTP };
#endif
#ifndef lint
static char	*sccsid = "@(#)main.c	1.10	(Berkeley) 2/6/88";
#endif

/*
 *	Network News Transfer Protocol server
 *
 *	Phil Lapsley
 *	University of California, Berkeley
 *	(Internet: phil@berkeley.edu; UUCP: ...!ucbvax!phil)
 */

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef EXCELAN
#include <netdb.h>
#endif
#include <signal.h>

main()
{

#ifdef ALONE	/* If no inetd */

	int			sockt, client, length;
	struct sockaddr_in	from;
	extern int 		reaper();

	disassoc();

	/* fd 0-2 should be open and point to / now. */

#ifdef SYSLOG
#ifdef BSD_42
	openlog("nntpd", LOG_PID);			/* fd 3 */
#else
	openlog("nntpd", LOG_PID, SYSLOG);		/* fd 3 */
#endif
#endif

#ifdef FASTFORK
	num_groups = read_groups();	/* Read active file now (fd 4) */
					/* and then do it every */
	set_timer();			/* so often later */
#endif

#ifndef EXCELAN
	sockt = get_socket();		/* should be fd 4 or 5 */
#endif

#ifndef USG
	(void) signal(SIGCHLD, reaper);
#endif

#ifndef EXCELAN
	if (listen(sockt, SOMAXCONN) < 0) {
#ifdef SYSLOG
		syslog(LOG_ERR, "main: listen: %m");
#endif
		exit(1);
	}
#endif

	for (;;) {
#ifdef EXCELAN
		int status;
		sockt = get_socket();
		if (sockt < 0)
			continue;
		client = accept(sockt, &from);
#else
		length = sizeof (from);
		client = accept(sockt, &from, &length);
#endif EXCELAN
		if (client < 0) {
#ifdef SYSLOG
			if (errno != EINTR)
				syslog(LOG_ERR, "accept: %m\n");
#endif
#ifdef EXCELAN
			close(sockt);
			sleep(1);
#endif
			continue;
		}

		switch (fork()) {
		case	-1:
#ifdef SYSLOG
				syslog(LOG_ERR, "fork: %m\n");
#endif
#ifdef EXCELAN
				(void) close(sockt);
#endif
				(void) close(client);
				break;

		case	0:
#ifdef EXCELAN
				if (fork())
					exit(0);
				bcopy(&from,&current_peer,sizeof(from));
				make_stdio(sockt);
#else
				(void) close(sockt);
				make_stdio(client);
#endif
				serve();
				break;

		default:
#ifdef EXCELAN
				(void) close(sockt);
				(void) wait(&status);
#else
				(void) close(client);
#endif
				break;
		}
	}

#else		/* We have inetd */

	serve();

#endif
}
