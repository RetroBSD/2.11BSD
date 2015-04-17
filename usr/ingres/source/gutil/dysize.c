
int dysize(year)
int year;
{
	if (!(year % 4))
		if (year % 400)
			return(366);
	return(365);
}
