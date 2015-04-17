#include <stdio.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include "./cpu.h"

double ldvec[3];

char *days[] = { "Sunday", "Monday", "Tuesday", "Wednesday",
		 "Thursday", "Friday", "Saturday" };

char *mons[] = { "January", "February", "March", "April",
		 "May", "June", "July", "August",
		 "September", "October", "November", "December" };

char *ttime[] = { "morning ", "afternoon ", "evening ", "night " };

main()
{
	register struct tm *det;
	size_t size;
	int a, b, x, y, mib[2];
	long secs;
	char	bot[100], *foo, goo[50], ap;
	char	machine[64], model[64], ostype[64];

	time(&secs);
	(void)getloadavg(ldvec, 3);

	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	size = sizeof (machine);
	if	(sysctl(mib, 2, machine, &size, NULL, 0) < 0)
		{
		printf("Can't get machine type\n");
		strcpy(machine, "?");
		}
	mib[0] = CTL_HW;
	mib[1] = HW_MODEL;
	size = sizeof (model);
	if	(sysctl(mib, 2, model, &size, NULL, 0) < 0)
		{
		printf("Can't get cpu type\n");
		strcpy(model, "?");
		}
	mib[0] = CTL_KERN;
	mib[1] = KERN_OSTYPE;
	size = sizeof (ostype);
	if	(sysctl(mib, 2, ostype, &size, NULL, 0) < 0)
		{
		printf("Can't get ostype\n");
		strcpy(ostype, "?");
		}

	ap = "AP"[(det = localtime(&secs))->tm_hour >= 12];
	if (det->tm_hour >= 0 && det->tm_hour < 12)
		foo = ttime[0];
	else if (det->tm_hour > 11 && det->tm_hour < 18)
		foo = ttime[1];
	else if (det->tm_hour > 17 && det->tm_hour < 24)
		foo = ttime[2];
	if ((det->tm_hour %= 12) == 0)
		det->tm_hour = 12;
	fflush(stdout);
	printf("\033[2J\033(0\033)0\033[m");
	printf("\033[3;11H\033(B\033)B   Load Average\033(0\033)0");
	printf("\033[4;11Hlqqqqqqqqqqqqqqqqk");
	printf("\033[5;11Hx                x");
	printf("\033[6;11Hmqqqqqqqqqqqqqqwqj");
	printf("\033[5;13H%.02f %.02f %.02f", ldvec[0], ldvec[1], ldvec[2]);
	printf("\033[7;16Hlqqqqqqqqqvqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk");
#ifdef	TTY
	printf("\033[3;51H\033(B\033)BTTY Name\033(0\033)0");
	printf("\033[4;46Hlqqqqqqqqqqqqqqqqk");
	printf("\033[5;46Hx                x");
	printf("\033[6;46Hmqwqqqqqqqqqqqqqqj");
	printf("\033[5;50H\033(B\033)B%s\033(0\033)0",ttyname(ttyslot(0)));
	printf("\033[7;16Hlqqqqqqqqqvqqqqqqqqqqqqqqqqqqqqqvqqqqqqqqk");
#else   TTY
	printf("\033[7;16Hlqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk");
#endif	TTY
	printf("\033[8;16Hx                                        x");
	printf("\033[9;13Hlqqvqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqvqqk");
	printf("\033[10;10Hlqqvqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqvqqk");
	printf("\033[11;10Hx\033[11;63Hx\033[12;10Hx\033[12;63Hx\033[13;10Hx\033[13;63Hx");
	printf("\033[14;10Hmqqwqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqwqqj");
	printf("\033[15;13Hmqqwqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqwqqj");
	printf("\033[16;16Hx                                        x");
	printf("\033[17;16Hmqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj");
	printf("\033(B\033)B");
#ifdef  LIGHT
	printf("\033[11;13H\033[7m                                                ");
#endif  LIGHT
	printf("\033[11;13H    Good %s%s,", foo, getenv("USER"));
	gethostname(goo, sizeof(goo));
#ifdef  LIGHT
	printf("\033[12;13H\033[7m                                                ");
	printf("\033[12;13H\033[7m        You have connected to %s", goo);
#else   LIGHT
	printf("\033[12;13H        You have connected to %s", goo);
#endif
#ifdef  LIGHT
	printf("\033[13;13H\033[7m                                                ");
#endif  LIGHT
	printf("\033[m");
#ifdef	LIGHT
	printf("\033[13;13H\033[7m       A %s/%s running %s",
#else
	printf("\033[13;13H       A %s/%s running %s",
#endif
		machine, model, ostype);

#ifdef	UVERS
	fp = fopen("/etc/motd", "r");
	fscanf(fp, "%s%s", poo, doo);
	printf(" V%c.%c", doo[1], doo[3]);
	fclose(fp);
	unlink("/tmp/foo");
#endif	UVERS
#ifdef	LIGHT
	printf("\033[8;18H                                      ");
	printf("\033[16;18H                                      ");
#endif	LIGHT
	x = 40 - (strlen(days[det->tm_wday]) / 2);
	y = (69 - x);
	printf("\033[8;%dH%s", y, days[det->tm_wday]);
	sprintf(bot, "%s %d, 19%d %d:%02d %cM", mons[det->tm_mon],
	        det->tm_mday, det->tm_year, det->tm_hour,
		det->tm_min, ap);
	a = 40 -(strlen(bot) / 2);
	b = (54 - a);
	printf("\033[m");
#ifdef LIGHT
	printf("\033[7m");
#endif LIGHT
	printf("\033[16;%dH%s", b, bot);
	printf("\033[23;1H");
	printf("\033[m");
	exit(0);
}
