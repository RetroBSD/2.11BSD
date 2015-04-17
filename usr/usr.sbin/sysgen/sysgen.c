/*
 *	SYSGEN -- an imitation of the CONFIG command of VMS SYSGEN.
 *
 *	$Id: sysgen.c,v 1.9 1997/06/08 12:12:59 tih Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "devices.h"

struct userdev {
  char username[DEVNAMLEN];
  char name[DEVNAMLEN];
  int vector;
  long csr;
};

struct userdev usertab[MAXDEVS];

#define roundup(value,align) (value+align-1-((value+align-1)%align))

int main(argc, argv)
     int argc;
     char **argv;
{
  int i, j, k, n;
  int vecbase = VECBASE;
  long csrbase = CSRBASE;
  char userdev[DEVNAMLEN];
  char useralias[DEVNAMLEN];
  int userdevs = 0;
  char inputline[BUFSIZ];
  char scanformat[32];

  if (isatty(fileno(stdin))) {
    fprintf(stderr, "Type device names one per line, optionally followed\n");
    fprintf(stderr, "by a comma and a repeat count.  Finish with EOF.\n");
  }

  sprintf(scanformat, "%%%d[A-Za-z0-9],%%d", DEVNAMLEN - 1);
  while (fgets(inputline, BUFSIZ, stdin)) {
    for (i = 0; i < strlen(inputline); i++)
      if (inputline[i] == '\n')
	inputline[i] = '\0';
      else if (islower(inputline[i]))
	inputline[i] = toupper(inputline[i]);
    n = sscanf(inputline, scanformat, userdev, &k);
    if (n == 0)
      continue;
    if (n == 1)
      k = 1;
    strcpy(useralias, userdev);
    for (i = 0; i < ALIASTABLEN; i++)
      if (!strcmp(aliastab[i].alias, userdev))
	strcpy(useralias, aliastab[i].name);
    for (i = 0; i < DEVTABLEN; i++)
      if (!strcmp(devtab[i].name, useralias))
	break;
    if (i >= DEVTABLEN) {
      fprintf(stderr, "sysgen: %s: unrecognized device name; ignored\n",
	      useralias);
      continue;
    }
    while (k--) {
      usertab[userdevs].vector = 0;
      usertab[userdevs].csr = 0;
      strcpy(usertab[userdevs].username, userdev);
      strcpy(usertab[userdevs++].name, useralias);
    }
  }

  for (i = 0; i < DEVTABLEN; i++) {
    if (devtab[i].csr == FLOAT)
      csrbase = roundup(csrbase + CSRLEN, devtab[i].csralign);
    for (j = 0; j < userdevs; j++) {
      if ((!strcmp(devtab[i].name, usertab[j].name)) &&
	  (usertab[j].csr == 0)) {
	if ((devtab[i].vector == FLOAT) && (devtab[i].csr == FLOAT)) {
	  vecbase = roundup(vecbase, devtab[i].vecalign);
	  usertab[j].vector = vecbase;
	  vecbase = vecbase + devtab[i].numvecs * VECLEN;
	  usertab[j].csr = csrbase;
	  csrbase = roundup(csrbase + devtab[i].csralign, devtab[i].csralign);
	  continue;
	}
	if (devtab[i].vector == FLOAT) {
	  vecbase = roundup(vecbase, devtab[i].vecalign);
	  usertab[j].vector = vecbase;
	  vecbase = vecbase + devtab[i].numvecs * VECLEN;
	} else {
	  usertab[j].vector = devtab[i].vector;
	}
	if (devtab[i].csr == FLOAT) {
	  usertab[j].csr = csrbase;
	} else {
	  usertab[j].csr = devtab[i].csr;
	}
	break;
      }
    }
  }

  for (i = 0; i < userdevs; i++) {
    if (usertab[i].csr == 0)
      fprintf(stderr, "sysgen: %s: too many units; ignored\n",
	      usertab[i].username);
  }
  
  printf("Table of standard DEC assignments for configuration:\n\n");

  printf("  DEVICE     CSR   VECTOR\n-------------------------\n");
  for (i = 0; i < userdevs; i++) {
    if (usertab[i].csr == 0)
      continue;
    printf("%8s  %06lo%c  %04o%c\n",
	   usertab[i].username,
	   usertab[i].csr, (usertab[i].csr < 0764000) ? '*' : ' ',
	   usertab[i].vector, (usertab[i].vector < 0300) ? ' ' : '*');
  }
  printf("\nCSRs and vectors marked '*' are in floating space.\n");
  
  return 0;
}

/*
 *	eof
 */
