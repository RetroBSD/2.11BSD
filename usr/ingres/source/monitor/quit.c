/*
 * @(#)		quit.c	1.1	(2.11BSD)	1996/3/22
*/

#include	<stdio.h>
#include	<signal.h>
#include	<string.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"monitor.h"

/*
**  QUIT INGRES
**
**	This routine starts the death of the other processes.  It
**	then prints out the logout message, and then waits for the
**	rest of the system to die.  Note, however, that no relations
**	are removed; this must be done using the PURGE command.
**
**	Uses trace flag 1
**
**	History:
**		3/2/79 (eric) -- Changed to close Trapfile.
*/

extern	char	*sys_siglist[];

quit()
{
	register int	ndx;
	register int	pidptr;
	register int	err;
	char		buf[100];
	int		status;
	int		pidlist[50];
	extern int	(*Exitfn)();
	extern		exit();
	char		indexx[0400];
	char		*cp;

#	ifdef xMTR1
	if (tTf(1, -1))
		printf("entered quit\n");
#	endif

	/* INTERCEPT ALL FURTHER INTERRUPTS */
	signal(1, 1);
	signal(2, 1);
	Exitfn = &exit;

	close(W_down);

#	ifdef xMTR3
	if (tTf(1, 2))
		printf("unlinking %s\n", Qbname);
#	endif

	/* REMOVE THE QUERY-BUFFER FILE */
	fclose(Qryiop);
	unlink(Qbname);
	if (Trapfile != NULL)
		fclose(Trapfile);
	pidptr = 0;
	err = 0;

	/* clear out the system error index table */
	bzero(indexx, sizeof (indexx));

	/* wait for all process to terminate */
	while ((ndx = wait(&status)) != -1)
	{
#		ifdef xMTR2
		if (tTf(1, 5))
			printf("quit: pid %u: %d/%d\n",
				ndx, status >> 8, status & 0177);
#		endif
		pidlist[pidptr++] = ndx;
		if ((status & 0177) != 0)
		{
			printf("%d: ", ndx);
			ndx = status & 0177;
			if (ndx > NSIG)
				printf("Abnormal Termination %d", ndx);
			else
				printf("%s", sys_siglist[ndx]);
			if ((status & 0200) != 0)
				printf(" -- Core Dumped");
			printf("\n");
			err++;
			indexx[0377 - ndx]++;
		}
		else
		{
			indexx[(status >> 8) & 0377]++;
		}
	}
	if (err)
	{
		printf("pid list:");
		for (ndx = 0; ndx < pidptr; ndx++)
			printf(" %u", pidlist[ndx]);
		printf("\n");
	}

	/* print index of system errors */
	err = 0;
	for (ndx = 1; ndx <= 0377; ndx++)
	{
		if (indexx[ndx] == 0)
			continue;
		cp = syserrlst(ndx);
		if (!cp)
			break;
		if (err == 0)
			printf("\nUNIX error dictionary:\n");
		printf("%3d: %s\n", ndx, cp);
		if (err == 0)
			err = ndx;
	}
	if (err)
		printf("\n");

	/* PRINT LOGOUT CUE ? */
	if (Nodayfile >= 0)
	{
		time(buf);
		printf("INGRES version %s logout\n%s", Version, ctime(buf));
		if (getuser(Usercode, buf) == 0)
		{
			for (ndx = 0; buf[ndx]; ndx++)
				if (buf[ndx] == ':')
					break;
			buf[ndx] = 0;
			printf("goodbye %s ", buf);
		}
		else
			printf("goodbye ");
		printf("-- come again\n");
	}
#	ifdef xMTR1
	if (tTf(1, 3))
		printf("quit: exit(%d)\n", err);
#	endif
	exit(err);
}
