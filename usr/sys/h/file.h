/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)file.h	1.3 (2.11BSD GTE) 1/19/95
 */

#include <fcntl.h>

#ifndef	_SYS_FILE_H_
#define	_SYS_FILE_H_

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	char	f_type;		/* descriptor type */
	u_char	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
	union {
		caddr_t	f_Data;
		struct socket *f_Socket;
	} f_un;
	off_t	f_offset;
};

#ifdef KERNEL
struct	fileops {
	int	(*fo_rw)();
	int	(*fo_ioctl)();
	int	(*fo_select)();
	int	(*fo_close)();
};

#define f_data		f_un.f_Data
#define f_socket	f_un.f_Socket

#ifndef SUPERVISOR
extern struct	file file[], *fileNFILE;
int	nfile;
#endif

struct	file *getf();
struct	file *falloc();
#endif

/*
 * Access call.
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#ifdef KERNEL
#define	GETF(fp, fd) { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}
#define	DTYPE_INODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#define	DTYPE_PIPE	3	/* I don't want to hear it, okay? */
#endif
#endif	/* _SYS_FILE_H_ */
