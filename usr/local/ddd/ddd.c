/*
 * ddd.c - double dd (version 2)
 *
 * Copyright 1988 Helsinki University of Technology.
 * All rights reserved.
 *
 * Permission granted to distribute, use and modify
 * this code for uncommercial purposes, provided
 * that this copyright notice is not changed.
 *
 * Author: Tapani Lindgren (nispa@cs.hut.fi)
 *
 * Ddd is a dd clone that operates as two processes;
 * one process reads while the other one writes and vice versa.
 * This way the throughput may be up to twice as good as that of dd,
 * especially with slow devices such as tape drives.
 *
 * ***** WARNING ***** (For U.S. residents & citizens only)
 *
 * Do not use this program!  Get rid of it as soon as you can!
 * It will probably corrupt all your data, break down your computer
 * and cause severe injury to the operators.
 * Even reading the source code may give you a headache.
 * I warned you!  I will take no responsibility whatsoever!
 */

/* declarations common to all unix versions */
#include <stdio.h>     /* for fprintf() and stderr() */
#include <signal.h>    /* for SIGTERM */
extern char *malloc();

/* version dependent declarations */

#ifdef BSD
#include <sys/wait.h>  /* for union wait */
#include <sys/file.h>  /* for O_RDONLY and O_WRONLY */
extern char *sprintf();
#endif

#ifdef SYSV
#include <fcntl.h>     /* for O_RDONLY and O_WRONLY */
void exit();
void perror();
#endif



/* macros to find min or max of two values */
#define min(a,b) ((a)<(b)? (a): (b))
#define max(a,b) ((a)>(b)? (a): (b))

/* inherited file descriptors */
#define STDIN 0
#define STDOUT 1

/* boolean values */
#define FALSE 0
#define TRUE 1

/* pipes have a read end and a write end */
#define P_REND 0
#define P_WEND 1

/* there are two pipes; one for read tokens and one for write tokens */
#define RTOK_P 0
#define WTOK_P 1

/* token bytes passed along pipes */
#define TOK_CONT 0     /* go ahead */
#define TOK_DONE 1     /* end of data */
#define TOK_ERROR 2    /* something's wrong, you've better stop now */

/* input/output full/short record counters are in a table;
 indexes defined below */
#define FULLIN 0
#define SHORTIN 1
#define FULLOUT 2
#define SHORTOUT 3

/* defaults */
#define DEFBS 512      /* default block size */

/* forward declarations */
int doerror();

/* global variables */
static int
  ifd, ofd,    /* input/output file descriptors */
  ibs, obs,    /* input/output block sizes */
  cbs, /* "conversion" buffer size */
  pid, /* pid of child (in parent) or 0 (in child) */
  eof = FALSE, /* have we encountered end-of-file */
  pipefd[2][2],        /* read/write fd's for 2 pipes */
  counters[4] = {0, 0, 0, 0},  /* input/output full/short record counters */
  buflen;      /* count of characters read into buffer */
static char
  *buffer;     /* address of buffer */


main(argc, argv)
int argc;
char *argv[];
{
  (void) catchsignals();       /* prepare for interrupts etc */
  (void) setdefaults();        /* set default values for parameters */
  (void) parsearguments(argc, argv);   /* parse arguments */
  (void) initbuffer(); /* initialize buffer */
  (void) inittokens(); /* create one READ and one WRITE token */
  (void) dofork();     /* 1 will be 2 */
  while (!eof) {       /* enter main loop */
    (void) gettoken(RTOK_P);   /* compete for first/next read turn */
    (void) readbuffer();       /* fill buffer with input */
    (void) gettoken(WTOK_P);   /* make sure we also get the next write turn */
    /* now others may read if they wish (and if there's any data left */
    (void) passtoken(RTOK_P, eof? TOK_DONE: TOK_CONT);
    (void) writebuffer();      /* ... while we write to output */
    /* this cycle is done now */
    if (!eof) (void) passtoken(WTOK_P, TOK_CONT);
  }    /* end of main loop */
  (void) passcounters(RTOK_P); /* send record counters to our partner */
  (void) terminate(0); /* and exit (no errors) */
  /* NOTREACHED */
}      /* end of main() */


catchsignals()
/* arrange for some signals to be catched, so that statistics can be printed */
{
  static int siglist[] = {
    SIGINT, SIGQUIT, SIGILL, SIGFPE,
    SIGBUS, SIGSEGV, SIGSYS, SIGPIPE,
    SIGALRM, SIGTERM, 0
    };
  int *sigp;

  for (sigp = siglist; *sigp != 0; sigp++)
    (void) signal(*sigp, doerror);
}      /* end of catchsignals() */

doerror()
/* what we do if we get an error or catch a signal */
{
  /* send error token to both pipes */
  (void) passtoken(RTOK_P, TOK_ERROR);
  (void) passtoken(WTOK_P, TOK_ERROR);
  /* also send i/o record counters */
  (void) passcounters(RTOK_P);
  (void) passcounters(RTOK_P);
  /* terminate with error status */
  (void) terminate(1);
}

terminate(stat)
int stat;
{
  /* parent will try to wait for child */
#ifdef BSD
  if (pid) (void) wait((union wait *) 0);
#endif
#ifdef SYSV
  if (pid) (void) wait((int *) 0);
#endif

  exit(stat);
}

setdefaults()
/* set default values */
{
  ifd = STDIN;
  ofd = STDOUT;
  ibs = obs = DEFBS;   /* block sizes */
  cbs = 0;     /* initially; will be set to max(ibs, obs, cbs) */
}

parsearguments(argc, argv)
int argc;
char *argv[];
  /* parse arguments */
{
  /* constant strings "array" for recognizing options */
  static struct {
    char *IF, *OF, *CBS, *IBS, *OBS, *BS, *NOTHING;
  } consts = {
    "if=", "of=", "cbs=", "ibs=", "obs=", "bs=", ""
    };
  char **constp;       /* const structure pointer */

  for (argc--, argv++; argc > 0; argc--, argv++) {
    constp = (char **) &consts;
    while (**constp && strncmp(*argv, *constp, strlen(*constp)))
      constp++;
    /* constp now points to one of the pointers in consts structure */
    *argv += strlen(*constp);  /* skip the constant part of the argument */
    if (constp == &consts.IF) {        /* open another file for input */
      ifd = open(*argv, O_RDONLY);
      if (ifd < 0) perror (*argv);
    } else if (constp == &consts.OF) {
      ofd = open(*argv, O_WRONLY | O_CREAT);   /* open file for output */
      if (ofd < 0) perror (*argv);
    } else if (constp == &consts.CBS) {        /* set buffer size */
      cbs = evalopt(*argv);
    } else if (constp == &consts.IBS) {        /* set input block size */
      ibs = evalopt(*argv);
    } else if (constp == &consts.OBS) {        /* set output block size */
      obs = evalopt(*argv);
    } else if (constp == &consts.BS) { /* set input and output block sizes */
      ibs = obs = evalopt(*argv);
    } else {
      (void) fprintf(stderr,
                    "usage: ddd [if=name] [of=name] [bs=n] [ibs=n obs=n]\n");
      exit(1);
    }
  } /* end of for loop */
} /* end of parsearguments() */

evalopt(p) /* return numerical value of string */
char *p;
{
  int temp = 0;

  for ( ; *p >= '0' && *p <= '9'; p++)
    temp = temp * 10 + *p - '0';
  if (temp < 1) {
    (void) fprintf(stderr, "ddd: illegal size option\n");
    exit(1);
  }
  switch (*p) {
  case '\0':
    return(temp);
  case 'w':
  case 'W':
    return(temp << 2); /* 4-byte words */
  case 'b':
  case 'B':
    return(temp << 9); /* 512-byte blocks */
  case 'k':
  case 'K':
    return(temp << 10);        /* kilobytes */
  default:
    (void) fprintf(stderr, "ddd: bad size option\n");
    exit(1);
  }
  /* NOTREACHED */
}      /* end of evalopt() */

initbuffer()
/* initialize buffer */
{
  cbs = max(cbs, max(ibs, obs));       /* determine buffer size */
  if (cbs % ibs || cbs % obs) {
    (void) fprintf(stderr, "ddd: warning: incompatible block/buffer sizes\n");
  }
  buffer = malloc((unsigned) cbs);
  if (buffer == NULL) {
    (void) perror("ddd: cannot allocate buffer");
    exit(1);
  }
}      /* end of initbuffer() */

inittokens()
/* initialize token passing system with 2 pipes */
{
  if(pipe(pipefd[RTOK_P]) < 0 || pipe(pipefd[WTOK_P]) < 0) {
    (void) perror("ddd: cannot create token pipes");
    exit(1);
  }
  /* create initial tokens */
  (void) passtoken(RTOK_P, TOK_CONT);
  (void) passtoken(WTOK_P, TOK_CONT);
}      /* end of inittokens() */

passtoken(pipenum, token)
int pipenum;
char token;
/* pass a token to a pipe */
{
  if (write(pipefd[pipenum][P_WEND], &token, 1) < 1) {
    (void) perror("ddd: cannot write token to pipe");
    exit(1);
  }
}      /* end of passtoken() */

gettoken(pipenum)
int pipenum;
/* wait to read a token from the pipe; also see if we should stop */
{
  char tokenbuf;

  if (read(pipefd[pipenum][P_REND], &tokenbuf, 1) < 1) {
    (void) perror("ddd: cannot read token from pipe");
    exit(1);
  }
  if (tokenbuf != TOK_CONT) {  /* we did not get what we wanted */
    (void) getcounters(pipenum);       /* report record counters */
    terminate(tokenbuf == TOK_DONE);   /* TOK_DONE means no error */
  }
}      /* end of gettoken() */

passcounters(pipenum)
int pipenum;
/* pass read/write counters to the other process */
{
  if (write(pipefd[pipenum][P_WEND], (char *) counters,
           sizeof(counters)) < sizeof(counters)) {
    (void) perror("ddd: cannot write counters to pipe");
    exit(1);
  }
}

getcounters(pipenum)
int pipenum;
/* report input/output record counts */
{
  int hiscounters[4];

  if (read(pipefd[pipenum][P_REND], (char *) hiscounters,
          sizeof(hiscounters)) < sizeof(hiscounters)) {
    (void) perror("ddd: cannot read counters from pipe");
    exit(1);
  }
  (void) fprintf(stderr,
                "%d+%d records in\n%d+%d records out\n",
                counters[FULLIN] + hiscounters[FULLIN],
                counters[SHORTIN] + hiscounters[SHORTIN],
                counters[FULLOUT] + hiscounters[FULLOUT],
                counters[SHORTOUT] + hiscounters[SHORTOUT]
                );
}      /* end of printcounters() */

dofork()
/* fork into 2 processes */
{
  if ((pid = fork()) < 0) {
    (void) perror("ddd: warning: cannot fork");
    /* But continue and do our job anyway, as regular dd */
  }
}

readbuffer()
/* read buffer from input */
{
  int iolen, ioresult;

  buflen = 0;
  while (buflen < cbs && !eof) {
    iolen = min(ibs, cbs - buflen);
#ifdef BSD
    ioresult = read(ifd, &buffer[buflen], iolen);
#endif
#ifdef SYSV
    ioresult = read(ifd, &buffer[buflen], (unsigned) iolen);
#endif
    if (ioresult == 0) {       /* end of file */
      eof = TRUE;
    } else if (ioresult < 0) {
      (void) perror("ddd: read error");
      (void) doerror();
    }
    buflen += ioresult;        /* update current count of chars in buffer */
    /* if we got any data, update appropriate input record count */
    if (ioresult > 0) counters[(ioresult == ibs)? FULLIN: SHORTIN]++;
  }
}      /* end of readbuffer() */

writebuffer()
/* writing phase */
{
  int ocount, iolen, ioresult;

  ocount = 0;  /* count of chars written */
  while (ocount < buflen) {
    iolen = min(obs, buflen - ocount);
#ifdef BSD
    ioresult = write(ofd, &buffer[ocount], iolen);
#endif
#ifdef SYSV
    ioresult = write(ofd, &buffer[ocount], (unsigned) iolen);
#endif
    if (ioresult < iolen) {
      perror("ddd: write error");
      (void) doerror();
    }
    ocount += ioresult;
    /* count output records */
    counters[(ioresult == obs)? FULLOUT: SHORTOUT]++;
  }
}      /* end of writebuffer() */
