main()
{
	register foo;
	int bar;
	foo = 4;
	bar = 6;
	asm("dec r4");
	asm("inc 177766(r5)");
	printf("4-1=%d, 6+1=%d\n", foo, bar);
}
