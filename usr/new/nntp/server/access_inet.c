#ifndef lint
static char	*sccsid = "@(#)access_inet.c	1.4	(Berkeley) 1/9/88";
#endif

#include "common.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef EXCELAN
#include <netdb.h>
#include <arpa/inet.h>
#endif

/*
 * inet_netnames -- return the network, subnet, and host names of
 * our peer process for the Internet domain.
 *
 *	Parameters:	"sock" is our socket, which we don't need.
 *			"sin" is a pointer to the result of
 *			a getpeername() call.
 *			"net_name", "subnet_name", and "host_name"
 *			are filled in by this routine with the
 *			corresponding ASCII names of our peer.
 *	Returns:	Nothing.
 *	Side effects:	None.
 */

inet_netnames(sock, sin, net_name, subnet_name, host_name)
	int			sock;
	struct sockaddr_in	*sin;
	char			*net_name;
	char			*subnet_name;
	char			*host_name;
{
#ifdef EXCELAN
	excelan_netnames(sock, sin, net_name, subnet_name, host_name);
#else
	static int		gotsubnetconf;
	u_long			net_addr;
	u_long			subnet_addr;
	struct hostent		*hp;
	struct netent		*np;

#ifdef SUBNET
	if (!gotsubnetconf) {
		if (getifconf() < 0) {
#ifdef SYSLOG
			syslog(LOG_ERR, "host_access: getifconf: %m");
#endif
			return;
		}
		gotsubnetconf = 1;
	}
#endif

	net_addr = inet_netof(sin->sin_addr);	/* net_addr in host order */
	np = getnetbyaddr(net_addr, AF_INET);
	if (np != NULL)
		(void) strcpy(net_name, np->n_name);
	else
		(void) strcpy(net_name,inet_ntoa(*(struct in_addr *)&net_addr));

#ifdef SUBNET
	subnet_addr = inet_snetof(sin->sin_addr.s_addr);
	if (subnet_addr == 0)
		subnet_name[0] = '\0';
	else {
		np = getnetbyaddr(subnet_addr, AF_INET);
		if (np != NULL)
			(void) strcpy(subnet_name, np->n_name);
		else
			(void) strcpy(subnet_name,
			    inet_ntoa(*(struct in_addr *)&subnet_addr));
	}
#else
	subnet_name[0] = '\0';
#endif SUBNET

	hp = gethostbyaddr((char *) &sin->sin_addr.s_addr,
		sizeof (sin->sin_addr.s_addr), AF_INET);
	if (hp != NULL)
		(void) strcpy(host_name, hp->h_name);
	else
		(void) strcpy(host_name, inet_ntoa(sin->sin_addr));
#endif
}


#ifdef EXCELAN
excelan_netnames(sock, sin, net_name, subnet_name, host_name)
	int			sock;
	struct sockaddr_in	*sin;
	char			*net_name;
	char			*subnet_name;
	char			*host_name;
{
	char *hp, *raddr();
	int octet[4];

	/* assumes sizeof addr is long */
	octet[0] = (sin->sin_addr.s_addr>>24)&0xff;
	octet[1] = (sin->sin_addr.s_addr>>16)&0xff;
	octet[2] = (sin->sin_addr.s_addr>>8 )&0xff;
	octet[3] = (sin->sin_addr.s_addr    )&0xff;

	hp = raddr(sin->sin_addr.s_addr);
	if (hp != NULL)
		(void) strcpy(host_name,hp);
	else

		(void) sprintf(host_name,"%d.%d.%d.%d",
			octet[0],octet[1],octet[2],octet[3]);	
	/* No inet* routines here, so we fake it. */
	if (octet[0] < 128)		/* CLASS A address */
		(void) sprintf(net_name,"%d",octet[0]);
	else if(octet[0] < 192)		/* CLASS B address */
			(void) sprintf(net_name,"%d.%d",
						octet[0],octet[1]);
		else 			/* CLASS C address */
			(void) sprintf(net_name,"%d.%d.%d",
					octet[0],octet[1],octet[2]);

/* hack to cover the subnet stuff */
	(void) sprintf(subnet_name,"%d.%d.%d",octet[0],octet[1],octet[2]);
}
#endif
