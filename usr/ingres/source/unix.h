#
/*
**	COPYRIGHT
**
**	The Regents of the University of California
**
**	1977
**
**	This program material is the property of the
**	Regents of the University of California and
**	may not be reproduced or disclosed without
**	the prior written permission of the owner.
*/



/*
**  This header file contains everything that we at Berkeley
**	think might vary from system to system.  Before
**	compiling INGRES you should eyeball this file to see
**	if your system is consistant with us.
**
**	History:
**		8/15/79 (eric) (6.2/7) -- added xV7_UNIX flag.
**			Changed 'struct inode' to 'struct stat'
**			for v7 compatibility, and changed the
**			member names.
*/




/*
**  UNIX version flags.
**	The following flags define what version of UNIX is being run.
**	They should be set as follows:
**
**	Bell-style version six UNIX, with 8-bit user id's and 8-bit
**	group id's, fetched with calls to getuid() and getgid()
**	respectively, and stored in the passwd file as two separate
**	eight-bit fields:
**		Set the xV6_UNIX flag only.
**
**	Berkeley-style version 6/7x-05a UNIX, with a single 16-bit
**	user id and no group id (and no getgid() or setgid() calls),
**	and stored in the passwd file as two separate eight-bit fields,
**	combined to make a single 16-bit field:
**		Set the xB_UNIX flag only.
**
**	Bell-style version seven UNIX, with 16-bit user id's and
**	16-bit group id's, fetched with calls to getuid() and
**	getgid() respectively, and stored in the passwd file as
**	two separate 16-bit fields:
**		Set the xV7_UNIX flag only.
*/

/* set for version seven */
/* # define	xV6_UNIX	/* Bell v6 UNIX flag */
# define	xV7_UNIX	/* Bell v7 UNIX flag */
/* # define	xB_UNIX		/* Berkeley UNIX flag */


/*
**  Maximum number of open files per process.
**  Must match 'NOFILE' entry in /usr/sys/param.h
*/

# define	MAXFILES	30


/*
**	USERINGRES is the UNIX login name of the INGRES superuser,
**		normally "ingres" of course.  The root of this persons
**		subtree as listed in /etc/passwd becomes the root of
**		the INGRES subtree.
*/

# define	USERINGRES	"ingres"


/*
**  Structure for 'gtty' and 'stty'
*/

# ifndef xV7_UNIX
struct sgttyb
{
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	int	sg_flags;
};
# endif
# ifdef xV7_UNIX
# include	<sgtty.h>
# endif


/*
**  Structure for 'fstat' and 'stat' system calls.
*/

# ifndef xV7_UNIX
struct stat {
	char	st_minor;	/* +0: minor device of i-node */
	char	st_major;	/* +1: major device */
	int	st_ino;	/* +2 */
	int	st_mode;	/* +4: see below */
	char	st_nlinks;	/* +6: number of links to file */
	char	st_uid;		/* +7: user ID of owner */
	char	st_gid;		/* +8: group ID of owner */
	char	st_sz0;		/* +9: high byte of 24-bit size */
	int	st_sz1;		/* +10: low word of 24-bit size */
	int	st_addr[8];	/* +12: block numbers or device number */
	int	st_actime[2];	/* +28: time of last access */
	int	st_modtime[2];	/* +32: time of last modification */
};

#define	IALLOC	0100000
#define	IFMT	060000
#define		IFDIR	040000
#define		IFCHR	020000
#define		IFBLK	060000
# endif
# ifdef xV7_UNIX
# include	<sys/types.h>
# include	<sys/stat.h>
# endif


/*
**  SETEXIT
**	In version 7, setexit is not defined:  this fakes it.
*/

# ifdef xV7_UNIX
# include	<setjmp.h>
# define	setexit()	setjmp(Sx_buf)
extern jmp_buf	Sx_buf;		/* defined in iutil/reset.c */
# endif
