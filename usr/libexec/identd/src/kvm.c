/*
** kvm.c            A set of functions emulating KVM for machines without them.
**
** This code is in the public domain and may be used freely by anyone
** who wants to.
**
** Last update: 12 Dec 1992
**
** Author: Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef NO_KVM
#ifndef HAVE_KVM

#ifdef NeXT31
#  include <libc.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <nlist.h>

#ifdef IRIX
#  ifdef IRIX62
#    if _MIPS_SZPTR == 64
#       define _K64U64 1
#    else
#       define _K32U32 1
#    endif
#  endif

#  include <sys/sbd.h>

#  ifndef K0_TO_PHYS
#    define K0_TO_PHYS(x)   (x) 
#  endif

#  if defined(IRIX6) || (defined(IRIX62) && _MIPS_SZPTR == 64)
#    define nlist nlist64
#  endif
#endif

#ifdef BSD43
#  include <sys/types.h>
#  include <sys/dir.h>
#  include <sys/param.h>
#  include <sys/vmmac.h>
#if 0
#  include <sys/time.h>
#else
#  include <sys/user.h>
#endif
#  ifdef DIRBLKSIZ
#    undef DIRBLKSIZ
#  endif
#if 0
#  include <sys/user.h>
#endif
#  include <sys/proc.h>
#  include <machine/pte.h>
#  include "paths.h"
#endif
 

#include "kernel/kvm.h"
#include "paths.h"

#if defined(BSD43) || defined(MIPS)
extern int errno;
#endif

extern void *malloc();


kvm_t *kvm_open(namelist, corefile, swapfile, flag, errstr)
  char *namelist;
  char *corefile;
  char *swapfile;
  int flag;
  char *errstr;
{
  kvm_t *kd;

  if (!namelist)
    namelist = _PATH_UNIX;
  if (!corefile)
    corefile = _PATH_KMEM;
  
#ifdef BSD43
  if (!swapfile)
    swapfile = _PATH_SWAP;
#endif
  
  kd = (kvm_t *) malloc(sizeof(kvm_t));
  if (!kd)
  {
    if (errstr)
      perror(errstr);
    return NULL;
  }

  kd->namelist = (char *) malloc(strlen(namelist)+1);
  if (!kd->namelist)
  {
    if (errstr)
      perror(errstr);
    return NULL;
  }
  
  if ((kd->fd = open(corefile, flag)) < 0)
  {
    if (errstr)
      perror(errstr);
    free(kd->namelist);
    free(kd);
    return NULL;
  }

#ifdef BSD43
  if ((kd->swap_fd = open(swapfile, flag)) < 0)
  {
    if (errstr)
      perror(errstr);
    close(kd->fd);
    free(kd->namelist);
    free(kd);
    return NULL;
  }

  if ((kd->mem_fd = open(_PATH_MEM, flag)) < 0)
  {
    if (errstr)
      perror(errstr);
    close(kd->swap_fd);
    close(kd->fd);
    free(kd->namelist);
    free(kd);
    return NULL;
  }
#endif
  
  strcpy(kd->namelist, namelist);
  return kd;
}


int kvm_close(kd)
  kvm_t *kd;
{
  int code;
  
  code = close(kd->fd);
#ifdef BSD43
  close(kd->swap_fd);
  close(kd->mem_fd);
  if (kd->proctab)
    free(kd->proctab);
#endif
  free(kd->namelist);
  free(kd);

  return code;
}


/*
** Extract offsets to the symbols in the 'nl' list. Returns 0 if all found,
** or else the number of variables that was not found.
*/
int kvm_nlist(kd, nl)
  kvm_t *kd;
  struct nlist *nl;
{
  int code;
  int i;

  code = nlist(kd->namelist, nl);

  if (code != 0)
    return code;
  
  /*
  ** Verify that we got all the needed variables. Needed because some
  ** implementations of nlist() returns 0 although it didn't find all
  ** variables.
  */
  if (code == 0)
  {
#if defined(__convex__) || defined(NeXT)
    for (i = 0; nl[i].n_un.n_name && nl[i].n_un.n_name[0]; i++)
#else  
    for (i = 0; nl[i].n_name && nl[i].n_name[0]; i++)
#endif
#if defined(_AUX_SOURCE) || defined(_CRAY) || defined(sco) || defined(_SEQUENT_)
      /* A/UX sets n_type to 0 if not compiled with -g. n_value will still
      ** contain the (correct?) value (unless symbol unknown).
      */
      if( nl[i].n_value == 0)
	code++;
#else
      if (nl[i].n_type == 0)
	code++;
#endif
  }
  
  return code;
}


/*
** Get a piece of the kernel memory
*/
static int readbuf(fd, addr, buf, len)
  int fd;
  long addr;
  char *buf;
  int len;
{
#ifdef IRIX
  addr = K0_TO_PHYS(addr);
#endif
  errno = 0;
#if defined(__alpha)
  /*
   * Let us be paranoid about return values.
   * It should be like this on all implementations,
   * but some may have broken lseek or read returns.
   */
  if (lseek(fd, addr, 0) != addr || errno != 0) return -1;
  if (read(fd, buf, len) != len  || errno != 0) return -1;
  return len;
#else
  if (lseek(fd, addr, 0) == -1 && errno != 0) return -1;
  return read(fd, buf, len);
#endif
}

int kvm_read(kd, addr, buf, len)
  kvm_t *kd;
  long addr;
  char *buf;
  int len;
{
  return readbuf(kd->fd, addr, buf, len);
}


#ifdef BSD43

struct user *kvm_getu(kd, procp)
  kvm_t *kd;
  struct proc *procp;
{
  static union
  {
    struct user user;
    char upages[UPAGES][NBPG];
  } userb;

  int ncl;
  struct pte *pteaddr, apte;
  struct pte arguutl[UPAGES+CLSIZE];
  
  
  if ((procp->p_flag & SLOAD) == 0)
  {
    if(readbuf(kd->swap_fd,
	       dtob(procp->p_swaddr),
	       &userb.user, sizeof(struct user)) < 0)
      return NULL;
  }
  else
  {
    /*
    ** Sigh. I just *love* hard coded variable names in macros...
    */
    {
      struct pte *usrpt = kd->usrpt;
      
      pteaddr = &kd->Usrptma[btokmx(procp->p_p0br) + procp->p_szpt - 1];
      if (readbuf(kd->fd, pteaddr, &apte, sizeof(apte)) < 0)
	return NULL;
    }
    
    if (readbuf(kd->mem_fd,
		ctob(apte.pg_pfnum+1)-(UPAGES+CLSIZE)*sizeof(struct pte),
		arguutl, sizeof(arguutl)) < 0)
      return NULL;

    ncl = (sizeof(struct user) + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);
    while (--ncl >= 0)
    {
      int i;

      
      i = ncl * CLSIZE;
      if(readbuf(kd->mem_fd,
		 ctob(arguutl[CLSIZE+i].pg_pfnum),
		 userb.upages[i], CLSIZE*NBPG) < 0)
	return NULL;
    }
  }

  return &userb.user;
}



struct proc *kvm_nextproc(kd)
  kvm_t *kd;
{
  if (kd->proctab == NULL)
    if (kvm_setproc(kd) < 0)
      return NULL;

  if (kd->procidx < kd->nproc)
    return &kd->proctab[kd->procidx++];

  return (struct proc *) NULL;
}

int kvm_setproc(kd)
  kvm_t *kd;
{
  long procaddr;
  
  static struct nlist nl[] =
  {
#define N_PROC 0
#define N_USRPTMA 1
#define N_NPROC 2
#define N_USRPT 3
 
    { "_proc" },
    { "_Usrptmap" },
    { "_nproc" },
    { "_usrpt" },
    { "" }
  };

  if (kvm_nlist(kd, nl) != 0)
    return -1;
  
  kd->Usrptma = (struct pte *) nl[N_USRPTMA].n_value;
  kd->usrpt   = (struct pte *) nl[N_USRPT].n_value;
  
  if (readbuf(kd->fd, nl[N_NPROC].n_value, &kd->nproc, sizeof(kd->nproc)) < 0)
    return -1;

  if (readbuf(kd->fd, nl[N_PROC].n_value, &procaddr, sizeof(procaddr)) < 0)
    return -1;
  
  if (kd->proctab)
    free(kd->proctab);

  kd->proctab = (struct proc *) calloc(kd->nproc, sizeof(struct proc));
  if (!kd->proctab)
    return -1;

  if (readbuf(kd->fd,
	      procaddr,
	      kd->proctab,
	      kd->nproc*sizeof(struct proc)) < 0)
    return -1;

  kd->procidx = 0;

  return 0;
}


struct proc *kvm_getproc(kd, pid)
  kvm_t *kd;
  int pid;
{
  struct proc *procp;


  if (kvm_setproc(kd) < 0)
    return NULL;

  while ((procp = kvm_nextproc(kd)) && procp->p_pid != pid)
    ;
  
  return procp;
}

#endif

#else
/* Just to make some compilers shut up! */
int kvm_dummy()
{
    return 1;
}
#endif
#endif
