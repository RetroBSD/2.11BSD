/*
 * cc -O -o play play.c
 * chog <play> play
 * chmod u+s   play
 * mv play /usr/games
 */
#include "config.h"
#include <signal.h>

main (argc, argv)
  int   argc;
  char  **argv;

{ int status;

  signal (SIGINT, SIG_IGN);
  signal (SIGQUIT, SIG_IGN);
  chdir (HACKDIR);
  if  (fork ())
  { wait (&status);
    unlink ("rclk");
    exit (0);
  }
  else
  { argv[0] = "hack";
    execv ("hack", argv);
  }
}
