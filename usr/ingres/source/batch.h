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



# define	BATCHSIZE	506	/* available buffer space */
# define	IDSIZE		6	/* size of file id */

struct batchbuf
{
	char	file_id[IDSIZE];	/* unique file name identifier */
	char	bbuf[BATCHSIZE];	/* buffer for batch storage */
};


struct si_doms
{
	int	rel_off;	/* offset in primary tuple */
	int	tupo_off;	/* offset in saved tuple-old */
	int	dom_size;	/* width of the domain */
				/* if zero then domain not used */
};
struct batchhd
{
	char	db_name[15];	/* data base name */
	char	rel_name[13];	/* relation name */
	char	userid[2];	/* ingres user code */
	long	num_updts;	/* actual number of tuples to be updated */
	int	mode_up;	/* type of update */
	int	tido_size;	/* width of old_tuple_id field */
	int	tupo_size;	/* width of old tuple */
	int	tupn_size;	/* width of new tuple */
	int	tidn_size;	/* width of new_tuple_id field */
	int	si_dcount;	/* number of sec. index domains affected */
	struct si_doms	si[MAXDOM+1];	/* entry for each domain with sec. index */
};



int	Batch_fp;	/* file descriptor for batch file */
int	Batch_cnt;	/* number of bytes taken from the current buffer */
int	Batch_dirty;	/* used during update to mark a dirty page */
int	Batch_lread;	/* number of bytes last read in readbatch() */
int	Batch_recovery;	/* TRUE is this is recovery, else FALSE */

extern char	*Fileset;	/* unique id of batch maker */
struct batchbuf	Batchbuf;
struct batchhd	Batchhd;
# define	MODBATCH	"_SYSmod"
# define	MODTEMP		"_SYSnewr"
# define	ISAM_SORTED	"_SYSsort"
# define	ISAM_DESC	"_SYSdesc"
# define	ISAM_SPOOL	"_SYSspol"
# define	MOD_PREBATCH	"_SYSpreb"
