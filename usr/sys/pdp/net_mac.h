/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	net_mac.h 2.0 (2.11BSD) 1997/2/14
 */

struct socket *asoqremque();
#define	ASOQREMQUE(so, n) \
	KScall(asoqremque, sizeof(struct socket *) + sizeof(int), so, n)

int b_to_q();
#define	B_TO_Q(cp, cc, q) \
	SKcall(b_to_q, sizeof(char *) + sizeof(int) + sizeof(struct clist *), \
	    cp, cc, q)

int connwhile();
#define	CONNWHILE(so) \
	KScall(connwhile, sizeof(struct socket *), so)

memaddr malloc();
#define	MALLOC(map, size) \
	(memaddr)SKcall(malloc, sizeof(struct map *) + sizeof(u_int), \
	    map, size)

int fpfetch();
#define	FPFETCH(fp, fpp) \
	SKcall(fpfetch, sizeof(struct file *) + sizeof(struct file *), \
	    fp, fpp)

void fpflags();
#define	FPFLAGS(fp, set, clear) \
	SKcall(fpflags, sizeof(struct file *) + sizeof(int) + \
	    sizeof(int), fp, set, clear)

int gsignal();
#define	GSIGNAL(pgrp, sig) \
	SKcall(gsignal, sizeof(int) + sizeof(int), pgrp, sig)

struct mbuf *m_free();
#define	M_FREE(m) \
	KScall(m_free, sizeof(struct mbuf *), m)

int m_freem();
#define	M_FREEM(m) \
	KScall(m_freem, sizeof(struct mbuf *), m)

int netcopyout();
#define	NETCOPYOUT(from, to, len) \
	KScall(netcopyout, sizeof(struct mbuf *) + sizeof(char *) + \
	    sizeof(u_int), from, to, len)

int netcrash();
#define	NETCRASH() \
	SKcall(netcrash, 0)

int iput();
#define	IPUT(ip) \
	SKcall(iput, sizeof(struct inode *), ip)

struct proc *netpfind();
#define	NETPFIND(pid) \
	SKcall(netpfind, sizeof(int), pid)

int netpsignal();
#define	NETPSIGNAL(p, sig) \
	SKcall(netpsignal, sizeof(struct proc *) + sizeof(int), p, sig)

void netsethz();
#define	NETSETHZ() \
	KScall(netsethz, sizeof (hz), hz)

int netstart();
#define	NETSTART() \
	KScall(netstart, 0)

int putc();
#define	PUTC(c, p) \
	SKcall(putc, sizeof(int) + sizeof(struct clist *), c, p)

int selwakeup();
#define	SELWAKEUP(p, coll) \
	SKcall(selwakeup, sizeof(struct proc *) + sizeof(long), p, coll)

int sleep();
#define	SLEEP(chan, pri) \
	SKcall(sleep, sizeof(caddr_t) + sizeof(int), SUPERADD(chan), pri)

int soacc1();
#define	SOACC1(so) \
	KScall(soacc1, sizeof(struct socket *), so)

int soaccept();
#define	SOACCEPT(so, nam) \
	KScall(soaccept, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, nam)

int sobind();
#define	SOBIND(so, nam) \
	KScall(sobind, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, nam)

int soclose();
#define	SOCLOSE(so) \
	KScall(soclose, sizeof(struct socket *), so)

int soconnect();
#define	SOCON1(so, nam) \
	KScall(soconnect, sizeof(struct socket *) + \
	    sizeof(struct mbuf *), so, nam)

int soconnect2();
#define	SOCON2(so1, so2) \
	KScall(soconnect2, sizeof(struct socket *) + \
	    sizeof(struct socket *), so1, so2)

int socreate();
#define	SOCREATE(dom, aso, type, proto) \
	KScall(socreate, sizeof(int) + sizeof(struct socket **) + \
	    sizeof(int) + sizeof(int), dom, aso, type, proto)

int sogetnam();
#define	SOGETNAM(so, m) \
	KScall(sogetnam, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, m)

int sogetopt();
#define	SOGETOPT(so, level, optname, mp) \
	KScall(sogetopt, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(int) + sizeof(struct mbuf **), so, level, optname, mp)

int sogetpeer();
#define	SOGETPEER(so, m) \
	KScall(sogetpeer, sizeof(struct socket *) + sizeof(struct mbuf *), \
	    so, m)

int solisten();
#define	SOLISTEN(so, backlog) \
	KScall(solisten, sizeof(struct socket *) + sizeof(int), so, backlog)

/* note; pass pointer to a socket, not pointer to file structure */
int soo_ioctl();
#define	SOO_IOCTL(fp, cmd, data) \
	KScall(soo_ioctl, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(caddr_t), fp->f_socket, cmd, data)

/* note; pass pointer to a socket, not pointer to file structure */
int soo_select();
#define	SOO_SELECT(fp, which) \
	KScall(soo_select, sizeof(struct socket *) + sizeof(int), \
	    fp->f_socket, which)

int soo_stat();
#define	SOO_STAT(so, ub) \
	KScall(soo_stat, sizeof(struct socket *) + sizeof(struct stat *), \
	    so, ub)

int soreceive();
#define	SORECEIVE(so, aname, uiop, flags, rightsp) \
	KScall(soreceive, sizeof(struct socket *) + sizeof(struct mbuf **) + \
	    sizeof(struct uio *) + sizeof(int) + sizeof(struct mbuf **), \
	    so, aname, uiop, flags, rightsp)

int sosend();
#define	SOSEND(so, nam, uiop, flags, rights) \
	KScall(sosend, sizeof(struct socket *) + sizeof(struct mbuf *) + \
	    sizeof(struct uio *) + sizeof(int) + sizeof(struct mbuf *), \
	    so, nam, uiop, flags, rights)

int sosetopt();
#define	SOSETOPT(so, level, optname, m0) \
	KScall(sosetopt, sizeof(struct socket *) + sizeof(int) + \
	    sizeof(int) + sizeof(struct mbuf *), so, level, optname, m0)

int soshutdown();
#define	SOSHUTDOWN(so, how) \
	KScall(soshutdown, sizeof(struct socket *) + sizeof(int), so, how)

int timeout();
#define	TIMEOUT(fun, arg, t) \
	SKcall(timeout, sizeof(memaddr) + sizeof(caddr_t) + sizeof(int), \
	    SUPERADD(fun), (caddr_t)arg, t)

int ttstart();
#define	TTSTART(tp) \
	SKcall(ttstart, sizeof(struct tty *), tp)

int ttyflush();
#define	TTYFLUSH(tp, rw) \
	SKcall(ttyflush, sizeof(struct tty *) + sizeof(int), tp, rw)

int ttywflush();
#define	TTYWFLUSH(tp) \
	SKcall(ttywflush, sizeof(struct tty *), tp)

int unpbind();
#define	UNPBIND(path, len, ipp, unpsock) \
	SKcall(unpbind, sizeof(char *) + sizeof(int) + \
	    sizeof(struct inode **) + sizeof(struct socket *), \
	    path, len, ipp, unpsock)

int unpconn();
#define	UNPCONN(path, len, so2, ipp) \
	SKcall(unpconn, sizeof(char *) + sizeof(int) + \
	    sizeof(struct socket **) + sizeof(struct inode **), \
	    path, len, so2, ipp)

int unputc();
#define	UNPUTC(p) \
	SKcall(unputc, sizeof(struct clist *), p)

void unpdet();
#define	UNPDET(ip) \
	SKcall(unpdet, sizeof(struct inode *), ip)

int wakeup();
#define	WAKEUP(chan) \
	SKcall(wakeup, sizeof(caddr_t), SUPERADD(chan))
