/* inode structure of version 6 Unix filesystem		*/
struct	inode
{
	int	flags;
	char	nlinks;		/* reference count */
	char	uid;		/*user id*/
	char	gid;
	char	size0;
	int	size1;
	unsigned	addr[8];	/*block numbers*/
	int	actime[2];
	int	modtime[2];
};

/* modes */
#define	IFMT	0170000		/* type of file */
#define	IFALC	0100000		/* allocated */
#define	IFDIR	0040000		/* directory */
#define	IFCHR	0020000		/* character special */
#define	IFBLK	0060000		/* block special */
#define	IFREG	0000000		/* regular */
#define	IFLRG	0010000		/* large file */
#define	ISUID	04000		/* set user id on execution */
#define ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100
