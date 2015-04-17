#if	NHP > 0				/* RJP04/06, RWP04/06 */
	DEVTRAP(254,	hpintr,	br5)
#endif

#if NHP > 0				/* RJP04/06, RWP04/06 */
	HANDLER(hpintr)
#endif

