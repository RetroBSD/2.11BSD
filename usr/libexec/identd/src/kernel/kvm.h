/*
** kvm.h                    Header file for the kernel virtual memory access
**                          routines
**
** This code is in the public domain and may be used freely by anyone
** who wants to.
**
** Last update: 19 Oct 1992
**
** Author: Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef __KVM_H__
#define __KVM_H__

typedef struct
{
  int fd;
  char *namelist;
#ifdef BSD43
  int swap_fd;
  int mem_fd;

  int procidx;
  int nproc;
  struct pte *Usrptma;
  struct pte *usrpt;
  struct proc *proctab;
#endif
} kvm_t;


extern kvm_t *kvm_open();
extern int kvm_close();
extern int kvm_nlist();
extern int kvm_read();

#ifdef BSD43
extern struct user *kvm_getu();
extern struct proc *kvm_getproc();
extern struct proc *kvm_nextproc();
extern int kvm_setproc();
#endif

#endif
