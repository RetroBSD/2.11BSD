extern	index();
extern	create();
extern	display();
extern  dest_const();

int	(*Func[])() =
{
	&index,
	&create,
	&display,
	&dest_const,
};
