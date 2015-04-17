/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)saio.h	2.2 (2.11BSD GTE) 1996/3/8
 */

/*
 * This must be done so that the standalone I/O system uses the same
 * size for the inode structure as the utilities (restor, mkfs, etc).
 * See the comments in mkfs.c for more information.
*/
#undef	EXTERNALITIMES

#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/disklabel.h>

/*
 * io block: includes an
 * inode, cells for the use of seek, etc, a buffer
 * and a disklabel.
 */
struct	iob {
	char	i_flgs;
	char	i_ctlr;
	struct	inode	i_ino;
	short	i_unit;
	short	i_part;
	daddr_t	i_boff;
	off_t	i_offset;
	daddr_t	i_bn;
	char	*i_ma;
	int	i_cc;
	char	i_buf[DEV_BSIZE];
	struct	disklabel i_label;
};

#define	F_READ	01
#define	F_WRITE	02
#define	F_ALLOC	04
#define	F_FILE	010
#define	F_TAPE	020
#define	READ	F_READ
#define	WRITE	F_WRITE

#define	READLABEL	0x1
#define	WRITELABEL	0x2
#define	DEFAULTLABEL	0x3

/*
 * device switch
 */
struct	devsw {
	char	*dv_name;
	int	(*dv_strategy)();
	int	(*dv_open)();
	int	(*dv_close)();
	caddr_t	**dv_csr;
	int	(*dv_label)();
	int	(*dv_seek)();
};

/*
 * Set to inhibit 'disklabel missing or corrupt' error messages.  This
 * is normally left off and only set by the standalone disklabeling utility
 * when it expects to be reading an unlabeled disk.
*/
int	Nolabelerr;

extern	char	*itoa();
extern	char	*devname();
