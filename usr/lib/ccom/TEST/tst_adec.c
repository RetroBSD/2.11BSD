int	arr[100], x;
long	p_end;
main()
{
	puts("test autodecrement inside array reference:");
	p_end = 0;
	printf("before %ld (should be 0)\n", p_end);
	x = arr[p_end--];
	printf("after %ld (should be -1)\n", p_end);
}
