#if	!defined(lint) && defined(DOSCCS)
static char *sccsid = "@(#)ildr.h	1.0	(2.11BSD GTE)	2/14/95";
#endif	lint

/*
 * This is the include file for the Ingres concurrency (lock) driver.
 * Only the parameters NLOCKS, PLOCKS, RLOCKS and DLOCKS may be changed.
 *
 * The initial values allow for 10 simultaneous Ingres users on the system.
 * This should be enough but if you wish to raise that limit then change
 * _only_ the 'IL_DLOCKS' parameter below.
*/

#define	KEYSIZE	12
/* max number of data base locks (max # of Ingreses) */
#define	IL_DLOCKS	10
#define	IL_RLOCKS	((2*IL_DLOCKS) + 6)
#define	IL_PLOCKS	(IL_RLOCKS + 3)
#define	IL_NLOCKS	(IL_PLOCKS + 1)

/*
 * Do not change anything below this line unless you are modifying the
 * driver.
*/

#define	LOCKPRI		1
#define	TRUE		1
#define	FALSE		0
#define	M_EMTY		0
#define	M_SHARE		2
#define	M_EXCL		1
#define	T_CS		0
#define	T_PAGE		1
#define	T_REL		2
#define	T_DB		3
#define	A_RTN		1
#define	A_SLP		2
#define	A_RLS1		3
#define	A_RLSA		4
#define	A_ABT		5
#define	W_ON		1
#define	W_OFF		0

/*
 *	data structure for Lock table
 */
struct	Lockform
	{
	int	l_pid;
	char	l_wflag;	/* wait flag: = 1 a process is waiting*/
	char	l_type;		/* type of lock:
					= 0 for critical section
					= 1 for page
					= 2 for logical
					= 3 for data base
				*/
	char	l_mod;		/* mod of Lock or lock action requested 
				 *	= 0 slot empty
				 *	= 1 exclusive lock
				 *	= 2 shared lock
				 */
	char	l_pad;
	char	l_key[KEYSIZE];
	};

/*
 * This is the structure written by Ingres programs.
 *
 * This structure _must_ agree in size and alignment with the 'lockreq'
 * structure defined in /usr/ingres/source/lock.h
*/

struct	ulock
	{
	char	_act;		/* requested action:
				 *	=1 request lock, err return
				 *	=2 request lock, sleep
				 *	=3 release lock
				 *	=4 release all locks for pid
				 */
	char	_type;		/* same as Locktab l_type */
	char	_mod;		/* same as Locktab l_mod */
	char	_pad;		/* Pad byte to align key on word boundary */
	char	_key[KEYSIZE];	/* requested key	*/
	};

struct Lockreq
	{
	int	lr_pid;		/* requesting process id */
	struct	ulock lr_req;	/* The structure written by the user */
	};

#define	lr_act	lr_req._act
#define	lr_type	lr_req._type
#define	lr_mod	lr_req._mod
#define	lr_key	lr_req._key

#define	ingres_maplock()	mapseg5(Locktabseg.se_addr, Locktabseg.se_desc)
#define	LOCKTABSIZE	(IL_NLOCKS * sizeof (struct Lockform))

	extern	segm	Locktabseg;
