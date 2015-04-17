#include "stdio.h"
main()
{
	register c, i;

	printf("___________\n");
	while ((c = getchar()) != EOF)
		putone(c);
	printf("___________\n");
}

putone(byte)
register byte;
{
	register i;

	putchar('|');
	for (i=7; i>=0; --i) {
		if (i==2)
			putchar('.');
		if (byte & (1 << i))
			putchar('o');
		else
			putchar(' ');
	}
	putchar('|');
	putchar('\n');
}
