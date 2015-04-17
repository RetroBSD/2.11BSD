#include  "less.h"

/*
 * Display some help.
 * Just invoke another "less" to display the help file.
 *
 * {{ This makes this function very simple, and makes changing the
 *    help file very easy, but it may present difficulties on
 *    (non-Unix) systems which do not supply the "system()" function. }}
 */

	public void
help()
{
	char cmd[200];

	sprintf(cmd, 
	 "-less -m '-Pm<HELP -- Press RETURN for more, or q when done >' %s",
	 HELPFILE);
	lsystem(cmd);
	error("End of help");
}
