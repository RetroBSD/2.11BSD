#include <stdio.h>

/*	Initilization of structure containing a union:
 * "tstlgm.c" actually is illegal anyway, since the structure
 * only has four entries.  This file is more legal.  One set of
 * compiler fixes allow this to compile (even correctly).
 */

struct TEST {
	int first;
	int second;
	union UTEST {
		int u_int;
		char *u_cptr;
		int *u_iptr;
	} third;
	int fourth;
};

struct TEST test[] = {
	{0},
	{1},
	{1},
	{1},
	{1,2,3,4,5,6},	/* this is what makes it actually illegal */
	{0},
	{1,2,0,0},
	{1,2,0,0},
	{0,0,0,0},
	{1}
};

main()
{
	int i;

	printf("Size of TEST structure = %d\n",sizeof(struct TEST));
	printf("Size of structure test = %d\n",sizeof(test));

	for (i = 0; i < 10; ++i){
		printf("Cycle %d ",i);
		printf("First = %d\tSecond = %d\t",
			test[i].first, test[i].second);
		printf("Union third = %d\t", test[i].third.u_int);
		printf("Fourth = %d\n", test[i].fourth);
	}
}
