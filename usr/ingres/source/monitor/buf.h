#

/*
**  BUF.H -- buffer definitions
*/

# define	BUFSIZE		256

struct buf
{
	struct buf	*nextb;
	char		buffer[BUFSIZE];
	char		*ptr;
};
