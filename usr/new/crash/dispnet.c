/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DO_SCCS) && !defined(lint)
static char sccsid[] = "@(#)dispnet.c	5.6 (Berkeley) 8/22/87";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <netdb.h>

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#define	KERNEL			/* to get routehash and RTHASHSIZ */
#include <net/route.h>
#undef	KERNEL
#include <net/if.h>
#include <net/raw_cb.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>

#include <arpa/inet.h>

#include <netinet/ip_var.h>
#include <netinet/tcp.h>

#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <sys/un.h>
#include <sys/unpcb.h>
#include "crash.h"
#include "net.h"

char	*subhead;			/* pntr to sub-heading */
struct arenas	arenash = {0};	/* list head */
long lseek();
char	*hoststr(),
	*sprintf();
char DASHES[] = "---------";
char FWARN[] = "\tWARNING:";
char Fccd[] = "%schar count discrepancy in %s queue; survey gives %d\n";

/*
 * Header portion of struct mbufx, defined in mbuf.h
 */
struct mbufxh {				/* external part of mbuf */
	struct	mbuf *mh_mbuf;		/* back pointer to mbuf */
	short	mh_ref;			/* reference counter */
};

char *mbtmsgs[] = {
	"bget(cache)     ",
	"bget(alloc)     ",
	"           bfree",
	"ioget           ",
	"sget            ",
	"           sfree",
	"MBXGET          ",
	"         MBXFREE",
	"MGET            ",
	"           MFREE",
};
#define	NMBTMSGS	10

/*
 * The following defines are taken from mbuf11.c.  They should
 * be in an include file.
 */

struct mbuf	*mbcache[256];		/* mbuf cache for fast allocation */
u_int		mbfree;
u_int		mbend;

/* End of stuff taken from mbuf11.c */

struct mbstat	mbstat;
struct ipstat	ipstat;
int in_ckodd;			/* from in_cksum.c */

struct	prname {
	int	pr_id;
	char	*pr_name;
};

struct	prname inetprotos[] = {
	{ IPPROTO_ICMP,	"icmp" },
	{ IPPROTO_GGP,	"ggp", },
	{ IPPROTO_TCP,	"tcp", },
	{ IPPROTO_EGP,	"egp", },
	{ IPPROTO_PUP,	"pup", },
	{ IPPROTO_UDP,	"udp", },
	{ IPPROTO_IDP,	"idp", },
	{ IPPROTO_RAW,	"raw", },
	0
};

struct	prname impprotos[] = {
	{ IMPLINK_IP,	"ip" },
	0
};

struct	pfname {
	int	pf_id;
	char	*pf_name;
	struct	prname *pf_protos;
} pfnames[] = {
	{ PF_UNSPEC,	"unspec",	0 },
	{ PF_UNIX,	"unix",		0 },
	{ PF_INET,	"inet",		inetprotos },
	{ PF_IMPLINK,	"implink",	impprotos },
	{ PF_PUP,	"pup",		0 },
	{ PF_CHAOS,	"chaos",	0 },
	{ PF_NS,	"ns",		0 },
	{ PF_NBS,	"nbs",		0 },
	{ PF_ECMA,	"ecma",		0 },
	{ PF_DATAKIT,	"datakit",	0 },
	{ PF_CCITT,	"ccitt",	0 },
	{ PF_SNA,	"sna",		0 },
	{ PF_DECnet,	"DECnet",	0 },
	{ PF_DLI,	"dli",		0 },
	{ PF_LAT,	"lat",		0 },
	{ PF_HYLINK,	"hylink",	0 },
	{ PF_APPLETALK,	"apple",	0 },
	0
};

int	internetprint();

struct	sockname {
	int	sa_id;
	char	*sa_name;
	int	(*sa_printer)();
} addrnames[] = {
	{ AF_UNSPEC,	"unspec",	0 },
	{ AF_UNIX,	"unix",		0 },
	{ AF_INET,	"inet",		internetprint },
	{ AF_IMPLINK,	"implink",	internetprint },
	{ AF_PUP,	"pup",		0 },
	{ AF_CHAOS,	"chaos",	0 },
	{ AF_NS,	"ns",		0 },
	{ AF_NBS,	"nbs",		0 },
	{ AF_ECMA,	"ecma",		0 },
	{ AF_DATAKIT,	"datakit",	0 },
	{ AF_CCITT,	"ccitt",	0 },
	{ AF_SNA,	"sna",		0 },
	{ AF_DECnet,	"DECnet",	0 },
	{ AF_DLI,	"dli",		0 },
	{ AF_LAT,	"lat",		0 },
	{ AF_HYLINK,	"hylink",	0 },
	{ AF_APPLETALK,	"apple",	0 },
	0
};

int	kmem,			/* Global FD for core file */
	line;
int	nflag = 0;		/* Ineternet addresses as 1.2.3.4 ? */
int	aflag = 1;		/* Display all connections ? */
int	Aflag = 1;		/* Display pcb address ? */
int	tflag = 1;		/* Display interface timer ? */
char	*interface = NULL;	/* interface name */
int	unit;			/* interface unit */

struct fetch fetchnet[] = {
	"_mbstat",	(char *) &mbstat,	sizeof mbstat,
	"_mbfree",	(char *) &mbfree,	sizeof mbfree,
	"_mbend",	(char *) &mbend,	sizeof mbend,
	"_mbcache",	(char *) mbcache,	sizeof mbcache,
	"_alloct",	(char *) &alloct,	sizeof alloct,
	"_in_ckod",	(char *) &in_ckodd,	sizeof in_ckodd,
	END
};

struct display net_tab[] = {
	"\nMBUFS:\tfree (low water mark)", (char *) &(mbstat.m_mbufs),DEC,0,
	"\tfree (current)",		(char *) &(mbstat.m_mbfree),	DEC,0,
	"\n\tmbufs in mbuf cache",	(char *) &mbstat.m_incache,	DEC,0,
	"\tdrops",			(char *) &(mbstat.m_drops),	DEC,0,
	"\nMBUFX area:\tStart click",	(char *) &mbstat.m_mbxbase,	MAD,0,
	"\tEnd click",			(char *) &mbend,		MAD,0,
	"\n\tSize in bytes",		(char *) &mbstat.m_mbxsize,	MAD,0,
	"\tmbufx free list (mbfree)",	(char *) &mbfree,		MAD,0,
	"\nARENA:\tallocs",		(char *) &allocs,		MAD,0,
	"\talloct",			(char *) &alloct,		MAD,0,
	"\n\tchars free (low water mark)",(char *) &(mbstat.m_clusters),DEC,0,
	"\tchars free (current)",	(char *) &(mbstat.m_clfree),	DEC,0,
	"\nMIO area:\tstart of free area", (char *) &mbstat.m_iobase,	MAD,0,
	"\tchars free",			(char *) &mbstat.m_iosize,	MAD,0,
	"\nMISC:\tin_ckodd",		(char *) &in_ckodd,		DEC,0,
	END
};

struct display mbuf_tab[] = {
	"\nnext",	(char *) &((struct mbuf *)0)->m_next,	MAD,0,
	"\toff",	(char *) &((struct mbuf *)0)->m_off,	MAD,0,
	"\tlen",	(char *) &((struct mbuf *)0)->m_len,	MAD,0,
	"\ttype",	(char *) &((struct mbuf *)0)->m_type,	MAD,0,
	"\tclick",	(char *) &((struct mbuf *)0)->m_click,	MAD,0,
	"\tact",	(char *) &((struct mbuf *)0)->m_act,	MAD,0,
	END
};

struct display mbufx_tab[] = {
	"\tm_mbuf",	(char *) &((struct mbufxh *)0)->mh_mbuf,MAD,0,
	"\tm_ref",	(char *) &((struct mbufxh *)0)->mh_ref,	DEC,0,
	END
};

/*
 * Display mbuf status info
 */
dispnet()
{
	char arena[NWORDS*2];

	subhead = "Network Data";
	arenap = &arena[0];		/* fill in static variables */
	fetch(fetchnet);
	allocs = findv("_allocs");	/* fill in automatic variables */
	vf("_allocs",(char *)arena, sizeof arena);
	newpage();
	display(net_tab,0);
	putchar('\n');			/* misc checks */
	if (alloct-allocs != (NWORDS-1)*2)
		printf("%s allocs/alloct inconsistent. Recompile ???\n", FWARN);
	if (mbfree != 0 && !VALMBXA(mbfree))
		printf("%s mbfree pointer bad\n", FWARN);
	if (mbstat.m_incache < 0 || mbstat.m_incache >= mbstat.m_totcache)
		printf("%s mbcache pointer bad\n", FWARN);

	arenack();
	mbufpr();

	newpage();
	protopr((off_t)findv("_tcb"), "tcp");
	protopr((off_t)findv("_udb"), "udp");
	rawpr();
	tcp_stats((off_t)findv("_tcpstat"), "tcp");
	udp_stats((off_t)findv("_udpstat"), "udp");
	ip_stats((off_t)findv("_ipstat"), "ip");
	icmp_stats((off_t)findv("_icmpstat"), "icmp");

	intpr(0,(off_t)findv("_ifnet"));

	routepr((off_t)findv("_rthost"),
		(off_t)findv("_rtnet"),
		(off_t)findv("_rthashsize"));
	rt_stats((off_t)findv("_rtstat"));

#ifdef	INET
	queuepr("_ipintrq", "ip");
#endif	INET
	queuepr("_rawintr", "raw");

	orphans();
	clrars();	/* return memory to pool */
}

queuepr(qname, qlbl)
char *qname, *qlbl;
{
	struct	ifqueue	ifqueue;		/* packet input queue */

	putchar('\n');
	if(vf(qlbl, (char *) &ifqueue, sizeof ifqueue)) {
		chasedm(ifqueue.ifq_head,"input",ifqueue.ifq_len);
		printf("%s input queue:\n", qname);
		printf("--head=%o  tail=%o  len=%d  maxlen=%d  drops=%d\n",
			ifqueue.ifq_head, ifqueue.ifq_tail, ifqueue.ifq_len,
			ifqueue.ifq_maxlen, ifqueue.ifq_drops);
	}
}
 
rawpr()
{
	register struct rawcb *prev, *next;
	struct rawcb *pcbp;
	struct rawcb *arawcb;
	struct socket *sockp;
	struct protosw proto;
	struct rawcb rawcb;
	int rcvct;	/* number of mbufs in receive queue */
	int sndct;	/* number of mbufs in send queue */

	printf("raw:\n");
	if (!vf("_rawcb", (char *) &rawcb, sizeof rawcb)) return;
	arawcb = (struct rawcb *)findv("_rawcb");
	pcbp = &rawcb;
	prev = arawcb;
	while (pcbp->rcb_next != arawcb) {
		next = pcbp->rcb_next;
		if (!VALADD(next, struct rawcb)) {
			printf("bad rawcb address (");
			DUMP((unsigned)next);
			puts(")");
			break;
		}
		(putars((unsigned)next,sizeof(struct rawcb),
			AS_RAWCB))->as_ref++;
		pcbp = XLATE(next, struct rawcb *);
		if (pcbp->rcb_prev != prev) {
			puts("???");
			break;
		}
		printf("%6o ", next);
		if (!VALADD(pcbp->rcb_socket, struct socket)) {
			printf("bad socket address (");
			DUMP((unsigned)pcbp->rcb_socket);
			puts(")");
			break;
		}
		(putars((unsigned)pcbp->rcb_socket,sizeof(struct socket),
			AS_SOCK))->as_ref++;
		sockp = XLATE(pcbp->rcb_socket, struct socket *);
		klseek(kmem, (off_t)sockp->so_proto, 0);
		read(kmem, &proto, sizeof (proto));
		/* chase the chains of mbufs */
		rcvct = chasem(sockp->so_rcv.sb_mb,"recv",sockp->so_rcv.sb_cc);
		sndct = chasem(sockp->so_snd.sb_mb,"send",sockp->so_snd.sb_cc);
		printaddress(&pcbp->rcb_laddr);
		printaddress(&pcbp->rcb_faddr);
		printf(" %4d%3d %4d%3d    ",sockp->so_rcv.sb_cc, rcvct,
					    sockp->so_snd.sb_cc, sndct);
		printproto(&proto);
		putchar('\n');
		prev = next;
	}
}

mbufpr()
{
	struct mbufxh mbufx;
	register i, j, k;
	unsigned base;
	int count;
	int freect;

	line = 22;
	printf("\n\t\t\tMbuf dump\n\nmbcache:");
	for (i=0, count=0; i<mbstat.m_totcache; i++) {
		if ((count++)%4 == 0) {
			putchar('\n');
			line++;
		}
		DUMP((unsigned)mbcache[i]);
		fputs(VALMBA(mbcache[i]) ? "       " : "(bad)  ",stdout);
	}
	printf("\n\n");
#ifdef DIAGNOSTIC
{ char mbts[32];
	for(i=0, j=mbstat.m_tbindex; i<mbstat.m_tbcount; i++, j--) {
		if (j < 0)
			j = mbstat.m_tbcount - 1;
		k = (mbstat.m_tbuf[j].mt_type>>4)&0x0fff;
		printf("%3d %s %4x %4x(%2d) ", i,
			((k<NMBTMSGS) ? mbtmsgs[k] :
				sprintf(mbts, "%16d", k)),
			mbstat.m_tbuf[j].mt_arg,
			mbstat.m_tbuf[j].mt_pc,
			mbstat.m_tbuf[j].mt_type&0x0f);
		symbol(mbstat.m_tbuf[j].mt_pc, ISYM,
			mbstat.m_tbuf[j].mt_type&0x0f);
		putchar('\n');
	}
}
#endif DIAGNOSTIC
	/* print header */
	line += 6;
	puts("\n\n\t\t\tMbufs\n\n click  mbuf   ref  next    off   len    act  click(mbuf)\n\n");

	base = mbstat.m_mbxbase;
	for(i=0, freect=0; i<mbstat.m_total ; i++) {
		lseek(kmem, ((long)base<<6), 0);
		read(kmem, &mbufx, sizeof mbufx);
		DUMP(base);
		putchar(' ');
		DUMP((unsigned)mbufx.mh_mbuf);
		printf("%5d ",mbufx.mh_ref);
		/* check for validity */
		if (mbufx.mh_ref == 0) {
			/* free */
			freect++;
			printf( "%sfree" , DASHES);
			if (mbufx.mh_mbuf && !VALMBXA(mbufx.mh_mbuf))
				fputs(" (bad pointer)",stdout);
		} else if (!VALMBA(mbufx.mh_mbuf)) {
			printf( "%sbad mbuf pointer" , DASHES);
		} else {
			struct arenas *asp;
			register int j;
			int found = 0;

			asp = putars(mbufx.mh_mbuf,sizeof(struct mbuf),AS_MBUF);
			for (j=0; j<mbstat.m_totcache && j<mbstat.m_incache;
			    j++) {
				if(mbufx.mh_mbuf == mbcache[j]) {
					found++;
					break;
				}
			}
			if (found) {
				/* in free cache */
				freect++;
				asp->as_ref++;
				printf("%sin free cache",DASHES);
				if (mbufx.mh_ref != 1)
					fputs("--bad ref count",stdout);
			} else {
				struct mbuf *mbptr;

				mbptr = XLATE(mbufx.mh_mbuf, struct mbuf *);
				DUMP(mbptr->m_next);
				printf(" %5d ",mbptr->m_off);
				printf(" %5d ",mbptr->m_len);
				printf(" %5d ",mbptr->m_type);
				DUMP(mbptr->m_act);
				if ((mbptr->m_click) != base) {
					putchar(' ');
					DUMP(mbptr->m_click);
				}
			}
		}
		putchar('\n');
		line++;
		if (line > LINESPERPAGE) {
			newpage();
			puts("\n click  mbuf   ref  next    off   len    act  click(mbuf)\n");
		}
		base += (MSIZE >> 6);
	}
	if (freect != mbstat.m_mbfree) {
		printf("\n%s free count discrepancy--\n", FWARN);
		printf("\n\tmbufx survey is %d, mbstat.m_mbfree is %d\n",
			freect, mbstat.m_mbfree);
	}
}

/*
 * Dump mbuf information
 * Flag mbufs as free or unaccounted for
 */
orphans()
{
	register i;
	struct arenas *ap;

	puts("\n\t\tMbufs Unaccounted For\n\n\n addr    next    off   len   click   act\n");
	ap = &arenash;
	while (ap = ap->as_next) {
		struct mbuf *mbptr;

		if (ap->as_ref ||
		    (!(ap->as_flags & AS_MBUF) && 
		      ap->as_size != sizeof(struct mbuf)))
			continue;
		mbptr = XLATE(ap->as_addr, struct mbuf *);
		DUMP(ap->as_addr);
		putchar(' ');
		DUMP(mbptr->m_next);
		putchar(' ');
		DUMP(mbptr->m_off);
		printf(" %5d ",mbptr->m_len);
		DUMP(mbptr->m_click);
		putchar(' ');
		DUMP(mbptr->m_act);
		if (!(ap->as_flags & AS_MBUF))
			fputs(" no refs at all",stdout);
		putchar('\n');
	}
	puts("\n\t\tA Trip through the Arena\n");
	ap = &arenash;
	while (ap = ap->as_next) {
		if (ap->as_ref || (ap->as_flags & AS_MBUF) == AS_MBUF) continue;
		printf("addr=%4x size=%d flags=%4x  ", ap->as_addr, 
			ap->as_size, ap->as_flags);
		switch (ap->as_size) {
			case sizeof(struct ifnet):
				puts("ifnet");
				break;
			case sizeof(struct in_addr):
				puts("in_addr");
				break;
			case sizeof(struct in_ifaddr):
				puts("in_ifaddr");
				break;
			case sizeof(struct inpcb):
				puts("inpcb");
				break;
			case sizeof(struct ipasfrag):
				puts("ipasfrag");
				break;
			case sizeof(struct ipq):
				puts("ipq");
				break;
			case sizeof(struct mbuf):
				puts("mbuf");
				break;
#ifdef	notyet
			case sizeof(struct ns_ifaddr):
				puts("ns_ifaddr");
				break;
			case sizeof(struct nspcb):
				puts("nspcb");
				break;
			case sizeof(struct sppcb):
				puts("sppcb");
				break;
#endif
			case sizeof(struct protosw):
				puts("protosw");
				break;
			case sizeof(struct rawcb):
				puts("rawcb");
				break;
			case sizeof(struct rtentry):
				puts("rtentry");
				break;
			case sizeof(struct sockaddr_in):
				puts("sockaddr_in");
				break;
			case sizeof(struct socket):
				puts("socket");
				break;
			case sizeof(struct tcpcb):
				puts("tcpcb");
				break;
			case sizeof(struct tcpiphdr):
				puts("tcpiphdr");
				break;
			case sizeof(struct unpcb):
				puts("unpcb");
				break;
			default:
				puts("unknown size");
				break;
		}
	}
}

printproto(sp)
	register struct protosw *sp;
{
	register struct pfname *pf = pfnames;
	register struct prname *pr;

#ifdef	HACK
	while (pf->pf_name) {
		if (pf->pf_id == sp->pr_family) {
			printf("%7s/", pf->pf_name);
			if (pr = pf->pf_protos)
				while (pr->pr_name) {
					if (pr->pr_id == sp->pr_protocol) {
						printf("%-5s", pr->pr_name);
						return;
					}
					pr++;
				}
			printf("%-5d", sp->pr_protocol);
			return;
		}
		pf++;
	}
	printf("%7d/%-5d", sp->pr_family, sp->pr_protocol);
#endif	HACK
}

printaddress(sa)
register struct sockaddr *sa;
{
	register struct sockname *sn;

	for (sn = addrnames; sn->sa_name; sn++)
		if (sn->sa_id == sa->sa_family) {
			printf("%8s,", sn->sa_name);
			if (!sn->sa_printer) goto generic;
			(*sn->sa_printer)(sa);
			return;
		}
	printf("%8d,", sa->sa_family);
generic:
	printf("%-8s", "?");
}

internetprint(sin)
	register struct sockaddr_in *sin;
{
	inetprint(&sin->sin_addr, sin->sin_port, "tcp");
}

/*
 * Chase list of mbufs, using m_next and m_act fields.
 */
chasedm(mbufp,str,cc)
register struct mbuf *mbufp;
char *str;
int cc;
{
	register struct arenas *asp;
	int count = 0;
	int chcnt = 0;
	register struct mbuf *mbptr;	/* points into arena */

	while (mbufp) {
		count += chaseit(mbufp,str,&chcnt);
		if (!VALMBA(mbufp)) {
			printf("---------bad mbuf pointer in %s queue (=%o)\n",
				str,mbufp);
			break;
		}
		if (!(asp=getars(mbufp)) ||
		   (asp->as_flags&AS_MBUF) != AS_MBUF) {
			printf("---------mbuf in %s queue not in list (=%o)\n",
				str,mbufp);
			break;
		}
		mbptr = XLATE(mbufp, struct mbuf *);
		mbufp = mbptr->m_act;
	}
	if (cc != chcnt)
		printf( Fccd, DASHES, str, chcnt);
	return(count);
}

/*
 * Chase list of mbufs, using only m_next field.
 */
chasem(mbufp,str,cc)
register struct mbuf *mbufp;
char *str;
int cc;
{
	register struct arenas *asp;
	int count = 0;
	int chcnt = 0;
	register struct mbuf *mbptr;	/* points into arena */

	count = chaseit(mbufp, str, &chcnt);
	if (cc != chcnt)
		printf( Fccd, DASHES, str, chcnt);
	return(count);
}


chaseit(mbufp,str,achcnt)
register struct mbuf *mbufp;
char *str;
int *achcnt;
{
	register struct arenas *asp;
	int count = 0;
	register struct mbuf *mbptr;	/* points into arena */

	while (mbufp) {
		int err = 0;

		count++;
		if (!VALMBA(mbufp)) {
			printf("%sbad mbuf pointer in %s queue (=%o)\n",
				DASHES, str,mbufp);
			break;
		}
		else if (!(asp=getars(mbufp)) ||
		    (asp->as_flags&AS_MBUF) != AS_MBUF) {
			printf("%smbuf in %s queue not in list (=%o)\n",
				DASHES, str,mbufp);
			asp = putars(mbufp,sizeof(struct mbuf),AS_MBUF);
			mbptr = XLATE(mbufp, struct mbuf *);
			printf("%soff=%d len=%d type=%d act=%o click=%o",DASHES,
				mbptr->m_off, mbptr->m_len, mbptr->m_type,
				mbptr->m_act, mbptr->m_click);
			if(!VALMBXA(mbptr->m_click))
				printf("(bad)");
			putchar('\n');
		}
		asp->as_ref++;
		mbptr = XLATE(mbufp, struct mbuf *);
		if (mbptr->m_len < 0 || mbptr->m_len > MLEN) {
			printf("---------bad mbuf length in %s queue;\n",str);
			err++;
		}
		if (mbptr->m_off < MMINOFF || mbptr->m_off >MMAXOFF) {
			printf("---------bad mbuf offset in %s queue;\n",str);
			err++;
		}
		if (err)
		    printf("---------off=%d len=%d type=%d act=%o click=%o\n",
			mbptr->m_off, mbptr->m_len, mbptr->m_type,
			mbptr->m_act, mbptr->m_click);
		*achcnt += mbptr->m_len;
		mbufp = mbptr->m_next;
	}
	return(count);
}

clrars()
{
	register struct arenas *asp;
	register struct arenas *next = arenash.as_next;

	while (asp = next) {
		next = asp->as_next;
		free(asp);
	}
	arenash.as_next = (struct arenas *) 0;
}

/*
 * Put entry for arena block in arena statistics list
 * This version allows entries with the same address
 */
struct arenas *
putars(addr,size,flags)
register unsigned addr;
int size, flags;
{
	register struct arenas *asp = &arenash;
	register struct arenas *prev;
	unsigned temp;

/* calculate size and get busy flag from info in arena */

	temp = *(int *)(arenap + (unsigned)addr - allocs - 2);
	if (temp&01==0) /* make sure not marked as free */
		flags |= AS_FREE;
	if ((temp&~01) - addr != size)
		flags |= AS_BDSZ;

	while(1) {
		prev = asp;
		asp = asp->as_next;
		if (asp == 0 || addr < asp->as_addr) {
			if (!(prev->as_next = (struct arenas *)malloc(sizeof(struct arenas)))) barf("out of memory");
			prev = prev->as_next;
			prev->as_next = asp;
			prev->as_size = size;
			prev->as_ref = 0;
			prev->as_addr = addr;
			prev->as_flags = flags;
			return(prev);
		}
		if (addr == asp->as_addr && (!(asp->as_flags & ~AS_ARCK)) && asp->as_size == size && !asp->as_ref) {
			asp->as_flags |= flags;
			return(asp);
		}
	}
}

/*
 * Find arena stat structure in list
 * Return zero if not found.
 */
struct arenas *
getars(addr)
register unsigned addr;
{
	register struct arenas *asp = &arenash;

	while (asp=asp->as_next) {
		if (asp->as_addr == addr)
			return(asp);
	}
	return((struct arenas *)0);
}

/*
 * Check arena for consistency
 */
arenack()
{
	unsigned ptr = allocs;
	unsigned temp;
	unsigned next;
	int busy;
	int size;
	int fsize = 0;	/* free size, should equal mbstat.clfree */

	fputs("\narena check:",stdout);
	for (;;) {
		temp = *(int *)(ptr - allocs + (unsigned)arenap);
		busy = temp & 01;
		next = temp & ~01;
		if (next < allocs || next > alloct ||
			(ptr > next && (ptr != alloct || next != allocs))){
			printf("\n%s arena clobbered; location: ", FWARN);
			DUMP(ptr);
			fputs(" pointer: ",stdout);
			DUMP(temp);
			return;
		}
		if (ptr > next) {
			printf("\n\tfree count: %d.\n",fsize);
			return;
		}
		size = next - ptr -2;
		if (!busy)
			fsize += (size+2);
		else
			putars(ptr+2,size,AS_ARCK);
		ptr = next;
	}
}

char *
hoststr(addr)
long addr;
{
	struct hostent *hp;

	hp = gethostbyaddr((char *) &addr, sizeof addr, AF_INET);
	if (hp)
		return(hp->h_name);
	else
		return(inet_ntoa(addr));
}

char *
getnet(addr)
union {
	long a_long;
	char a_octet[4];
} addr;
{
	char buf[32];
	struct netent *np;

	np = getnetbyaddr(inet_netof(addr.a_long), AF_INET);
	if (np)
		return(np->n_name);
	else {
		sprintf(buf, "%u", (unsigned)addr.a_octet[0]);
		if((unsigned)addr.a_octet[0] >= 128) 
			sprintf(buf+strlen(buf), ".%u", (unsigned)addr.a_octet[1]);
		if((unsigned)addr.a_octet[0] >= 192) 
			sprintf(buf+strlen(buf), ".%u", (unsigned)addr.a_octet[2]);
		return(buf);
	}
}

char *
plural(n)
	long n;
{

	return (n != 1 ? "s" : "");
}

#ifdef	for_handy_reference
struct tcpcb {
	struct	tcpiphdr *seg_next;	/* sequencing queue */
	struct	tcpiphdr *seg_prev;
	short	t_state;		/* state of this connection */
	short	t_timer[TCPT_NTIMERS];	/* tcp timers */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	struct	mbuf *t_tcpopt;		/* tcp options */
	u_short	t_maxseg;		/* maximum segment size */
	char	t_force;		/* 1 if forcing out a byte */
	u_char	t_flags;
#define	TF_ACKNOW	0x01		/* ack peer immediately */
#define	TF_DELACK	0x02		/* ack, but try to delay it */
#define	TF_NODELAY	0x04		/* don't delay packets to coalesce */
#define	TF_NOOPT	0x08		/* don't use tcp options */
#define	TF_SENTFIN	0x10		/* have sent FIN */
	struct	tcpiphdr *t_template;	/* skeletal packet for transmit */
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq	snd_una;		/* send unacknowledged */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	snd_up;			/* send urgent pointer */
	tcp_seq	snd_wl1;		/* window update seg seq number */
	tcp_seq	snd_wl2;		/* window update seg ack number */
	tcp_seq	iss;			/* initial send sequence number */
	u_short	snd_wnd;		/* send window */
/* receive sequence variables */
	u_short	rcv_wnd;		/* receive window */
	tcp_seq	rcv_nxt;		/* receive next */
	tcp_seq	rcv_up;			/* receive urgent pointer */
	tcp_seq	irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq	rcv_adv;		/* advertised window */
/* retransmit variables */
	tcp_seq	snd_max;		/* highest sequence number sent
					 * used to recognize retransmits
					 */
/* congestion control (for source quench) */
	u_short	snd_cwnd;		/* congestion-controlled window */
/* transmit timing stuff */
	short	t_idle;			/* inactivity time */
	short	t_rtt;			/* round trip time */
	u_short max_rcvd;		/* most peer has sent into window */
	tcp_seq	t_rtseq;		/* sequence number being timed */
	short   t_srtt;                 /* smoothed round-trip time (*10) */
	u_short	max_sndwnd;		/* largest window peer has offered */
/* out-of-band data */
	char	t_oobflags;		/* have some */
	char	t_iobc;			/* input character */
#define	TCPOOB_HAVEDATA	0x01
#define	TCPOOB_HADDATA	0x02
};
#endif	for_handy_reference
