#
/*
**  pipes.h
**
**	Interprocess pipe format and associated manifest constants
*/

# define	PBUFSIZ		120	/* length of pipe buffer */
# define	HDRSIZ		8	/* length of pipe header */

/* -- PIPE PARAMETERS -- */
# define	NORM_STAT	0	/* HDRSTAT: data block */
# define	LAST_STAT	1	/* HDRSTAT: last block of cmnd */
# define	ERR_STAT	2	/* HDRSTAT: error block (sync) */
# define	SYNC_STAT	3	/* sync on delete signal */

struct pipfrmt			/* interprocess pipe format: */
{
	char	exec_id;	/* target overlay id */
	char	func_id;	/* command id in target overlay */
	int	err_id;		/* error status
				    0 - no errors
				    non zero, error identifier	*/
	char	hdrstat;	/* block status word
				    0 - good block in command
				    1 - last block in command	*/
	char	buf_len;	/* # of useful bytes in PBUF
					values 0 to 120.
					rdpipe effectively
					concatenates significant
					parts of blocks.	*/
	char	param_id;	/* one byte param available to calling pgm */
	char	expansion;	/* reserved for expansion */
	char	pbuf[PBUFSIZ];	/* pipe data buffer */
	int	pbuf_pt;	/* next available slot in PBUF */
};


/* modes for rdpipe and wrpipe */
# define	P_PRIME		0	/* prime the pipe */
# define	P_NORM		1	/* normal read or write */
# define	P_SYNC		2	/* read for sync */
# define	P_END		2	/* write a sync block */
# define	P_EXECID	3	/* read execid */
# define	P_FUNCID	4	/* read funcid */
# define	P_FLUSH		3	/* write non-sync block */
# define	P_WRITE		4	/* write block, previous type */
# define	P_INT		5	/* synchronize on delete interrupt */
# define	P_PARAM		6	/* set/get param field */

/* pipe descriptors */
extern int	R_up;		/* Read from process n-1 */
extern int	W_up;		/* Write to process n-1 */
extern int	R_down;		/* Read from process n+1 */
extern int	W_down;		/* Write to process n+1 */
extern int	R_front;	/* Read from front end */
extern int	W_front;	/* Write to front end */
