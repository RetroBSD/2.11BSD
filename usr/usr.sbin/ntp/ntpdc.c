#if !defined(lint) && defined(DOSCCS)
static char *RCSid = "$Source: /usr/src/new/ntp/ntpdc.c,v $ $Revision: 3.4.1.8 $ $Date: 95/01/27 17:31:26 $";
#endif

/*
 * $Log:	ntpdc.c,v $
 * Revision 3.4.1.8  95/01/27  17:31:26  sms
 * Fix name clockinfo name collision with sysctl.h
 *
 * Revision 3.4.1.7  89/05/18  18:31:26  louie
 * A few cosmetic changes for ntpd.c
 * 
 * Revision 3.4.1.6  89/05/03  15:17:27  louie
 * ntpdc now will display addional peer flags which indicate how far through
 * the clock selection process a peer was considered.
 * 
 * Revision 3.4.1.5  89/04/08  10:38:06  louie
 * Minor cosmetic changes and removed dead debug code from ntpd.c
 * 
 * Revision 3.4.1.4  89/03/29  12:41:56  louie
 * Check for success sending query before trying to listen for answers.  Will 
 * catch case of no server running and an ICMP port unreachable being returned.
 * 
 * Revision 3.4.1.3  89/03/22  18:29:53  louie
 * patch3: Use new RCS headers.
 * 
 * Revision 3.4.1.2  89/03/22  18:04:18  louie
 * Display dispersion in milliseconds.  The peer->refid field was being ntohl()'ed
 * when it should have stayed in network byte order.
 * 
 * Revision 3.4.1.1  89/03/20  00:13:41  louie
 * patch1: Delete unused variables.  Display interface address in numeric form
 * patch1: for local address, rather than symbolically.  For multiple host
 * patch1: queries, the name of the host is emitted prior to the data for that
 * patch1: host.
 * 
 * Revision 3.4  89/03/17  18:37:16  louie
 * Latest test release.
 * 
 * Revision 3.3.1.1  89/03/17  18:27:43  louie
 * Fix version number mismatch error message.
 * 
 * Revision 3.3  89/03/15  14:20:00  louie
 * New baseline for next release.
 * 
 * Revision 3.2.1.2  89/03/15  14:03:02  louie
 * The logical used to receive replies has been revised considerably.  Each packet
 * in the reply from the ntpd program carries the total number of packets in the
 * reply as well as a sequence number for this packet.  Thus, we know how many
 * packets to expect, and which one's we're received already.  A new UDP socket
 * is used for each host to prevent the replies from being mixed.  This was
 * a problem when querying an old ntpd which returned 7 bad version packets..
 * Use "%f" rather than "%lf" in format strings.
 * 
 * Revision 3.2.1.1  89/03/10  12:28:24  louie
 * Clean up output fomatting somewhat.
 * 
 * Revision 3.2  89/03/07  18:27:52  louie
 * Cosmetic changes and bug fixes.  Note that this version is likely to be
 * slightly incompatible with previous versions because the definitions of
 * the flage bits (PEER_FL_*) in ntp.h have changed.
 * 
 * A future version of this program will have a considerably different
 * packet format when Version 2 support is added.
 * 
 * Revision 3.1.1.1  89/02/15  09:01:39  louie
 * Bugfixes to previous release version.
 * 
 * 
 * Revision 3.1  89/01/30  14:43:16  louie
 * Second UNIX NTP test release.
 * 
 * Revision 3.0  88/12/12  15:57:28  louie
 * Test release of new UNIX NTP software.  This version should conform to the
 * revised NTP protocol specification.
 * 
 */

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include "ntp.h"

#define	WTIME	10		/* Time to wait for all responses */
#define	STIME	500000		/* usec to wait for another response */
#define	MAXPACKETSIZE 1500

extern int errno;
int debug;
int s;
int timedout, timeout();
int nflag, vflag, tflag;

struct sockaddr_in sin = {AF_INET};

char packet[MAXPACKETSIZE];
#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN	64
#endif
char	LocalHostName[MAXHOSTNAMELEN+1];	/* our hostname */
char	*LocalDomain;		/* our local domain name */


main(argc, argv)
	int argc;
	char *argv[];
{
	char *p;
#ifdef	pdp11
	int on = 4*1024;
#else
	int on = 48*1024;
#endif

	(void) gethostname(LocalHostName, sizeof LocalHostName);
	if (p = index(LocalHostName, '.')) {
		*p++ = '\0';
		LocalDomain = p;
	}
	else
		LocalDomain = "";

	if (argc < 2) {
usage:
		printf("usage: %s [ -v ][ -n ] hosts...\n", argv[0]);
		exit(1);
	}

	argv++, argc--;
	if (*argv[0] == '-') {
		switch (argv[0][1]) {
		case 'n':
			nflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			goto usage;
		}
		argc--, argv++;
	}
	if (argc > 1)
		printf("--- %s ---\n", *argv);

	while (argc > 0) {
		/*
		 * Get a new socket each time - this will cause us to ignore
		 * packets from the previously queried host.
		 */
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
			perror("socket");
			exit(2);
		}
#ifdef	SO_RCVBUF
		if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &on, sizeof (on)) < 0) {
			fprintf(stderr, "setsockopt SO_RCVBUF\n");
		}
#endif
		if (query(*argv))
			answer(*argv);
		close(s);
		argv++;
		if (argc-- > 1)
			printf("--- %s ---\n", *argv);
	}

}

answer(host)
	char *host;
{
	register struct ntpinfo *msg = (struct ntpinfo *) packet;
	register struct xclockinfo *n;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	int count, cc;
	fd_set bits;
	struct timeval shorttime;
	int first = 1;
	long replies = 0;

	/*
	 * Listen for returning packets; may be more than one packet per
	 * host. 
	 */
	FD_ZERO(&bits);
	FD_SET(s, &bits);
	shorttime.tv_sec = 0;
	shorttime.tv_usec = STIME;
	(void) signal(SIGALRM, timeout);
	(void) alarm(WTIME);
	timedout = 0;
	while ((first || replies) && 
	       (!timedout || select(FD_SETSIZE, &bits, (fd_set *) 0,
				    (fd_set *) 0, &shorttime) > 0)) {
		if ((cc = recvfrom(s, packet, sizeof(packet), 0,
			     (struct sockaddr *)&from, &fromlen)) <= 0) {
			if (cc == 0 || errno == EINTR)
				continue;
			fflush(stdout);
			perror(host);
			(void) close(s);
			return;
		}
		FD_SET(s, &bits);

		if (msg->type != INFO_REPLY)
			return;

		if (msg->version != NTPDC_VERSION) {
			printf("ntpd(%d) - ntpdc(%d) version mismatch\n",
			       msg->version, NTPDC_VERSION);
			alarm(0);
			return;
		}

		if (first) {
			first = 0;
			replies = (1L << msg->npkts) - 1;
			if (!vflag) {
				if (tflag)
					printf("    Address      Reference     Strat Poll Reach    Delay   Offset    Disp\n");
				else
					printf("   (rem)  Address   (lcl)      Strat Poll Reach    Delay   Offset    Disp\n");
				printf("==========================================================================\n");
			}
		}
		replies &= ~(1L << msg->seq);
		n = (struct xclockinfo *)&msg[1];
		for (count = msg->count; count > 0; count--) {
			if(vflag)
				print_verbose(n);
			else
				print_terse(n);
			n++;
		}
	}
	alarm(0);
	if (replies)
		printf("Timed out waiting for replies\n");
}

int
query(host)
	char *host;
{
	struct sockaddr_in watcher;
	register struct ntpdata *msg = (struct ntpdata *) packet;
	struct hostent *hp;
	static struct servent *sp = NULL;
	long HostAddr;

	bzero((char *) &watcher, sizeof(watcher));
	watcher.sin_family = AF_INET;
	HostAddr = inet_addr(host);
	watcher.sin_addr.s_addr = (u_long) HostAddr;
	if (HostAddr == -1) {
		hp = gethostbyname(host);
		if (hp == 0) {
			fprintf(stderr,"%s: unknown\n", host);
			return 0;
		}
		bcopy(hp->h_addr, (char *) &watcher.sin_addr, hp->h_length);
	}
	sp = getservbyname("ntp", "udp");
	if (sp == 0) {
		fprintf(stderr,"udp/ntp: service unknown, using default %d\n",
			NTP_PORT);
		watcher.sin_port = htons(NTP_PORT);
	} else
		watcher.sin_port = sp->s_port;
	msg->status = NTPVERSION_1;
	msg->stratum = INFO_QUERY;
	if (connect(s, (struct sockaddr *) &watcher, sizeof(watcher))) {
		perror("connect");
		return 0;
	}
	if (send(s, packet, sizeof(struct ntpdata), 0) < 0) {
		perror("send");
		return 0;
	}
	return 1;
}

timeout()
{
	timedout = 1;
}

print_terse (n)
	struct xclockinfo *n;
{
	int i;
	double offset[PEER_SHIFT], delay[PEER_SHIFT], dsp,del,off;
	char c;
	char *cvthname();
	int flags;

	sin.sin_addr.s_addr = n->net_address;
	for (i = 0; i < PEER_SHIFT; i++) {
		delay[i] = (double) ((long) (ntohl(n->info_filter.delay[i])/1000.0));
		offset[i] = (double) ((long) (ntohl(n->info_filter.offset[i])/1000.0));
	}
	dsp = (long) ntohl(n->estdisp);		/* leave in milliseconds */
	del = (long) ntohl(n->estdelay);	/* leave in milliseconds */
	off = (long) ntohl(n->estoffset);	/* leave in milliseconds */
	c = ' ';
	flags = ntohs(n->flags);
	if (flags & PEER_FL_CONFIG)
		c = '-';		/* mark pre-configured */
	if (flags & PEER_FL_SANE)
		c = '.';		/* passed sanity check */
	if (flags & PEER_FL_CANDIDATE)
		c = '+';		/* made candidate list */
	if (flags & PEER_FL_SELECTED)
		c = '*';		/* mark peer selection */
	sin.sin_addr.s_addr = n->net_address;
	printf("%c%-15.15s ", c, cvthname(&sin));
	if (tflag) {
		if (n->stratum == 1 || n->stratum == 0) {
			printf("%-4.4s             ", (char *) &n->refid);
		} else {
			sin.sin_addr.s_addr = (u_long) n->refid;
			printf("%-16.16s ", cvthname(&sin));
		}
	} else {
		sin.sin_addr.s_addr = n->my_address;
		printf("%-16.16s ", sin.sin_addr.s_addr ?
		       inet_ntoa(sin.sin_addr) : "wildcard");
	}
	printf("%2d %4d  %03o  %8.1f %8.1f %8.1f\n",
	       n->stratum, (int)ntohl((u_long)n->timer), 
	       ntohs(n->reach) & SHIFT_MASK, del, off, dsp);
}	

print_verbose(n)
	struct xclockinfo *n;
{
	int i;
	struct in_addr clock_host;
	double offset[PEER_SHIFT], delay[PEER_SHIFT], dsp,del,off;
	char *cvthname();

	sin.sin_addr.s_addr = n->net_address;
	for (i = 0; i < PEER_SHIFT; i++) {
		delay[i] = (double) (long) ntohl(n->info_filter.delay[i]);
		offset[i] = (double) (long) ntohl(n->info_filter.offset[i]);
	}
	dsp = (double) ((long) ntohl(n->estdisp));	/* in milliseconds */
	del = (double) ((long) ntohl(n->estdelay));	/* in milliseconds */
	off = (double) ((long) ntohl(n->estoffset));	/* in milliseconds */
	printf("Neighbor address %s port:%d",
	       inet_ntoa(sin.sin_addr), (int)ntohs(n->port));
	sin.sin_addr.s_addr = n->my_address;
	printf("  local address %s\n", inet_ntoa(sin.sin_addr));
	printf("Reach: 0%o stratum: %d, precision: %d\n",
	       ntohs(n->reach) & SHIFT_MASK, n->stratum, n->precision);
	printf("dispersion: %f, flags: %x, leap: %x\n",
	       dsp,
	       ntohs(n->flags),
	       n->leap);
	if (n->stratum == 1 || n->stratum == 0) {
		printf("Reference clock ID: %.4s", (char *)&n->refid);
	} else {
		clock_host.s_addr = (u_long) n->refid;
		printf("Reference clock ID: [%s]", inet_ntoa(clock_host));
	}
	printf(" timestamp: %08lx.%08lx\n", ntohl(n->reftime.int_part),
	       ntohl(n->reftime.fraction));

	printf("hpoll: %d, ppoll: %d, timer: %d, sent: %d received: %d\n",
	       n->hpoll, n->ppoll,
	       (int)ntohl((u_long)n->timer),
	       (int)ntohl(n->pkt_sent),
	       (int)ntohl(n->pkt_rcvd));
	printf("Delay(ms)  ");
	for (i = 0; i < PEER_SHIFT; i++)
		printf("%7.2f ", delay[i]);
	printf("\n");
	printf("Offset(ms) ");
	for (i = 0; i < PEER_SHIFT; i++)
		printf("%7.2f ", offset[i]);
	printf("\n");
	printf("\n\tdelay: %f offset: %f dsp %f\n", del, off, dsp);
	printf("\n");
}
/*
 * Return a printable representation of a host address.
 */
char *
cvthname(f)
	struct sockaddr_in *f;
{
	struct hostent *hp;
	register char *p;
	extern char *inet_ntoa();

	if (f->sin_family != AF_INET) {
		printf("Malformed from address\n");
		return ("???");
	}
	if (!nflag)
		hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr),
				   f->sin_family);
	else
		return (inet_ntoa(f->sin_addr));

	if (hp == 0)
		return (inet_ntoa(f->sin_addr));

	if ((p = index(hp->h_name, '.')) && strcmp(p + 1, LocalDomain) == 0)
		*p = '\0';
	return (hp->h_name);
}
