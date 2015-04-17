/*
 * Routines dealing with signals.
 *
 * A signal usually merely causes a bit to be set in the "signals" word.
 * At some convenient time, the mainline code checks to see if any
 * signals need processing by calling psignal().
 * An exception is made if we are reading from the keyboard when the
 * signal is received.  Some operating systems will simply call the
 * signal handler and NOT return from the read (with EINTR).
 * To handle this case, we service the interrupt directly from
 * the handler if we are reading from the keyboard.
 */

#include "less.h"
#include <signal.h>
#include <setjmp.h>

/*
 * The type of signal handler functions.
 * Usually int, although it should be void.
 */
typedef	int		HANDLER;

/*
 * "sigs" contains bits indicating signals which need to be processed.
 */
public int sigs;
#define	S_INTERRUPT	01
#ifdef SIGTSTP
#define	S_STOP		02
#endif

extern int reading;
extern char *first_cmd;
extern jmp_buf main_loop;

/*
 * Interrupt signal handler.
 */
	static HANDLER
interrupt()
{
	SIGNAL(SIGINT, interrupt);
	sigs |= S_INTERRUPT;
	if (reading)
		psignals();
}

#ifdef SIGTSTP
/*
 * "Stop" (^Z) signal handler.
 */
	static HANDLER
stop()
{
	SIGNAL(SIGTSTP, stop);
	sigs |= S_STOP;
	if (reading)
		psignals();
}
#endif

/*
 * Set up the signal handlers.
 */
	public void
init_signals()
{
	(void) SIGNAL(SIGINT, interrupt);
#ifdef SIGTSTP
	(void) SIGNAL(SIGTSTP, stop);
#endif
}

/*
 * Process any signals we have recieved.
 * A received signal cause a bit to be set in "sigs".
 */
	public void 
psignals()
{
	register int tsignals;

	tsignals = sigs;
	sigs = 0;
	if (tsignals == 0)
		return;

	dropout();		/* Discard any buffered output */

#ifdef SIGTSTP
	if (tsignals & S_STOP)
	{
		/*
		 * Clean up the terminal.
		 */
#ifdef SIGTTOU
		SIGNAL(SIGTTOU, SIG_IGN);
#endif
		lower_left();
		clear_eol();
		flush();
		raw_mode(0);
#ifdef SIGTTOU
		SIGNAL(SIGTTOU, SIG_DFL);
#endif
		SIGNAL(SIGTSTP, SIG_DFL);
#if SIGSETMASK
		/*
		 * This system will not allow us to send a 
		 * stop signal (SIGTSTP) to ourself
		 * while we are in the signal handler, like maybe now.
		 * (This can be the case if we are reading; see comment above.)
		 * So we ask the silly system for permission to do so.
		 */
		sigsetmask(0L);
#endif
		kill(getpid(), SIGTSTP);
		/*
		 * ... Bye bye. ...
		 * Hopefully we'll be back later and resume here...
		 * Reset the terminal and arrange to repaint the
		 * screen when we get back to the main command loop.
		 */
		SIGNAL(SIGTSTP, stop);
		raw_mode(1);
		first_cmd = "r";
		longjmp(main_loop, 1);
	}
#endif
	if (tsignals & S_INTERRUPT)
	{
		bell();
		/*
		 * {{ You may wish to replace the bell() with 
		 *    error("Interrupt"); }}
		 */
	}

	longjmp(main_loop, 1);
}

/*
 * Pass the specified command to a shell to be executed.
 * Like plain "system()", but handles resetting terminal modes, etc.
 */
	public void
lsystem(cmd)
	char *cmd;
{
	int inp;

	/*
	 * Print the command which is to be executed,
	 * unless the command starts with a "-".
	 */
	if (cmd[0] == '-')
		cmd++;
	else
	{
		lower_left();
		clear_eol();
		puts("!");
		puts(cmd);
		puts("\n");
	}

	/*
	 * De-initialize the terminal and take out of raw mode.
	 */
	deinit();
	flush();
	raw_mode(0);

	/*
	 * Restore signals to their defaults.
	 */
	SIGNAL(SIGINT, SIG_DFL);
#ifdef SIGTSTP
	SIGNAL(SIGTSTP, SIG_DFL);
#endif
	/*
	 * Force standard input to be the terminal, "/dev/tty".
	 */
	inp = dup(0);
	close(0);
	open("/dev/tty", 0);

	/*
	 * Pass the command to the system to be executed.
	 */
	system(cmd);

	/*
	 * Restore standard input, reset signals, raw mode, etc.
	 */
	close(0);
	dup(inp);
	close(inp);

	init_signals();
	raw_mode(1);
	init();
}

/*
 * Expand a filename, substituting any environment variables, etc.
 * The implementation of this is necessarily very operating system
 * dependent.  This implementation is unabashedly only for Unix systems.
 */
#if GLOB

#include <stdio.h>

	char *
glob(filename)
	char *filename;
{
	FILE *f;
	char *p;
	int ch;
	static char filebuf[128];
	static char ECHO[] = "echo ";

	strcpy(filebuf, ECHO);
	strtcpy(filebuf+sizeof(ECHO)-1, filename, sizeof(filebuf)-sizeof(ECHO));
	if ((f = popen(filebuf, "r")) == NULL)
		return (filename);
	for (p = filebuf;  p < &filebuf[sizeof(filebuf)-1];  p++)
	{
		if ((ch = getc(f)) == '\n' || ch == EOF)
			break;
		*p = ch;
	}
	*p = '\0';
	pclose(f);
	return (filebuf);
}

#else

	char *
glob(filename)
	char *filename;
{
	return (filename);
}

#endif
