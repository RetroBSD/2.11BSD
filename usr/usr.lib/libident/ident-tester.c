/*
** ident-tester.c           A small daemon that can be used to test Ident
**                          servers
**
** Author: Peter Eriksson <pen@lysator.liu.se>, 10 Aug 1992
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>

#define IN_LIBIDENT_SRC
#include "ident.h"

#include <arpa/inet.h>

/*
** Return the name of the connecting host, or the IP number as a string.
*/
char *gethost __P1(struct in_addr *, addr)
{
  struct hostent *hp;

  
  hp = gethostbyaddr((char *) addr, sizeof(struct in_addr), AF_INET);
  if (hp)
    return (char *) hp->h_name;
  else
    return inet_ntoa(*addr);
}

void
main __P2(int, argc,
	  char **, argv)
{
  struct sockaddr_in laddr, faddr;
  int len, res, lport, fport;
  ident_t *id;
  char *identifier, *opsys, *charset;
  

  puts("Welcome to the IDENT server tester, version 1.9\r\n");
  printf("(Linked with libident-%s)\r\n\n", id_version);
  
  fflush(stdout);
  
  len = sizeof(faddr);
  getpeername(0, (struct sockaddr *) &faddr, &len);

  len = sizeof(laddr);
  getsockname(0, (struct sockaddr *) &laddr, &len);

  printf("Connecting to Ident server at %s...\r\n", inet_ntoa(faddr.sin_addr));
  fflush(stdout);

#ifdef LOG_LOCAL3
  openlog("tidentd", 0, LOG_LOCAL3);
#else
  openlog("tidentd", 0);
#endif
  
  id = id_open(&laddr.sin_addr, &faddr.sin_addr, NULL);
  if (!id)
  {
      if (errno)
      {
	  int saved_errno = errno;
	  char *hs;
	  
	  perror("Connection denied");
	  fflush(stderr);

	  hs = gethost(&faddr.sin_addr);
	  errno = saved_errno;
	  syslog(LOG_DEBUG, "Error: id_open(): host=%s, error=%m", hs);
      }
      else
	  puts("Connection denied.");
      exit(0);
  }

  printf("Querying for lport %d, fport %d....\r\n",
	 (int) ntohs(faddr.sin_port),
	 (int) ntohs(laddr.sin_port));
  fflush(stdout);

  errno = 0;
  if (id_query(id, ntohs(faddr.sin_port), ntohs(laddr.sin_port), 0) < 0)
  {
      if (errno)
      {
	  int saved_errno = errno;
	  char *hs;
	  
	  perror("id_query()");
	  fflush(stderr);

	  hs = gethost(&faddr.sin_addr);
	  errno = saved_errno;
	  syslog(LOG_DEBUG, "Error: id_query(): host=%s, error=%m", hs);
		 
      }
      else
      {
	  puts("Query failed.\n");
      }
      
    exit(0);
  }

  printf("Reading response data...\r\n");
  fflush(stdout);

  res = id_parse(id, NULL,
		   &lport, &fport,
		   &identifier,
		   &opsys,
		   &charset);

  switch (res)
  {
    default:
      if (errno)
      {
	  int saved_errno = errno;
	  char *hs;
	  
	  perror("id_parse()");
	  hs = gethost(&faddr.sin_addr);
	  errno = saved_errno;
	  syslog(LOG_DEBUG, "Error: id_parse(): host=%s, error=%m", hs);
      }
      else
	  puts("Error: Invalid response (empty response?).\n");
      
      break;

    case -2:
	{
	    int saved_errno = errno;
	    char *hs = gethost(&faddr.sin_addr);

	    errno = saved_errno;
	    syslog(LOG_DEBUG, "Error: id_parse(): host=%s, Parse Error: %s",
		   hs,
		   identifier ? identifier : "<no information available>");
	    if (identifier)
		printf("Parse error on reply:\n  \"%s\"\n", identifier);
	    else
		printf("Unidentifiable parse error on reply.\n");
	}
        break;

    case -3:
	{
	    int saved_errno = errno;
	    char *hs = gethost(&faddr.sin_addr);

	    errno = saved_errno;
	    
	    syslog(LOG_DEBUG,
		   "Error: id_parse(): host=%s, Illegal reply type: %s",
		   hs,
		   identifier);

	    printf("Parse error in reply: Illegal reply type: %s\n",
		   identifier);
	}
        break;
	     
    case 0:
	{
	    int saved_errno = errno;
	    char *hs = gethost(&faddr.sin_addr);

	    errno = saved_errno;
	    
	    syslog(LOG_DEBUG, "Error: id_parse(): host=%s, NotReady",
		   hs);
	    puts("Not ready. This should not happen...\r");
	}
      break;

    case 2:
	{
	    int saved_errno = errno;
	    char *hs = gethost(&faddr.sin_addr);

	    errno = saved_errno;
	    
	    syslog(LOG_DEBUG, "Reply: Error: host=%s, error=%s",
		   hs, identifier);
	}
      
      printf("Error response is:\r\n");
      printf("   Lport........ %d\r\n", lport);
      printf("   Fport........ %d\r\n", fport);
      printf("   Error........ %s\r\n", identifier);
      break;

    case 1:
      if (charset)
	syslog(LOG_INFO,
	       "Reply: Userid: host=%s, opsys=%s, charset=%s, userid=%s",
	       gethost(&faddr.sin_addr), opsys, charset, identifier);
      else
	syslog(LOG_INFO, "Reply: Userid: host=%s, opsys=%s, userid=%s",
	       gethost(&faddr.sin_addr), opsys, identifier);
      
      printf("Userid response is:\r\n");
      printf("   Lport........ %d\r\n", lport);
      printf("   Fport........ %d\r\n", fport);
      printf("   Opsys........ %s\r\n", opsys);
      printf("   Charset...... %s\r\n", charset ? charset : "<not specified>");
      printf("   Identifier... %s\r\n", identifier);

      if (id_query(id, ntohs(faddr.sin_port), ntohs(laddr.sin_port), 0) >= 0)
      {
	if (id_parse(id, NULL,
		     &lport, &fport,
		     &identifier,
		     &opsys,
		     &charset) == 1)
	  printf("   Multiquery... Enabled\r\n");
      }
  }

  fflush(stdout);
  sleep(1);
  exit(0);
}

