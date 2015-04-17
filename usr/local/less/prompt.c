/*
 * Prompting and other messages.
 * There are three flavors of prompts, SHORT, MEDIUM and LONG,
 * selected by the -m/-M options.
 * A prompt is either a colon or a message composed of various
 * pieces, such as the name of the file being viewed, the percentage
 * into the file, etc.
 */

#include "less.h"
#include "position.h"

extern int pr_type;
extern int ispipe;
extern int hit_eof;
extern int new_file;
extern int sc_width;
extern char current_file[];
extern int ac;
extern char **av;
extern int curr_ac;

/*
 * Prototypes for the three flavors of prompts.
 * These strings are expanded by pr_expand().
 */
char *prproto[] = { 
	"fo",		/* PR_SHORT */
	"foP",		/* PR_MEDIUM */
	"Fobp"		/* PR_LONG */
};

static char message[200];
static char *mp;

	static void
setmp()
{
	mp = message + strlen(message);
}

/*
 * Append the name of the current file (to the message buffer).
 */
	static void
ap_filename()
{
	if (ispipe)
		return;
	strtcpy(mp, current_file, &message[sizeof(message)] - mp);
	setmp();
}

/*
 * Append the "file N of M" message.
 */
	static void
ap_of()
{
	if (ac <= 1)
		return;
	sprintf(mp, " (file %d of %d)", curr_ac+1, ac);
	setmp();
}

/*
 * Append the byte offset into the current file.
 */
	static void
ap_byte()
{
	POSITION pos, len;

	pos = position(BOTTOM_PLUS_ONE);
	if (pos == NULL_POSITION)
		pos = ch_length();
	if (pos != NULL_POSITION)
	{
		sprintf(mp, " byte %ld", (long)pos);
		setmp();
		len = ch_length();
		if (len > 0)
		{
			sprintf(mp, "/%ld", (long)len);
			setmp();
		}
	}
}

/*
 * Append the percentage into the current file.
 * If we cannot find the percentage and must_print is true,
 * use the byte offset.
 */
	static void
ap_percent(must_print)
{
	POSITION pos,len;

	pos = position(BOTTOM_PLUS_ONE);
	len = ch_length();
	if (len > 0 && pos != NULL_POSITION)
	{
		sprintf(mp, " (%ld%%)", (100 * (long)pos) / len);
		setmp();
	} else if (must_print)
		ap_byte();
}

/*
 * Append the end-of-file message.
 */
	static void
ap_eof()
{
	strcpy(mp, " (END)");
	setmp();
	if (curr_ac + 1 < ac)
	{
		sprintf(mp, " - Next: %s", av[curr_ac+1]);
		setmp();
	}
}

/*
 * Construct a message based on a prototype string.
 */
	static char *
pr_expand(proto, maxwidth)
	char *proto;
	int maxwidth;
{
	register char *p;

	mp = message;

	for (p = proto;  *p != '\0';  p++)
	{
		if (maxwidth > 0 && mp >= message + maxwidth)
		{
			/*
			 * Truncate to the screen width.
			 * {{ This isn't very nice. }}
			 */
			mp = message + maxwidth;
			break;
		}
		switch (*p)
		{
		case 'f':
			if (new_file)
				ap_filename();
			break;
		case 'F':
			ap_filename();
			break;
		case 'o':
			if (new_file)
				ap_of();
			break;
		case 'O':
			ap_of();
			break;
		case 'b':
			ap_byte();
			break;
		case 'p':
			if (!hit_eof)
				ap_percent(0);
			break;
		case 'P':
			if (!hit_eof)
				ap_percent(1);
			break;
		case '<':
			while (*++p != '>')
			{
				if (*p == '\0')
				{
					p--;
					break;
				}
				*mp++ = *p;
			}
			break;
		default:
			*mp++ = *p;
			break;
  		}
	}
	if (hit_eof)
		ap_eof();

	new_file = 0;
	if (mp == message)
		return (NULL);
	*mp = '\0';
	return (message);
}

/*
 * Return a message suitable for printing by the "=" command.
 */
	public char *
eq_message()
{
	return (pr_expand("FObp", 0));
}

/*
 * Return a prompt.
 * This depends on the prompt type (SHORT, MEDIUM, LONG), etc.
 * If we can't come up with an appropriate prompt, return NULL
 * and the caller will prompt with a colon.
 */
	public char *
pr_string()
{
	return (pr_expand(prproto[pr_type], sc_width-2));
}
