#include	<stdio.h>
#include	<sys/types.h>
#include	<pwd.h>
#include	<time.h>

main()
	{
	struct	tm	*tm;
	struct	passwd	*pw;
	char	*cp;
	time_t	l;
	uid_t	uid;

	l = time(0);
	cp = ctime(&l);
	printf("ctime(%ld) = %s", l, cp);
	tm = gmtime(&l);
	dump_tm("gmtime", tm);
	cp = asctime(tm);
	printf("asctime(tm) = %s", cp);
	tm = localtime(&l);
	dump_tm("localtime", tm);

	if	(setpassent(1))
		printf("setpassent(1) returned non-zero (success) status\n");
	else
		printf("setpassent(1) returned zero (shouldn't happen)\n");

	printf("getpwent\n");
	while	(pw = getpwent())
		{
		dump_pw(pw);
		}
	printf("getpwnam(root)\n");
	pw = getpwnam("root");
	if	(!pw)
		printf("OOPS - root doesn't apparently exist\n");
	else
		dump_pw(pw);
	printf("getpwnam(nobody)\n");
	pw = getpwnam("nobody");
	if	(!pw)
		printf("OOPS - nobody doesn't apparently exist\n");
	uid = pw->pw_uid;
	printf("getpwuid\n");
	pw = getpwuid(uid);
	if	(!pw)
		printf("OOPS - uid %u (from nobody) doesn't exist\n", uid);
	else
		dump_pw(pw);

	printf("endpwent\n");
	endpwent();
	}

dump_tm(str, tm)
	char	*str;
	struct	tm *tm;
	{
	printf("%s sec: %d min: %d hr: %d mday: %d mon: %d yr: %d wday: %d yday: %d isdst: %d zone: %s gmtoff: %d\n",
		str,
		tm->tm_sec,
		tm->tm_min,
		tm->tm_hour,
		tm->tm_mday,
		tm->tm_mon,
		tm->tm_year,
		tm->tm_wday,
		tm->tm_yday,
		tm->tm_isdst,
		tm->tm_zone,
		tm->tm_gmtoff);
	}

dump_pw(p)
	struct	passwd *p;
	{
	printf("%s:%s:%u:%u:%s:%ld:%ld:%s:%s:%s\n",
		p->pw_name,
		p->pw_passwd,
		p->pw_uid, p->pw_gid,
		p->pw_class, p->pw_change, p->pw_expire,
		p->pw_gecos, p->pw_dir, p->pw_shell);
	}
