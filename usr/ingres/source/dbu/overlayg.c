extern int	create();
extern int	destroy();
extern int	rupdate();
extern int	print();
extern int	help();
extern int	resetrel();
extern int	copy();
extern int	save();
extern int	modify();
extern int	index();
extern int	display();
extern int	dest_const();

int	(*Func[])() =
{
	&create,
	&destroy,
	&rupdate,
	&print,
	&help,
	&resetrel,
	&copy,
	&save,
	&modify,
	&index,
	&display,
	&dest_const,
};
