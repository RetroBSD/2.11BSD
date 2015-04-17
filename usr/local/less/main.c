/*
 * Entry point, initialization, miscellaneous routines.
 */

#include "less.h"
#include "position.h"
#include <setjmp.h>

public int	ispipe;
public jmp_buf	main_loop;
public char *	first_cmd;
public char *	every_first_cmd;
public int	new_file;
public int	is_tty;
public char 	current_file[128];
public int	any_display;
public int	ac;
public char **	av;
public int 	curr_ac;
#if LOGFILE
public int	logfile = -1;
public char *	namelogfile = NULL;
#endif
#if EDITOR
public char *	editor;
#endif

extern int file;
extern int nbufs;
extern int sigs;
extern int quit_at_eof;
extern int p_nbufs, f_nbufs;
extern int back_scroll;
extern int top_scroll;
extern int sc_height;
extern int errmsgs;


/*
 * Edit a new file.
 * Filename "-" means standard input.
 * No filename means the "current" file, from the command line.
 */
	public void
edit(filename)
	char *filename;
{
	register int f;
	char message[100];
	static didpipe;

	if (filename == NULL || *filename == '\0')
	{
		if (curr_ac >= ac)
		{
			error("No current file");
			return;
		}
		filename = av[curr_ac];
	}
	if (strcmp(filename, "-") == 0)
	{
		/* 
		 * Use standard input.
		 */
		if (didpipe)
		{
			error("Can view standard input only once");
			return;
		}
		f = 0;
	} else if ((f = open(filename, 0)) < 0)
	{
		static char co[] = "Cannot open ";
		strcpy(message, co);
		strtcpy(message+sizeof(co)-1, filename, 
			sizeof(message)-sizeof(co));
		error(message);
		return;
	}

	if (isatty(f))
	{
		/*
		 * Not really necessary to call this an error,
		 * but if the control terminal (for commands)
		 * and the input file (for data) are the same,
		 * we get weird results at best.
		 */
		error("Can't take input from a terminal");
		if (f > 0)
			close(f);
		return;
	}

#if LOGFILE
	/*
	 * If he asked for a log file and we have opened standard input,
	 * create the log file.  
	 * We take care not to blindly overwrite an existing file.
	 */
	end_logfile();
	if (f == 0 && namelogfile != NULL && is_tty)
	{
		int exists;
		int answer;

		/*
		 * {{ We could use access() here. }}
		 */
		exists = open(namelogfile, 0);
		close(exists);
		exists = (exists >= 0);

		if (exists)
		{
			static char w[] = "WARNING: log file exists: ";
			strcpy(message, w);
			strtcpy(message+sizeof(w)-1, namelogfile,
				sizeof(message)-sizeof(w));
			error(message);
			answer = 'X';	/* Ask the user what to do */
		} else
			answer = 'O';	/* Create the log file */

	loop:
		switch (answer)
		{
		case 'O': case 'o':
			logfile = creat(namelogfile, 0644);
			break;
		case 'A': case 'a':
			logfile = open(namelogfile, 1);
			if (lseek(logfile, (off_t)0, 2) < 0)
			{
				close(logfile);
				logfile = -1;
			}
			break;
		case 'D': case 'd':
			answer = 0;	/* Don't print an error message */
			break;
		case 'q':
			quit();
		default:
			puts("\n  Overwrite, Append, or Don't log? ");
			answer = getc();
			puts("\n");
			flush();
			goto loop;
		}

		if (logfile < 0 && answer != 0)
		{
			sprintf(message, "Cannot write to \"%s\"", 
				namelogfile);
			error(message);
		}
	}
#endif

	/*
	 * We are now committed to using the new file.
	 * Close the current input file and set up to use the new one.
	 */
	if (file > 0)
		close(file);
	new_file = 1;
	strtcpy(current_file, filename, sizeof(current_file));
	ispipe = (f == 0);
	if (ispipe)
		didpipe = 1;
	file = f;
	ch_init( (ispipe) ? p_nbufs : f_nbufs );
	init_mark();

	if (every_first_cmd != NULL)
		first_cmd = every_first_cmd;

	if (is_tty)
	{
		int no_display = !any_display;
		any_display = 1;
		if (no_display && errmsgs > 0)
		{
			/*
			 * We displayed some messages on error output
			 * (file descriptor 2; see error() function).
			 * Before erasing the screen contents,
			 * display the file name and wait for a keystroke.
			 */
			error(filename);
		}
		/*
		 * Indicate there is nothing displayed yet.
		 */
		pos_clear();
	}
}

/*
 * Edit the next file in the command line list.
 */
	public void
next_file(n)
	int n;
{
	if (curr_ac + n >= ac)
	{
		if (quit_at_eof)
			quit();
		error("No (N-th) next file");
	} else
		edit(av[curr_ac += n]);
}

/*
 * Edit the previous file in the command line list.
 */
	public void
prev_file(n)
	int n;
{
	if (curr_ac - n < 0)
		error("No (N-th) previous file");
	else
		edit(av[curr_ac -= n]);
}

/*
 * Copy a file directly to standard output.
 * Used if standard output is not a tty.
 */
	static void
cat_file()
{
	register int c;

	while ((c = ch_forw_get()) != EOF)
		putc(c);
	flush();
}

/*
 * Entry point.
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	char *getenv();


	/*
	 * Process command line arguments and LESS environment arguments.
	 * Command line arguments override environment arguments.
	 */
	init_option();
	scan_option(getenv("LESS"));
	argv++;
	while ( (--argc > 0) && 
		(argv[0][0] == '-' || argv[0][0] == '+') && 
		argv[0][1] != '\0')
		scan_option(*argv++);

#if EDITOR
	editor = getenv("EDITOR");
	if (editor == NULL || *editor == '\0')
		editor = EDIT_PGM;
#endif

	/*
	 * Set up list of files to be examined.
	 */
	ac = argc;
	av = argv;
	curr_ac = 0;

	/*
	 * Set up terminal, etc.
	 */
	is_tty = isatty(1);
	if (!is_tty)
	{
		/*
		 * Output is not a tty.
		 * Just copy the input file(s) to output.
		 */
		if (ac < 1)
		{
			edit("-");
			cat_file();
		} else
		{
			do
			{
				edit((char *)NULL);
				if (file >= 0)
					cat_file();
			} while (++curr_ac < ac);
		}
		exit(0);
	}

	raw_mode(1);
	get_term();
	open_getc();
	init();

	if (setjmp(main_loop))
		quit();
	init_signals();

	/*
	 * Select the first file to examine.
	 */
	if (ac < 1)
		edit("-");	/* Standard input */
	else 
	{
		/*
		 * Try all the files named as command arguments.
		 * We are simply looking for one which can be
		 * opened without error.
		 */
		do
		{
			edit((char *)NULL);
		} while (file < 0 && ++curr_ac < ac);
	}

	if (file >= 0)
		commands();
	quit();
}

/*
 * Copy a string, truncating to the specified length if necessary.
 * Unlike strncpy(), the resulting string is guaranteed to be null-terminated.
 */
strtcpy(to, from, len)
	char *to;
	char *from;
	int len;
{
	strncpy(to, from, len);
	to[len-1] = '\0';
}

/*
 * Exit the program.
 */
	public void
quit()
{
	/*
	 * Put cursor at bottom left corner, clear the line,
	 * reset the terminal modes, and exit.
	 */
#if LOGFILE
	end_logfile();
#endif
	lower_left();
	clear_eol();
	deinit();
	flush();
	raw_mode(0);
	exit(0);
}
