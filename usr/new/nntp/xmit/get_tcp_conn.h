/*
** Return codes from get_tcp_conn().
*/
#ifndef	FAIL
#define FAIL		(-1)		/* routine failed */
#endif
#define	NOHOST		(FAIL-1)	/* no such host */
#define	NOSERVICE	(FAIL-2)	/* no such service */

#ifndef NULL
#define	NULL	0
#endif

#ifdef USG	/* brain-dead USG compilers can't deal with typedef */
#define	u_long	unsigned long
#define	u_short	unsigned short
#endif

#ifdef	EXCELAN
#define	NONETDB
#define	OLDSOCKET
#endif

#ifdef	NONETDB
#define	IPPORT_NNTP	119		/* NNTP is on TCP port 119 */
#endif	NONETDB
