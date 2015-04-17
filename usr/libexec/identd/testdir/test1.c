/*
** A small test program. If run without any arguments, it should
** return *your* user name if the machine it is run on has a correctly
** installed and running Ident server.
**
** Last modified: 11 August 1993 by Michael Kuch
**
** Author: Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>


int Toupper(c)
  int c;
{
  if (islower(c))
    return toupper(c);
  else
    return c;
}


/*
** Compare two strings, case insensitive
*/
int Stricmp(s1, s2)
  char *s1, *s2;
{
  int diff;


  while (!(diff = Toupper(*s1) - Toupper(*s2)) && *s1)
    s1++, s2++;

  return diff;
}


Perror(str)
  char *str;
{
  perror(str);
  exit(1);
}


main(argc,argv)
  int argc;
  char *argv[];
{
  int fd;
  struct sockaddr_in addr;
  int addrlen;
  int port;
  FILE *fp_in, *fp_out;
  int lport, fport;
  char buffer[8192];
  char *cp;
  char reply_type[81];
  char opsys_or_error[81];
  char identifier[1024];
  
  
  if (argc > 3)
  {
    printf("usage: %s [{host-ip-number} [{port number}]]", argv[0]);
    exit(1);
  }
  
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    Perror("socket");

  addr.sin_family = AF_INET;
  if(argc > 1)
    addr.sin_addr.s_addr = inet_addr(argv[1]);
  else
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (argc > 2)
    port = atoi(argv[2]);
  else
    port = 113;
  
  addr.sin_port = htons(port);
  addrlen = sizeof(addr);

  if (connect(fd, &addr, addrlen) == -1)
    Perror("connect");

  addrlen = sizeof(addr);
  if (getsockname(fd, &addr, &addrlen) == -1)
    Perror("getsockname");

  fp_in  = fdopen(fd, "r");
  fp_out = fdopen(fd, "w");
  if (!fp_in || !fp_out)
    Perror("fdopen");

  fprintf(fp_out, "%d , %d\n", ntohs(addr.sin_port), port);
  fflush(fp_out);

  if (fgets(buffer, sizeof(buffer)-1, fp_in) == NULL)
    Perror("fgets");

  shutdown(fd, 1);

  cp = buffer;
  while (*cp != 0 && (*cp < ' ' || isspace(*cp)))
      ++cp;
  
  argc = sscanf(cp, "%d , %d : %[^ \t\n\r:] : %[^\t\n\r:] : %[^\n\r]",
		&lport, &fport, reply_type, opsys_or_error, identifier);
  if (argc < 3)
  {
    fprintf(stderr, "sscanf: too few arguments (%d)\n", argc);
    exit(1);
  }
  if (Stricmp(reply_type, "ERROR") == 0)
  {
    printf("Ident error: error code: %s\n", opsys_or_error);
    exit(1);
  }
  else if (Stricmp(reply_type, "USERID") != 0)
  {
    printf("Ident error: illegal reply type: %s\n", reply_type);
    exit(1);
  }
  else
    printf("Ident returned:\n");
    printf("\tOpsys and Charset (if -c specified for identd) = %s\n",
				 opsys_or_error);
    printf("\tIdentifier: %s\n", identifier);

  fclose(fp_out);
  fclose(fp_in);

  exit(0);
}
