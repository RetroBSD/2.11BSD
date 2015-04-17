#

/*
**  GLOBALS -- INGRES globals which belong everywhere
**
**	Variables in this module should be included by everything
**	INGRES knows about.  The real purpose of this module is
**	so that actual definition of space can occur here (and
**	everything can be 'extern' everywhere else).
**
**	Defines:
**		Alockdes -- the lock descriptor for the concurrency
**			device.
**
**	History:
**		1/24/79 (eric) -- created.
*/






int	Alockdes =	-1;	/* the concurrency device descriptor */
