/*
** kernel/sco324.c             Kernel access functions to retrieve user number
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 17 March 1993
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
**
** fast COFF nlist() code written by Peter Wemm <peter@DIALix.oz.au>
** This is up to 100 times faster than 3.2v1.0 to 3.2v4.1's nlist().
** This is slightly faster than the 3.2v4.2 nlist().
**
** Preliminary SCO support by Peter Wemm <peter@DIALix.oz.au>
** Known Limitations:
**   1: Can only get *effective* UID of a socket.  This is a real
**      serious problem, as it looks like all rlogins, and rcp'c
**      come from root.  Any volunteers to emulate the fuser command
**      and grope around the kernel user structs for file pointers
**      to get the *real* uid?
**   2: THIS WILL NOT (YET!) WORK WITH SCO TCP 1.2.1 and hence ODT 3.0
*/

#include <stdio.h>

#include <filehdr.h>
#include <syms.h>

/* Name space collision */
#undef n_name

#include <nlist.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

/* how much buffer space to allocate to the symbol scanning */
/* Make this at least 4096, but much more than 8192 is a waste */
#define CHUNKSIZE 8192

int nlist(const char *filename, struct nlist *nl)
{
  FILHDR fh;			/* COFF file header */
  SYMENT *se;			/* pointer to a SYMENT */

  int i, n;			/* iterative variables */

  long strsect;			/* seek pos of extended string table */
  long strsize;			/* byte size of extended string table*/
  
  int mchunk;			/* max number of syments per chunk */
  int p;			/* Symbol entry cache */
  int slotnum;			/* Symbol entry cache */
  int chunknum;			/* Symbol entry cache */
  int oldchunknum;		/* Symbol entry cache */

  char *strtab = NULL;		/* malloc'ed extended string table buffer */
  char *symchunk = NULL;	/* malloc'ed SYMENT symbol entry cache */

  int fd = -1;			/* File descriptor we are dealing with */

  int nument = 0;		/* How many symbols in the array */
  int numremaining;		/* Counter for symbols not found yet */
  struct nlist *nptr;		/* Pointer to current struct nlist entry */

  /* a check from sanity claus */
  if (filename == NULL || nl == NULL)
    goto cleanup;

  /* count the entries in the request table */
  nptr = nl;
  while (nptr->n_name && strlen(nptr->n_name) > 0) {
    /* clear out the values as per the man-page */
    nptr->n_value  = 0;
    nptr->n_scnum  = 0;
    nptr->n_type   = 0;
    nptr->n_sclass = 0;
    nptr->n_numaux = 0;
    nptr++;
    nument++;
  }

  /* early exit if nothing wanted.. return success */
  if (nument == 0)
    return 0;

  /* no point scanning whole list if we've found'em all */
  numremaining = nument;

  /* open the COFF file */
  fd = open(filename, O_RDONLY, 0);

  if (fd < 0)
    goto cleanup;

  /* read the COFF file header */
  if (read(fd, &fh, FILHSZ) < FILHSZ)
    goto cleanup;

  /* calcualte the starting offset of the string table */
  strsect = fh.f_symptr + (fh.f_nsyms * SYMESZ);

  /* read the length of the string table */
  if (lseek(fd, strsect, SEEK_SET) < 0)
    goto cleanup;
  if (read(fd, &strsize, sizeof(strsize)) < sizeof(strsize))
    goto cleanup;

  /* allocate a buffer for the string table */
  strtab = malloc(strsize);
  if (strtab == NULL)
    goto cleanup;

  /* allocate a buffer for the string table */
  mchunk = CHUNKSIZE / SYMESZ;
  symchunk = malloc(mchunk * SYMESZ);
  if (symchunk == NULL)
    goto cleanup;

  /* read the string table */
  if (lseek(fd, strsect, SEEK_SET) < 0)
    goto cleanup;
  if (read(fd, strtab, strsize) < strsize)
    goto cleanup;

  /* step through the symbol table */
  if (lseek(fd, fh.f_symptr, SEEK_SET) < 0)
    goto cleanup;

  oldchunknum = -1;
  p = 0;			/* symbol slot number */
  for (i = 0; i < fh.f_nsyms; i++) {

    /* which "chunk" of the symbol table we want */
    chunknum = p / mchunk;

    /* Where in the chunk is the SYMENT we are up to? */
    slotnum  = p % mchunk;

    /* load the chunk buffer if needed */
    if (chunknum != oldchunknum) {
      if (read(fd, symchunk, mchunk * SYMESZ) <= 0)
	goto cleanup;
      oldchunknum = chunknum;
    }

    /* and of course.. Get a pointer.. */
    se = (SYMENT *) (symchunk + slotnum * SYMESZ);

    /* next entry */
    p++;

    /* is it a long string? */
    if (se->n_zeroes == 0) {

      /* check table */
      for (n = 0; n < nument; n++) {
	if (strcmp(strtab + se->n_offset, nl[n].n_name) == 0) {
	  /* load the requested table */
	  nl[n].n_value  = se->n_value;
	  nl[n].n_scnum  = se->n_scnum;
	  nl[n].n_type   = se->n_type;
	  nl[n].n_sclass = se->n_sclass;
	  nl[n].n_numaux = se->n_numaux;
	  numremaining--;
	  break;
	}
      }
      if (numremaining == 0)
	break;			/* drop the for loop */

    } else {

      /* check table */
      for (n = 0; n < nument; n++) {
	
	if (strncmp(se->_n._n_name, nl[n].n_name, sizeof(se->_n._n_name)) == 0) {
	  /* since we stopped at the 8th char, make sure that's all we want */
	  if ((int)strlen(nl[n].n_name) <= 8) {
	    /* load the requested table */
	    nl[n].n_value  = se->n_value;
	    nl[n].n_scnum  = se->n_scnum;
	    nl[n].n_type   = se->n_type;
	    nl[n].n_sclass = se->n_sclass;
	    nl[n].n_numaux = se->n_numaux;
	    numremaining--;
	    break;
	  }
	}
      }
      if (numremaining == 0)
	break;			/* drop the for loop */

    }
    
    /* does it have auxillary entries? */
    p += se->n_numaux;

  }
  
  free(strtab);
  free(symchunk);
  close(fd);

  return numremaining;

cleanup:
  if (fd >= 0)
    close(fd);
  if (strtab)
    free(strtab);
  if (symchunk)
    free(symchunk);
  return -1;

}

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include "kvm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>

#define _KERNEL

#include <sys/file.h>
#include <sys/dir.h>

#include <sys/inode.h>

#include <fcntl.h>

#include <sys/user.h>
#include <sys/wait.h>

#include <sys/var.h>
  
#undef _KERNEL

#include <sys/socket.h>
#include <sys/stream.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>

#include <netinet/ip_var.h>

#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>

#include <arpa/inet.h>

#include <sys/net/protosw.h>
#include <sys/net/socketvar.h>

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();

struct nlist nl[] =
{
#define N_V    0
#define N_TCB  1
 
  { "v" },
  { "tcb" },
  { "" }
};

static kvm_t *kd;

static struct var v;

static struct inpcb tcb;


int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if (!(kd = kvm_open(path_unix, path_kmem, NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");
  
  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) != 0)
    ERROR("main: kvm_nlist");

  return 0;
}


/*
** Get a piece of kernel memory with error handling.
** Returns 1 if call succeeded, else 0 (zero).
*/
static int getbuf(addr, buf, len, what)
  long addr;
  char *buf;
  int len;
  char *what;
{
  if (kvm_read(kd, addr, buf, len) < 0)
  {
    if (syslog_flag)
      syslog(LOG_ERR, "getbuf: kvm_read(%08x, %d) - %s : %m",
	     addr, len, what);

    return 0;
  }
  
  return 1;
}



/*
** Traverse the inpcb list until a match is found.
** Returns NULL if no match.
*/
static struct inpcb *
    getlist(pcbp, faddr, fport, laddr, lport)
  struct inpcb *pcbp;
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
  struct inpcb *head;

  if (!pcbp)
    return NULL;

  head = pcbp->inp_prev;
  do 
  {
    if ( pcbp->inp_faddr.s_addr == faddr->s_addr &&
	 pcbp->inp_laddr.s_addr == laddr->s_addr &&
	 pcbp->inp_fport        == fport &&
	 pcbp->inp_lport        == lport )
    {
      return pcbp;
    }

  } while (pcbp->inp_next != head &&
	   getbuf((long) pcbp->inp_next,
		  pcbp,
		  sizeof(struct inpcb),
		  "tcblist"));

  return NULL;
}

static caddr_t
    followqueue(pcbp)
  struct inpcb *pcbp;
{
  queue_t *q;
  queue_t qbuf;
  int n = 1;

  if (!pcbp)
    return NULL;

  q = pcbp->inp_q;

  while (getbuf((long) q, &qbuf, sizeof(qbuf), "queue_t inp_q"))
  {
    q = qbuf.q_next;
    n++;
    if (qbuf.q_next == NULL)
      return qbuf.q_ptr;
  }

  return NULL;
}


/*
** Return the user number for the connection owner
*/
int k_getuid(faddr, fport, laddr, lport, uid)
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
{
  int i;
  struct inode inode;
  struct stat s;
  struct inpcb *sockp;
  caddr_t cad;
  struct socket socket;
  struct file file;
  short sockmajor;
  short sockminor;


  /* -------------------- TCP PCB LIST -------------------- */
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;
  
  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;

  sockp = getlist(&tcb, faddr, fport, laddr, lport);
  
  if (!sockp)
    return -1;

  if (sockp->inp_protoopt & SO_IMASOCKET)
  {

    cad = followqueue(sockp); /* socket pointer */

    if (!getbuf((long)cad, &socket, sizeof(struct socket), "socket"))
      return -1;

    if (!getbuf((long)socket.so_fp, &file, sizeof(struct file), "file"))
      return -1;

    if (!getbuf((long)file.f_inode, &inode, sizeof(struct inode), "inode"))
      return -1;

    *uid = inode.i_uid;
    return 0;
  } else {

    /* we just need to determine the major number of the tcp module, which
       is the minor number of the tcp clone device /dev/tcp */
    if (stat("/dev/inet/tcp", &s))
      ERROR("stat(\"/dev/inet/tcp\")");
  
    sockmajor = major(s.st_rdev);
    sockminor = sockp->inp_minor;

    if (!getbuf(nl[N_V].n_value, &v, sizeof(struct var), "var"))
      return -1;

    for (i = 0; i < v.v_inode; i++) {

      if (!getbuf(((long) v.ve_inode) + i * sizeof(struct inode),
	  &inode, sizeof(struct inode), "inode"))
	return -1;
    
      if (((inode.i_ftype & IFMT) == IFCHR) &&
	  (major(inode.i_rdev) == sockmajor) &&
	  (minor(inode.i_rdev) == sockminor))
      {
	*uid = inode.i_uid;
	return 0;
      }
    }
    return -1;
  }
}


