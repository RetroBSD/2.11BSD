#include	"parms.h"
#include	"structs.h"

#ifdef	RCSIDENT
static char rcsid[] = "$Header: index.c,v 1.8 87/07/01 13:47:37 paul Exp $";
#endif	RCSIDENT

/*
 * INPUT KEY PROCESSING FOR INDEX PHASE 
 *
 *	Process all keystrokes while the index page is on the screen.
 *	invokes director options also (if user qualified)
 *	Returns: >=0 read note with that number ( 0 is policy)
 *		 -1	reprint the index page
 *		 -2	leave notefiles, update sequencer time
 *		 -3	leave notefile, dont update sequencer
 *		 -4	Universal leave notefiles
 *			(-2 and -3 leave single notefile, -4 leaves
 *			entire package, aborting rest of notefiles.
 *
 *	Original Coding:	Rob Kolstad	Winter 1980
 *	modifications:		Ray Essick	December 1981
 *	added MODERATED code:	Rich $alz	August 1985
 *	added LOCAL flag:	Rich $alz	August 1985
 */

static char YES[] = "YES";
static char NO[] = "NO ";
#define YorN(c)		((c) != 0 ? YES : NO)

indx (io, firstdis, lastdis, respnum)
struct io_f *io;
int    *firstdis,
       *lastdis,
       *respnum;
{
    struct io_f io2;					/* for nested notefiles */
    char    nfname[WDLEN + 1];				/* for nested nfs */
    int     num;					/* note number */
    int     i;
    int     c;
    int     znote,
            zresp;					/* for asearch */

    *respnum = 0;					/* init response */
    while (1)
    {
	at (-1, 1);
#ifdef	PROMPT
	printf (PROMPT);				/* issue a prompt */
#endif
	c = gchar ();
	printf ("\10 \10");				/* erase the key */
	switch (c)					/* what to do? */
	{
	    case '?': 
	    case 'h': 
		help (INDXHLP);				/* put the help on screen */
		return (-1);				/* back and display the index */

	    case 'r': 					/* replot the index page */
	    case '\f': 					/* everyone else uses ^L, might as well */
		return (-1);

	    case 'W': 					/* this too shall write a note */
	    case 'w': 					/* write a note */
		if ( ! (io -> descr.d_stat & MODERATED ) )
		    return addnote (io, NULL, "Edit Note text:", "Note Title: ", NULL, EDIT);
		if (askyn ("Moderated; send mail to moderator? ") == 'y')
		    mailit(io, (struct daddr_f *)0, (struct auth_f *)0,
			(struct when_f *)0, (char *)0, MODERATED, 0);
		return (-1);


	    case 'B': 					/* bitch, bitch, bitch */
		if (init (&io2, GRIPES) < 0)		/* no gripe file */
		{
		    at (0, 1);
		    printf ("Gripe file not available");
		}
		else
		{
		    addnote (&io2, NULL, "Edit Gripe text:", "Gripe Header: ", NULL, EDIT);
							/* let him put the note in */
		    finish (&io2);			/* close up the gripe file */
		}
		return (-1);				/* replot */

	    case '-': 					/* back up a little */
	    case '\b': 					/* add backspace also */
		*firstdis -= Nindex - 1;
		return (-1);

	    case '=': 					/* back up a lot */
		*firstdis = 1;
		return (-1);

	    case '+': 
	    case '\r': 
	    case '\n': 
	    case ' ': 
		if (*lastdis < io -> descr.d_nnote)
		{
		    *firstdis = *lastdis;
		    return (-1);
		}
		break;					/* keep doing what we were */

	    case '*': 					/* skip to last page */
		if (*lastdis < io -> descr.d_nnote)
		{
		    *firstdis = io -> descr.d_nnote - Nindex + 1;
		    return (-1);
		}
		break;					/* already at end */

	    case 'q': 
#ifdef	K_KEY
	    case 'k': 					/* can use right hand */
#endif	K_KEY
		return QUITSEQ;

	    case '\04': 				/* ^D */
		return QUITFAST;			/* total exit */

	    case 'z': 					/* ^D w/sequencer update */
		return QUITUPD;				/* total exit w/update */

	    case 'Q': 					/* exit without update of sequencer */
#ifdef	K_KEY
	    case 'K': 					/* so can use just right hand */
#endif	K_KEY
		return QUITNOSEQ;

	    case 'N': 					/* go to an archive */
		sprintf (nfname, "%s/%s", ARCHDIR, io -> nf);/* build dest */
		goto donest;				/* share common code */



	    case 'n': 					/* nest notesfiles - a stack */
		at (-1, 10);
		printf ("New notesfile: ");
		printf ("               \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		if (gline (nfname, WDLEN) == 1)
		    return (-1);			/* forget it, replot */
	donest: 					/* used by N */
		closenf (io);				/* save fids */
		if ((i = control (nfname, NOSEQ)) == -1)/* do the other */
		    sleep (1);				/* some error there */
		if (opennf (io, 0) < 0)
		{
		    at (0, 1);
		    printf ("Couldn't reopen notesfile %s", io -> fullname);
		    fflush (stdout);
		    sleep (2);
		    return QUITNOSEQ;			/* don't update */
		}
		if (i == QUITFAST)			/* he in a hurry? */
		    return QUITFAST;			/* oblige him */
		return (-1);				/* redisplay index */

	    case 'p': 					/* to read note 0 */
		if (io -> descr.d_plcy)
		    return 0;
		else
		{
		    at (0, PROMPTMSGX);
		    printf ("There is no policy note");
		    continue;				/* grab another key */
		}



	    case 'd': 					/* to director options */
		if (allow (io, DRCTOK))
		    return direct (io);			/* perform those babies */
		else
		{
		    at (0, PROMPTMSGX);
		    printf (
         "Anonymous: %s   Networked: %s   Moderated: %s   Local: %s",
			    YorN (io -> descr.d_stat & ANONOK),
			    YorN (io -> descr.d_stat & NETWRKD),
			    YorN (io -> descr.d_stat & MODERATED),
			    YorN (io -> descr.d_stat & LOCAL));

		    continue;
		}

	    case 'x': 
	    case 'X': 
		i = tsearch (io, *lastdis, c == 'x');	/* assume lies before here */
		if (i > 0)
		    return i;				/* return that one */
		continue;				/* otherwise get another key */

	    case 'a': 
	    case 'A': 					/* author search from current spot */
		znote = *lastdis;
		zresp = 0;				/* start at the correct place */
		i = asearch (io, &znote, &zresp, (c == 'a'));
							/* look */
		if (i > 0)
		{
		    *respnum = zresp;			/* return correct value */
		    return znote;			/* and such */
		}
		continue;				/* get another command */

	    case '1': 
	    case '2': 
	    case '3': 
	    case '4': 
	    case '5': 
	    case '6': 
	    case '7': 
	    case '8': 
	    case '9': 
		at (-1, 1);
		printf ("Read note > ");
		if ((num = getnum (c)) == 0)
		{
		    at (-1, 3);
		    printf ("                      ");
		    continue;
		}

		return num;

	    case 'j': 
	    case 'J': 					/* goto first unread article */
		return (nxtnote (io, 0, &io -> stime));

	    case 'l': 					/* leave if no new */
	    case 'L': 
		if ((i = nxtnote (io, 0, &io -> stime)) < 0)
		    return QUITSEQ;			/* nope, leave */
		else
		    return (i);				/* go there */

	    case 'o': 					/* modify sequencer time */
		gdate (&io -> stime);			/* let him hack on the time */
		continue;				/* and go back */

	    case 'O': 					/* set it for today's notes */
		gettime (&io -> stime);			/* grab current date */
		io -> stime.w_hours = 0;		/* beginning of day */
		io -> stime.w_mins = 0;
		io -> stime.w_gmttime = 0;		/* use formatted */
		at (0, PROMPTMSGX);			/* tell him what we did */
		printf ("Set to read notes since: ");
		prdate (&io -> stime);
		continue;				/* and get the next command */


	    case '!': 
		gshell ();				/* give him a shell in right directory */
		return (-1);

	    default: 
		at (0, 5);
		printf ("\07     type ? for help, q to quit     ");
		continue;
	}


/* end main index page code */
    }
}
