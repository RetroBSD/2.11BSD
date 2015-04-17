null()
{
	return (0);

}

extern int	openr(), closer();
int	(*Func[])() =
{
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	&null,
	/* the functions below are needed only for compilation
	** and is not intended to be executed
	*/
	&openr,
	&closer,
};
