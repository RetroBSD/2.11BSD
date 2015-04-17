/*
** Program to produce reliable locks for shell scripts.
** Algorithmn suggested by Peter Honeyman, January 1984,
** in connection with HoneyDanBer UUCP.
**
** I tried extending this to handle shared locks in November 1987,
** and ran into to some fundamental problems:
**
**	Neither 4.3 BSD nor System V have an open(2) with locking,
**	so that you can open a file and have it locked as soon as
**	it's real; you have to make two system calls, and there's
**	a race...
**
**	When removing dead process id's from a list in a file,
**	you need to truncate the file (you don't want to create a
**	new one; see above); unfortunately for the portability of
**	this program, only 4.3 BSD has ftruncate(2).
**
** Erik E. Fair <fair@ucbarpa.berkeley.edu>, November 8, 1987
*/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>			/* Needed on hpux */
#include <sys/file.h>
#include <errno.h>

#define	LOCK_SET	0
#define	LOCK_FAIL	1

#define	FAIL		(-1)

#define	TRUE	1
#define	FALSE	0

#ifdef USG
#define	index	strchr
#define	rindex	strrchr
#endif

int	Debug = FALSE;
char	*Pname;
char	*USAGE = "%s: USAGE: shlock -f file -p pid [-d]\n";
char	*errmsg();
char	*xtmpfile();

#define	dprintf	if (Debug) printf

extern	int	errno;
extern	char	*rindex();
extern	char	*strcpy();
extern	char	*strcat();

main(ac, av)
int	ac;
char	*av[];
{
	register int	x;
	char	*file;
	int	pid;

	Pname = ((Pname = rindex(av[0], '/')) ? Pname + 1 : av[0]);

	for(x = 1; x < ac; x++) {
		if (av[x][0] == '-') {
			switch(av[x][1]) {
			case 'd':
				Debug = TRUE;
				break;
			case 'p':
				if (strlen(av[x]) > 2) {
					pid = atoi(&av[x][2]);
				} else {
					pid = atoi(av[++x]);
				}
				break;
			case 'f':
				if (strlen(av[x]) > 2) {
					file = &av[x][2];
				} else {
					file = av[++x];
				}
				break;
			default:
				fprintf(stderr, USAGE, Pname);
				exit(LOCK_FAIL);
			}
		}
	}
	if (pid == 0 || file == (char *)NULL) {
		fprintf(stderr, USAGE, Pname);
		exit(LOCK_FAIL);
	}

	exit(mklock(file, pid) ? LOCK_SET : LOCK_FAIL);
}

char *
errmsg(n)
register int	n;
{
	return(strerror(n));
}

mklock(file, pid)
char	*file;
int	pid;
{
	register char	*tmp;
	register int	retcode = FALSE;
	char	*e_unlk = "%s: unlink(%s): %s\n";

	dprintf("%s: trying lock <%s> for process %d\n", Pname, file, pid);
	if ((tmp = xtmpfile(file, pid)) == (char *)NULL)
		return(FALSE);

linkloop:
	if (link(tmp, file) < 0) {
		switch(errno) {
		case EEXIST:
			dprintf("%s: lock <%s> already exists\n", Pname, file);
			if (cklock(file)) {
				dprintf("%s: extant lock is valid\n", Pname);
				break;
			} else {
				dprintf("%s: lock is invalid, removing\n",
					Pname);
				if (unlink(file) < 0) {
					fprintf(stderr, e_unlk,
						Pname, file, errmsg(errno));
					break;
				}
			}
			/*
			** I hereby profane the god of structured programming,
			** Edsgar Dijkstra
			*/
			goto linkloop;
		default:
			fprintf(stderr, "%s: link(%s, %s): %s\n",
				Pname, tmp, file, errmsg(errno));
			break;
		}
	} else {
		dprintf("%s: got lock <%s>\n", Pname, file);
		retcode = TRUE;
	}
	if (unlink(tmp) < 0) {
		fprintf(stderr, e_unlk, Pname, tmp, errmsg(errno));
	}
	return(retcode);
}

/*
** Does the PID exist?
** Send null signal to find out.
*/
p_exists(pid)
int	pid;
{
	dprintf("%s: process %d is ", Pname, pid);
	if (pid <= 0) {
		dprintf("invalid\n");
		return(FALSE);
	}
	if (kill(pid, 0) < 0) {
		switch(errno) {
		case ESRCH:
			dprintf("dead\n");
			return(FALSE);	/* pid does not exist */
		case EPERM:
			dprintf("alive\n");
			return(TRUE);	/* pid exists */
		default:
			dprintf("state unknown: %s\n", errmsg(errno));
			return(TRUE);	/* be conservative */
		}
	}
	dprintf("alive\n");
	return(TRUE);	/* pid exists */
}

/*
** Check the validity of an existing lock file.
**
**	Read the PID out of the lock
**	Send a null signal to determine whether that PID still exists
**	Existence (or not) determines the validity of the lock.
**
**	Two bigs wins to this algorithmn:
**
**	o	Locks do not survive crashes of either the system or the
**			application by any appreciable period of time.
**
**	o	No clean up to do if the system or application crashes.
**
*/

cklock(file)
char	*file;
{
	register int	fd = open(file, O_RDONLY);
	register int	len;
	char	buf[BUFSIZ];

	dprintf("%s: checking extant lock <%s>\n", Pname, file);
	if (fd < 0) {
		fprintf(stderr,"%s: open(%s): %s\n", Pname, file, errmsg(errno));
		return(TRUE);	/* might or might not; conservatism */
	}

	if ((len = read(fd, buf, sizeof(buf))) <= 0) {
		close(fd);
		dprintf("%s: lock file format error\n", Pname);
		return(FALSE);
	}
	close(fd);
	buf[len + 1] = '\0';
	return(p_exists(atoi(buf)));
}

/*
** Create a temporary file, all ready to lock with.
** The file arg is so we get the filename right, if he
** gave us a full path, instead of using the current directory
** which might not be in the same filesystem.
*/
char *
xtmpfile(file, pid)
char	*file;
int	pid;
{
	register int	fd;
	register int	len;
	char	*cp, buf[BUFSIZ];
	static char	tempname[BUFSIZ];

	sprintf(buf, "shlock%d", getpid());
	if ((cp = rindex(strcpy(tempname, file), '/')) != (char *)NULL) {
		*++cp = '\0';
		(void) strcat(tempname, buf);
	} else
		(void) strcpy(tempname, buf);
	dprintf("%s: temporary filename: %s\n", Pname, tempname);

	sprintf(buf, "%d\n", pid);
	len = strlen(buf);
openloop:
	if ((fd = open(tempname, O_RDWR|O_CREAT|O_EXCL, 0644)) < 0) {
		switch(errno) {
		case EEXIST:
			dprintf("%s: file %s exists already.\n",
				Pname, tempname);
			if (unlink(tempname) < 0) {
				fprintf(stderr, "%s: unlink(%s): %s\n",
					Pname, tempname, errmsg(errno));
				return((char *)NULL);
			}
			/*
			** Further profanity
			*/
			goto openloop;
		default:
			fprintf(stderr, "%s: open(%s): %s\n",
				Pname, tempname, errmsg(errno));
			return((char *)NULL);
		}
	}

	/*
	** Write the PID into the temporary file before attempting to link
	** to the actual lock file. That way we have a valid lock the instant
	** the link succeeds.
	*/
	if (write(fd, buf, len) < 0) {
		fprintf(stderr, "%s: write(%s,%d): %s\n",
			Pname, tempname, pid, errmsg(errno));
		(void) close(fd);
		return((char *)NULL);
	}
	(void) close(fd);
	return(tempname);
}
