extern	create();
extern	destroy();
extern	rupdate();
extern	print();
extern	help();
extern	resetrel();

int	(*Func[])() =
{
	&create,
	&destroy,
	&rupdate,
	&print,
	&help,
	&resetrel,
};
