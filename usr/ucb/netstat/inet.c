/*
 * Copyright (c) 1983,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#if	defined(DOSCCS) && !defined(lint)
static char sccsid[] = "@(#)inet.c	5.9.3 (2.11BSD GTE) 8/28/94";
#endif

#include <strings.h>
#include <stdio.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <arpa/inet.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netdb.h>

struct	inpcb inpcb;
struct	tcpcb tcpcb;
struct	socket sockb;
extern	int kmem;
extern	int Aflag;
extern	int aflag;
extern	int nflag;
extern	int sflag;
extern	char *plural();

#ifdef pdp11
#define klseek slseek
#endif

char	*inetname();

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TCP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
protopr(off, name)
	off_t off;
	char *name;
{
	struct inpcb cb;
	register struct inpcb *prev, *next;
	int istcp;
	static int first = 1;

	if (off == 0)
		return;
	istcp = strcmp(name, "tcp") == 0;
	klseek(kmem, off, 0);
	read(kmem, (char *)&cb, sizeof (struct inpcb));
	inpcb = cb;
	prev = (struct inpcb *)off;
	if (inpcb.inp_next == (struct inpcb *)off)
		return;
	while (inpcb.inp_next != (struct inpcb *)off) {

		next = inpcb.inp_next;
		klseek(kmem, (off_t)next, 0);
		read(kmem, (char *)&inpcb, sizeof (inpcb));
		if (inpcb.inp_prev != prev) {
			printf("???\n");
			break;
		}
		if (!aflag &&
		  inet_lnaof(inpcb.inp_laddr.s_addr) == INADDR_ANY) {
			prev = next;
			continue;
		}
		klseek(kmem, (off_t)inpcb.inp_socket, 0);
		read(kmem, (char *)&sockb, sizeof (sockb));
		if (istcp) {
			klseek(kmem, (off_t)inpcb.inp_ppcb, 0);
			read(kmem, (char *)&tcpcb, sizeof (tcpcb));
		}
		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
				"Proto", "Recv-Q", "Send-Q",
				"Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			if (istcp)
				printf("%8x ", inpcb.inp_ppcb);
			else
				printf("%8x ", next);
		printf("%-5.5s %6d %6d ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		inetprint(&inpcb.inp_laddr, inpcb.inp_lport, name);
		inetprint(&inpcb.inp_faddr, inpcb.inp_fport, name);
		if (istcp) {
			if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
				printf(" %d", tcpcb.t_state);
			else
				printf(" %s", tcpstates[tcpcb.t_state]);
		}
		putchar('\n');
		prev = next;
	}
}

/*
 * Dump TCP statistics structure.
 */
tcp_stats(off, name)
	off_t off;
	char *name;
{
	struct tcpstat tcpstat;

	if (off == 0)
		return;
	printf ("%s:\n", name);
	klseek(kmem, off, 0);
	read(kmem, (char *)&tcpstat, sizeof (tcpstat));

#define	p(f, m)		printf(m, tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m)	printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2, plural(tcpstat.f2))
  
	p(tcps_sndtotal, "\t%ld packet%s sent\n");
	p2(tcps_sndpack,tcps_sndbyte,
		"\t\t%ld data packet%s (%ld byte%s)\n");
	p2(tcps_sndrexmitpack, tcps_sndrexmitbyte,
		"\t\t%ld data packet%s (%ld byte%s) retransmitted\n");
	p2(tcps_sndacks, tcps_delack,
		"\t\t%ld ack-only packet%s (%ld delayed)\n");
	p(tcps_sndurg, "\t\t%ld URG only packet%s\n");
	p(tcps_sndprobe, "\t\t%ld window probe packet%s\n");
	p(tcps_sndwinup, "\t\t%ld window update packet%s\n");
	p(tcps_sndctrl, "\t\t%ld control packet%s\n");
	p(tcps_rcvtotal, "\t%ld packet%s received\n");
	p2(tcps_rcvackpack, tcps_rcvackbyte, "\t\t%ld ack%s (for %D byte%s)\n");
	p(tcps_rcvdupack, "\t\t%ld duplicate ack%s\n");
	p(tcps_rcvacktoomuch, "\t\t%ld ack%s for unsent data\n");
	p2(tcps_rcvpack, tcps_rcvbyte,
		"\t\t%ld packet%s (%ld byte%s) received in-sequence\n");
	p2(tcps_rcvduppack, tcps_rcvdupbyte,
		"\t\t%ld completely duplicate packet%s (%ld byte%s)\n");
	p2(tcps_rcvpartduppack, tcps_rcvpartdupbyte,
		"\t\t%ld packet%s with some dup. data (%ld byte%s duped)\n");
	p2(tcps_rcvoopack, tcps_rcvoobyte,
		"\t\t%ld out-of-order packet%s (%ld byte%s)\n");
	p2(tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
		"\t\t%ld packet%s (%ld byte%s) of data after window\n");
	p(tcps_rcvwinprobe, "\t\t%ld window probe%s\n");
	p(tcps_rcvwinupd, "\t\t%ld window update packet%s\n");
	p(tcps_rcvafterclose, "\t\t%ld packet%s received after close\n");
	p(tcps_rcvbadsum, "\t\t%ld discarded for bad checksum%s\n");
	p(tcps_rcvbadoff, "\t\t%ld discarded for bad header offset field%s\n");
	p(tcps_rcvshort, "\t\t%ld discarded because packet too short\n");
	p(tcps_connattempt, "\t%ld connection request%s\n");
	p(tcps_accepts, "\t%ld connection accept%s\n");
	p(tcps_connects, "\t%D connection%s established (including accepts)\n");
	p2(tcps_closed, tcps_drops,
		"\t%ld connection%s closed (including %ld drop%s)\n");
	p(tcps_conndrops, "\t%ld embryonic connection%s dropped\n");
	p2(tcps_rttupdated, tcps_segstimed,
		"\t%ld segment%s updated rtt (of %ld attempt%s)\n");
	p(tcps_rexmttimeo, "\t%ld retransmit timeout%s\n");
	p(tcps_timeoutdrop, "\t\t%ld connection%s dropped by rexmit timeout\n");
	p(tcps_persisttimeo, "\t%ld persist timeout%s\n");
	p(tcps_keeptimeo, "\t%ld keepalive timeout%s\n");
	p(tcps_keepprobe, "\t\t%ld keepalive probe%s sent\n");
	p(tcps_keepdrops, "\t\t%ld connection%s dropped by keepalive\n");
#undef p
#undef p2
}

/*
 * Dump UDP statistics structure.
 */
udp_stats(off, name)
	off_t off;
	char *name;
{
	struct udpstat udpstat;

	if (off == 0)
		return;
	klseek(kmem, off, 0);
	read(kmem, (char *)&udpstat, sizeof (udpstat));
	printf("%s:\n", name);
#define	p(f, m) printf(m, udpstat.f, plural(udpstat.f))
	p(udps_hdrops, "\t%lu incomplete header%s\n");
	p(udps_badlen, "\t%lu bad data length field%s\n");
	p(udps_badsum, "\t%lu bad checksum%s\n");
	p(udps_noport, "\t%lu no port%s\n");
	p(udps_noportbcast, "\t%lu (arrived as bcast) no port%s\n");
#undef p
}

/*
 * Dump IP statistics structure.
 */
ip_stats(off, name)
	off_t off;
	char *name;
{
	struct ipstat ipstat;

	if (off == 0)
		return;
	klseek(kmem, off, 0);
	read(kmem, (char *)&ipstat, sizeof (ipstat));
#if BSD>=43
	printf("%s:\n\t%lu total packets received\n", name,
		ipstat.ips_total);
#endif
	printf("\t%lu bad header checksum%s\n",
		ipstat.ips_badsum, plural(ipstat.ips_badsum));
	printf("\t%lu with size smaller than minimum\n", ipstat.ips_toosmall);
	printf("\t%lu with data size < data length\n", ipstat.ips_tooshort);
	printf("\t%lu with header length < data size\n", ipstat.ips_badhlen);
	printf("\t%lu with data length < header length\n", ipstat.ips_badlen);
#if BSD>=43
	printf("\t%lu fragment%s received\n",
		ipstat.ips_fragments, plural(ipstat.ips_fragments));
	printf("\t%lu fragment%s dropped (dup or out of space)\n",
		ipstat.ips_fragdropped, plural(ipstat.ips_fragdropped));
	printf("\t%lu fragment%s dropped after timeout\n",
		ipstat.ips_fragtimeout, plural(ipstat.ips_fragtimeout));
	printf("\t%lu packet%s forwarded\n",
		ipstat.ips_forward, plural(ipstat.ips_forward));
	printf("\t%lu packet%s not forwardable\n",
		ipstat.ips_cantforward, plural(ipstat.ips_cantforward));
	printf("\t%lu redirect%s sent\n",
		ipstat.ips_redirectsent, plural(ipstat.ips_redirectsent));
#endif
}

static	char *icmpnames[] = {
	"echo reply",
	"#1",
	"#2",
	"destination unreachable",
	"source quench",
	"routing redirect",
	"#6",
	"#7",
	"echo",
	"#9",
	"#10",
	"time exceeded",
	"parameter problem",
	"time stamp",
	"time stamp reply",
	"information request",
	"information request reply",
	"address mask request",
	"address mask reply",
};

/*
 * Dump ICMP statistics.
 */
icmp_stats(off, name)
	off_t off;
	char *name;
{
	struct icmpstat icmpstat;
	register int i, first;

	if (off == 0)
		return;
	klseek(kmem, off, 0);
	read(kmem, (char *)&icmpstat, sizeof (icmpstat));
	printf("%s:\n\t%lu call%s to icmp_error\n", name,
		icmpstat.icps_error, plural(icmpstat.icps_error));
	printf("\t%lu error%s not generated 'cuz old message was icmp\n",
		icmpstat.icps_oldicmp, plural(icmpstat.icps_oldicmp));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
				icmpstat.icps_outhist[i]);
		}
	printf("\t%lu message%s with bad code fields\n",
		icmpstat.icps_badcode, plural(icmpstat.icps_badcode));
	printf("\t%lu message%s < minimum length\n",
		icmpstat.icps_tooshort, plural(icmpstat.icps_tooshort));
	printf("\t%lu bad checksum%s\n",
		icmpstat.icps_checksum, plural(icmpstat.icps_checksum));
	printf("\t%lu message%s with bad length\n",
		icmpstat.icps_badlen, plural(icmpstat.icps_badlen));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
				icmpstat.icps_inhist[i]);
		}
	printf("\t%lu message response%s generated\n",
		icmpstat.icps_reflect, plural(icmpstat.icps_reflect));
}

/*
 * Pretty print an Internet address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */
inetprint(in, port, proto)
	register struct in_addr *in;
	u_short port; 
	char *proto;
{
	struct servent *sp = 0;
	char line[80], *cp, *index();
	int width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(*in));
	cp = index(line, '\0');
	if (!nflag && port)
		sp = getservbyport((int)port, proto);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%u", ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give 
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(in)
	struct in_addr in;
{
	register char *cp;
	static char line[50];
	struct hostent *hp;
	struct netent *np;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && in.s_addr != INADDR_ANY) {
		u_long net = inet_netof(in);
		u_long lna = inet_lnaof(in);

		if (lna == INADDR_ANY) {
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp == 0) {
			hp = gethostbyaddr((char *)&in, sizeof (in), AF_INET);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
	}
	if (in.s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		in.s_addr = ntohl(in.s_addr);
#define C(x)	(u_char)((x) & 0xff)
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}
