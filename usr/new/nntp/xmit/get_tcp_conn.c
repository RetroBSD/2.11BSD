/*
** Routines to open a TCP connection
**
** New version that supports the old (pre 4.2 BSD) socket calls,
** and systems with the old (pre 4.2 BSD) hostname lookup stuff.
** Compile-time options are:
**
**	USG		- you're on System III/V (you have my sympathies)
**	NONETDB		- old hostname lookup with rhost()
**	OLDSOCKET	- different args for socket() and connect()
**
** Erik E. Fair <fair@ucbarpa.berkeley.edu>
**
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdio.h>
#include "get_tcp_conn.h"
#ifndef	NONETDB
#include <netdb.h>
#endif	NONETDB

extern	int	errno;
extern	char	*Pname;
extern	char	*errmsg();
#ifndef	htons
extern	u_short	htons();
#endif	htons
#ifndef	NONETDB
extern	char	*inet_ntoa();
extern	u_long	inet_addr();
#else
/*
 * inet_addr for EXCELAN (which does not have it!)
 *
 */
u_long
inet_addr(cp)
register char	*cp;
{
	u_long val, base, n;
	register char c;
 	u_long octet[4], *octetptr = octet;
#ifndef	htonl
	extern	u_long	htonl();
#endif	htonl
again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		/*
		 * Internet format:
		 *	a.b.c.d
		 *	a.b.c	(with c treated as 16-bits)
		 *	a.b	(with b treated as 24 bits)
		 */
		if (octetptr >= octet + 4)
			return (-1);
		*octetptr++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp))
		return (-1);
	*octetptr++ = val;
	/*
	 * Concoct the address according to
	 * the number of octet specified.
	 */
	n = octetptr - octet;
	switch (n) {

	case 1:				/* a -- 32 bits */
		val = octet[0];
		break;

	case 2:				/* a.b -- 8.24 bits */
		val = (octet[0] << 24) | (octet[1] & 0xffffff);
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		val = (octet[0] << 24) | ((octet[1] & 0xff) << 16) |
			(octet[2] & 0xffff);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		val = (octet[0] << 24) | ((octet[1] & 0xff) << 16) |
		      ((octet[2] & 0xff) << 8) | (octet[3] & 0xff);
		break;

	default:
		return (-1);
	}
	val = htonl(val);
	return (val);
}

char *
inet_ntoa(in)
struct in_addr in;
{
	static char address[20];

	sprintf(address, "%u.%u.%u.%u",
			 (in.s_addr>>24)&0xff,
			 (in.s_addr>>16)&0xff,
			 (in.s_addr>>8 )&0xff,
			 (in.s_addr    )&0xff);
	return(address);
}
#endif	NONETDB

#ifdef	USG
void
bcopy(s, d, l)
register caddr_t s, d;
register int l;
{
	while (l-- > 0)	*d++ = *s++;
}
#endif	USG

/*
** Take the name of an internet host in ASCII (this may either be its
** official host name or internet number (with or without enclosing
** backets [])), and return a list of internet addresses.
**
** returns NULL for failure to find the host name in the local database,
** or for a bad internet address spec.
*/
u_long **
name_to_address(host)
char	*host;
{
	static	u_long	*host_addresses[2];
	static	u_long	haddr;

	if (host == (char *)NULL) {
		return((u_long **)NULL);
	}

	host_addresses[0] = &haddr;
	host_addresses[1] = (u_long *)NULL;

	/*
	** Is this an ASCII internet address? (either of [10.0.0.78] or
	** 10.0.0.78). We get away with the second test because hostnames
	** and domain labels are not allowed to begin in numbers.
	** (cf. RFC952, RFC882).
	*/
	if (*host == '[' || isdigit(*host)) {
		char	namebuf[128];
		register char	*cp = namebuf;

		/*
		** strip brackets [] or anything else we don't want.
		*/
		while(*host != '\0' && cp < &namebuf[sizeof(namebuf)]) {
			if (isdigit(*host) || *host == '.')
				*cp++ = *host++;	/* copy */
			else
				host++;			/* skip */
		}
		*cp = '\0';
		haddr = inet_addr(namebuf);
		return(&host_addresses[0]);
	} else {
#ifdef	NONETDB
		extern	u_long	rhost();

		/* lint is gonna bitch about this (comparing an unsigned?!) */
		if ((haddr = rhost(&host)) == FAIL)
			return((u_long **)NULL);	/* no such host */
		return(&host_addresses[0]);
#else
		struct hostent	*hstp = gethostbyname(host);

		if (hstp == NULL) {
			return((u_long **)NULL);	/* no such host */
		}

		if (hstp->h_length != sizeof(u_long))
			abort();	/* this is fundamental */
#ifndef	h_addr
		/* alignment problems (isn't dbm wonderful?) */
		bcopy((caddr_t)hstp->h_addr, (caddr_t)&haddr, sizeof(haddr));
		return(&host_addresses[0]);
#else
		return((u_long **)hstp->h_addr_list);
#endif	h_addr
#endif	NONETDB
	}
}

/*
** Get a service port number from a service name (or ASCII number)
**
** Return zero if something is wrong (that's a reserved port)
*/
#ifdef	NONETDB
static struct Services {
	char	*name;
	u_short	port;
} Services[] = {
	{"nntp",	IPPORT_NNTP},		/* RFC977 */
	{"smtp",	IPPORT_SMTP},		/* RFC821 */
	{"name",	IPPORT_NAMESERVER},	/* RFC881, RFC882, RFC883 */
	{"time",	IPPORT_TIMESERVER},	/* RFC868 */
	{"echo",	IPPORT_ECHO},		/* RFC862 */
	{"discard",	IPPORT_DISCARD},	/* RFC863 */
	{"daytime",	IPPORT_DAYTIME},	/* RFC867 */
	{"login",	IPPORT_LOGINSERVER},	/* N/A - 4BSD specific */
};
#endif	NONETDB

u_short
gservice(serv, proto)
char	*serv, *proto;
{
	if (serv == (char *)NULL || proto == (char *)NULL)
		return((u_short)0);

	if (isdigit(*serv)) {
		return(htons((u_short)(atoi(serv))));
	} else {
#ifdef	NONETDB
		register int	i;

		for(i = 0; i < (sizeof(Services) / sizeof(struct Services)); i++) {
			if (strcmp(serv, Services[i].name) == 0)
				return(htons(Services[i].port));
		}
		return((u_short)0);
#else
		struct servent	*srvp = getservbyname(serv, proto);

		if (srvp == (struct servent *)NULL)
			return((u_short)0);
		return((u_short)srvp->s_port);
#endif	NONETDB
	}
}

/*
** given a host name (either name or internet address) and service name
** (or port number) (both in ASCII), give us a TCP connection to the
** requested service at the requested host (or give us FAIL).
*/
get_tcp_conn(host, serv)
char	*host, *serv;
{
	register int	sock;
	u_long	**addrlist;
	struct sockaddr_in	sadr;
#ifdef	OLDSOCKET
	struct sockproto	sp;

	sp.sp_family	= (u_short)AF_INET;
	sp.sp_protocol	= (u_short)IPPROTO_TCP;
#endif	OLDSOCKET

	if ((addrlist = name_to_address(host)) == (u_long **)NULL) {
		return(NOHOST);
	}

	sadr.sin_family = (u_short)AF_INET;	/* Only internet for now */
	if ((sadr.sin_port = gservice(serv, "tcp")) == 0)
		return(NOSERVICE);

	for(; *addrlist != (u_long *)NULL; addrlist++) {
		bcopy((caddr_t)*addrlist, (caddr_t)&sadr.sin_addr,
			sizeof(sadr.sin_addr));

#ifdef	OLDSOCKET
		if ((sock = socket(SOCK_STREAM, &sp, (struct sockaddr *)NULL, 0)) < 0)
			return(FAIL);

		if (connect(sock, (struct sockaddr *)&sadr) < 0) {
#else
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			return(FAIL);

		if (connect(sock, (struct sockaddr *)&sadr, sizeof(sadr)) < 0) {
#endif	OLDSOCKET
			int	e_save = errno;

			fprintf(stderr, "%s: %s [%s]: %s\n", Pname, host,
				inet_ntoa(sadr.sin_addr), errmsg(errno));
			(void) close(sock);	/* dump descriptor */
			errno = e_save;
		} else
			return(sock);
	}
	return(FAIL);
}
