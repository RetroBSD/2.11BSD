char	lbuf[100];
char	buf[128];

int	num;
main(argc, argv)
char	*argv[];
{
	register char	*p1, *p2;
	register int	j;
	int	i;

	num = 1;
	if(argc > 1 && *argv[1] == '-') {
		num = argv[1][1] - '0';
		if(num < 1 || num > 9)
			num = 1;
		argv++;
		argc--;
	}

	p1 = argv[1];
	while(*p1) {
		buf[*p1++]++;
	}
	if(num > 1)
		printf("#n\n");
	for(i = 1; i < 10; i++) {
		p2 = lbuf;
		for(j = 'A'; j <= 'Z'; j++) {
			if(buf[j] == i) {
				*p2++ = j;
			}
		}
		for(j = 'a'; j <= 'z'; j++) {
			if(buf[j] == i) {
				*p2++ = j;
			}
		}

		if(p2 == lbuf)	continue;
		*p2 = 0;
		printf("/\\([%s]\\)", lbuf);
		for(j = 0; j < i; j++)
			printf(".*\\1");
		printf("/d\n");
	}
	printf("s/-//g\n");
	printf("/");
	while(num--)
		printf(".");
	printf("/p\n");

}
