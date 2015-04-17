/* Structure of the version 6 super-block	*/
struct	filsys {
	int	isize;	/* size in blocks of i-list */
	int	fsize;   	/* size in blocks of entire volume */
	int  	nfree;   	/* number of addresses in free */
	int	free[100];	/* free block list */
	int  	ninode;  	/* number of i-nodes in inode */
	int  	finode[100];	/* free i-node list */
	char   	flock;   	/* lock during free list manipulation */
	char   	ilock;   	/* lock during i-list manipulation */
	char   	fmod;    	/* super block modified flag */
	int 	time[2];    	/* last super block update */
};
