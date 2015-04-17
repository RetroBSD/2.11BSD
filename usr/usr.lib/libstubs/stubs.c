#include <paths.h>
#include <stdio.h>
#include <sysexits.h>
#include <varargs.h>

#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <utmp.h>

#define	SEND_FD	W[1]
#define	RECV_FD	R[0]

#define	CTIME	1
#define	ASCTIME	2
#define	TZSET	3
#define	LOCALTIME 4
#define	GMTIME	5
#define	OFFTIME	6

#define GETPWENT        7
#define GETPWNAM        8
#define GETPWUID        9
#define SETPASSENT      10
#define ENDPWENT        11

	static	int	R[2], W[2], inited;
	static	char	result[256 + 4];
	static	struct	tm	tmtmp;
	static	struct	passwd	_pw, *getandfixpw();

char	*
ctime(t)
	time_t	*t;
	{
	u_char	fnc = CTIME;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, t, sizeof (*t));
	getb(RECV_FD, result, 26);
	return(result);
	}

char	*
asctime(tp)
	struct	tm	*tp;
	{
	u_char	fnc = ASCTIME;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, result, 26);
	return(result);
	}

void
tzset()
	{
	u_char	fnc = TZSET;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	}

struct	tm *
localtime(tp)
	time_t	*tp;
	{
	u_char	fnc = LOCALTIME;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	getb(RECV_FD, result, 24);
	tmtmp.tm_zone = result;
	return(&tmtmp);
	}

struct	tm *
gmtime(tp)
	time_t	*tp;
	{
	u_char	fnc = GMTIME;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, tp, sizeof (*tp));
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	getb(RECV_FD, result, 24);
	tmtmp.tm_zone = result;
	return(&tmtmp);
	}

struct	tm *
offtime(clock, offset)
	time_t	*clock;
	long	offset;
	{
	u_char	fnc = OFFTIME;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, clock, sizeof (*clock));
	write(SEND_FD, &offset, sizeof offset);
	getb(RECV_FD, &tmtmp, sizeof tmtmp);
	tmtmp.tm_zone = "";
	return(&tmtmp);
	}

struct passwd *
getpwent()
	{
	u_char	fnc = GETPWENT;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	return(getandfixpw());
	}

struct	passwd *
getpwnam(nam)
	char	*nam;
	{
	u_char	fnc = GETPWNAM;
	char	lnam[UT_NAMESIZE + 1];
	int	len;

	len = strlen(nam);
	if	(len > UT_NAMESIZE)
		len = UT_NAMESIZE;
	bcopy(nam, lnam, len);
	lnam[len] = '\0';

	sewer();
	write(SEND_FD, &fnc, 1);
	write(SEND_FD, &len, sizeof (int));
	write(SEND_FD, lnam, len);
	return(getandfixpw());
	}

struct	passwd	*
getpwuid(uid)
	uid_t	uid;
	{
	u_char	fnc = GETPWUID;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, &uid, sizeof (uid_t));
	return(getandfixpw());
	}

setpwent()
	{
	return(setpassent(0));
	}

setpassent(stayopen)
	int	stayopen;
	{
	u_char	fnc = SETPASSENT;
	int	sts;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	write(SEND_FD, &stayopen, sizeof (int));
	getb(RECV_FD, &sts, sizeof (int));
	return(sts);
	}

void
endpwent()
	{
	u_char	fnc = ENDPWENT;

	sewer();
	write(SEND_FD, &fnc, sizeof fnc);
	return;
	}

/* setpwfile() is deprecated */
void
setpwfile(file)
	char	*file;
	{
	return;
	}

struct passwd *
getandfixpw()
	{
	short	sz;

	getb(RECV_FD, &sz, sizeof (int));
	if	(sz == 0)
		return(NULL);
	getb(RECV_FD, &_pw, sizeof (_pw));
	getb(RECV_FD, result, sz);
	_pw.pw_name += (int)result;
	_pw.pw_passwd += (int)result;
	_pw.pw_class += (int)result;
	_pw.pw_gecos += (int)result;
	_pw.pw_dir += (int)result;
	_pw.pw_shell += (int)result;
	return(&_pw);
	}

getb(f, p, n)
	register int f, n;
	register char *p;
	{
	int	i;

	while	(n)
		{
		i = read(f, p, n);
		if	(i <= 0)
			return;
		p += i;
		n -= i;
		}
	}

sewer()
	{
	register int	pid, ourpid = getpid();

	if	(inited == ourpid)
		return;
	if	(inited)
		{
		close(SEND_FD);
		close(RECV_FD);
		}
	pipe(W);
	pipe(R);
	pid = vfork();
	if	(pid == 0)
		{			/* child */
		alarm(0);		/* cancel alarms */
		dup2(W[0], 0);		/* parent write side to our stdin */
		dup2(R[1], 1);		/* parent read side to our stdout */
		close(SEND_FD);		/* copies made, close the... */
		close(RECV_FD);		/* originals now */
		execl(_PATH_CTIMED, "ctimed", 0);
		_exit(EX_OSFILE);
		}
	if	(pid == -1)
		abort();		/* nothing else really to do */
	close(W[0]);			/* close read side of SEND channel */
	close(R[1]);			/* close write side of RECV channel */
	inited = ourpid;		/* don't do this again in this proc */
	}

XXctime()
	{

	if	(SEND_FD)
		close(SEND_FD);
	if	(RECV_FD)
		close(RECV_FD);
	SEND_FD = RECV_FD = 0;
	inited = 0;
	}
