/*
 * Dungeon - open UP dungeon
 */
#include	<stdio.h>
#include <time.h>

#define GUEST 16

int users[] = {
	3,	/* bin */
/*	17,*/
	22,
	0 };

main(argc,argv)
	int	argc;
	char	*argv[];
{

	register char	*pname = "zork";
	register int *up;
	register uid;
	int fd3, fd4, fd5;

	int wizard = 0;

	uid = getuid();
	for (up=users; *up; up++)
		if (*up == uid)
			wizard++;

	if(argc == 2)
		if(wizard != 0)
			pname = argv[0];
	/*
	 * open up files needed by program
	 * look in current directory first, then try default names
	 * The following files must be as follows:
	 * "dtext.dat" open read-only on fd 3
	 * "dindex.dat" open read-only on fd 4 
	 * "doverlay" open read-only on fd 5 (put this file on fast disk)
	 */
	close(3);
	close(4);
	close(5);

	if ((fd3 = open("dtext.dat",0)) < 0)
	    if ((fd3 = open("/usr/games/lib/dtext.dat", 0)) < 0)
		error("Can't open dtext.dat\n");

	if ((fd4 = open("dindex.dat", 0)) < 0)
	    if ((fd4 = open("/usr/games/lib/dindex.dat", 0)) < 0)
		error("Can't open dindex.dat\n");

	if ((fd5 = open("doverlay",0)) < 0)
	    if ((fd5 = open("/usr/games/lib/doverlay", 0)) < 0)
		error("Can't open doverlay\n");

	if (fd3 != 3 || fd4 != 4 || fd5 != 5)
		error("Files opened on wrong descriptors\n");

	signal (2,1);

/*	if(wizard == 0)
		datecheck();*/

	printf(">\t\t\tSaves game on file dsave.dat\n");
	printf("<\t\t\tRestores game from file dsave.dat\n");
	printf("!command\t\tExecute shell command from Dungeon\n");
	printf("\n\n");

	printf("Welcome to Dungeon.\t\tThis version created 10-SEP-78.\n");
	printf("You are in an open field west of a big white house with a boarded\n");
	printf("front door.\n");
	printf("There is a small mailbox here.\n>");
	fflush(stdout);
	nice(2);
	execl("/usr/games/lib/zork",pname, 0);
	execl("zork",pname, 0);
	printf("Can't start zork.\n");
	fflush(stdout);
	exit(1);
}
error(s)
char *s;
{
	printf("%s", s);
	fflush(stdout);
	exit(1);
}

struct	hol_dom
	{
		int	m_month;
		int	m_day;
		char	*m_msg;
	};

struct hol_dom	hol_dom[] =
	{
		 0,  1, "Happy New Year",
		 6,  4, "Happy Fourth of July",
		11, 25, "Merry Christmas",
		-1,  0, 0,
	};

struct hol_dow
	{
		int	w_month;
		int	w_day;
		int	w_weekskip;
		char	*w_msg;
	};

struct hol_dow	hol_dow[] =
	{
		 8,  1,  0, "Happy Labor Day",
		10,  4,  3, "Happy Thanksgiving",
		10,  5,  3, "Happy Day-After-Thanksgiving",
		-1,  0,  0, 0,
	};

datecheck() {
	long tm;
	register struct tm *l;
	register struct hol_dom	*m;
	register struct hol_dow *w;

	time(&tm);
	l = localtime(&tm);
	if(l->tm_wday == 0 || l->tm_wday == 6)
		return;
	if(l->tm_hour <= 8 || l->tm_hour >= 17)
		return;
	for(m = hol_dom ; m->m_month < 0 ; m++)
	{
		if(l->tm_mon == m->m_month && l->tm_mday == m->m_day)
			return;
	}
	for(w = hol_dow ; w->w_month < 0 ; w++)
	{
		if(l->tm_mon == w->w_month
		  && l->tm_mday == w->w_day
		  && (7 * w->w_weekskip) < l->tm_mday
		  && l->tm_mday >= (7 * (w->w_weekskip + 1)))
			return;
	}
	goaway();
}

goaway () {
	printf("\nThere appears before you a threatening figure clad all over\n");
	printf("in heavy black armor.  His legs seem like the massive trunk\n");
	printf("of the oak tree.  His broad shoulders and helmeted head loom\n");
	printf("high over your own puny frame, and you realize that his powerful\n");
	printf("arms could easily crush the very life from your body.  There\n");
	printf("hangs from his belt a veritable arsenal of deadly weapons:\n");
	printf("sword, mace, ball and chain, dagger, lance, and trident.\n");
	printf("He speaks with a commanding voice:\n\n");
	printf("                    You shall not pass.\n\n");
	printf("As he grabs you by the neck all grows dim about you.\n");
	sleep (2);
	fflush(stdout);
	exit(1);
}
