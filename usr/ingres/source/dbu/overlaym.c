extern	modify();
extern	create();
extern	destroy();
extern	resetrel();

int	(*Func[])() =
{
	&modify,
	&create,
	&destroy,
	&resetrel,
};
