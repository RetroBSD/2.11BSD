int zero = 0;
int selfinv;
int i;
int j;
int k;

main()
{
	selfinv = ~(((unsigned)~0) >> 1);
	i = selfinv + 0101;	/* neg */
	j = selfinv - 0100;	/* pos */
	printf("everything should be true\n");
	printf("selfinv = 0%o\n", selfinv);
	printf("zero = %d\n", zero);
	printf("i = 0%o = %d.\n", i, i);
	printf("j = 0%o = %d.\n", j, j);
	printf("j-i = 0%o = %d.\n", j - i);
	printf("j - i < 0\t");
	if ((j-i) < 0)
		printf("true\n");
	else
		printf("false\n");
	k = j - i;
	printf("k=j-i; (k < 0)\t");
	if (k < 0)
		printf("true\n");
	else
		printf("false\n");
	printf("j - i < zero\t");
	if (j - i < zero)
		printf("true\n");
	else
		printf("false\n");
	printf("(j-i < 0) == (j - i < zero)\t");
	if ((j-i < 0) == (j - i < zero))
		printf("true\n");
	else
		printf("false\n");
}
