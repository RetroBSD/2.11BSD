# Placed in the public domain - 1995/05/06
#
#	@(#)Makefile	1.9 (2.11BSD) 2000/2/28

CPP= /lib/cpp
LD= /bin/ld
AS= /bin/as

# modules which can not use SYSCALL and must be assembled from sources.  The 
# rest of the system calls are generated with printf(1) and do not have 
# source files associated with them.

PDPSRCS= _exit.s brk.s execl.s execle.s execv.s fork.s pipe.s ptrace.s sbrk.s \
	setlogin.s sigaction.s sigreturn.s vfork.s

PDPOBJS= _exit.o brk.o execl.o execle.o execv.o fork.o pipe.o ptrace.o sbrk.o \
	setlogin.o sigaction.o sigreturn.o vfork.o

# these are generated with a second argument to SYSCALL of 'norm'.  Have to
# split into more than one list because 'make' only permits 80 "lefts".

NORM1= __sysctl.o accept.o access.o adjtime.o bind.o chdir.o chflags.o \
	chmod.o chown.o chroot.o close.o connect.o dup.o dup2.o \
	execve.o fchdir.o fchflags.o fchmod.o fchown.o fcntl.o flock.o fstat.o \
	fsync.o ftruncate.o getdtablesize.o getgroups.o getitimer.o \
	_getlogin.o getsockname.o \
	getpeername.o getpriority.o getrlimit.o getrusage.o getsockopt.o \
	gettimeofday.o ioctl.o kill.o killpg.o link.o listen.o lstat.o mkdir.o

NORM2=  mknod.o mount.o open.o pselect.o quota.o read.o readlink.o readv.o \
	reboot.o \
	recv.o recvfrom.o recvmsg.o rename.o rmdir.o select.o send.o sendmsg.o \
	sendto.o setgroups.o setitimer.o setpgrp.o setpriority.o setquota.o \
	setuid.o seteuid.o setgid.o setegid.o \
	setrlimit.o setsockopt.o settimeofday.o \
	shutdown.o sigaltstack.o socket.o socketpair.o stat.o symlink.o \
	sigprocmask.o sigstack.o sigwait.o \
	statfs.o fstatfs.o getfsstat.o \
	truncate.o umount.o unlink.o utimes.o wait4.o write.o writev.o

# these are generated with a second argument to SYSCALL of 'long'

LONG=	lseek.o

# these are generated with a second argument to SYSCALL of 'error'

ERROR= sigsuspend.o

# these are generated with a second argument to SYSCALL of 'noerror'

NOERROR= getgid.o getegid.o getpgrp.o getpid.o \
	getppid.o getuid.o geteuid.o profil.o sigpending.o sync.o \
	umask.o vhangup.o

OBJS= ${PDPOBJS} ${NORM1} ${NORM2} ${LONG} ${ERROR} ${NOERROR}

TAGSFILE=tags

.s.o:
	/lib/cpp -DPROF ${DEFS} $< | sed -e 's;^#;/;' | as -V -o $@
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	/lib/cpp ${DEFS} $< | sed -e 's;^#;/;' | as -V -o $@
	-ld -X -r $*.o
	mv a.out $*.o

syslib syslib_p: ${OBJS}
	@echo "building profiled syslib"
	@cd profiled; ar cru ../syslib_p ${OBJS}
	@echo "building normal syslib"
	@ar cru syslib ${OBJS}

${NORM1}: SYS.h /usr/include/syscall.h
	@echo creating $*.o profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,norm)\n' | \
	    ${CPP} -DPROF ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -X -r $*.o
	@mv a.out profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,norm)\n' | \
	    ${CPP} ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -x -r $*.o
	@mv a.out $*.o

${NORM2}: SYS.h /usr/include/syscall.h
	@echo creating $*.o profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,norm)\n' | \
	    ${CPP} -DPROF ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -X -r $*.o
	@mv a.out profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,norm)\n' | \
	    ${CPP} ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -x -r $*.o
	@mv a.out $*.o

${LONG}: SYS.h /usr/include/syscall.h
	@echo creating $*.o profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,long)\n' | \
	    ${CPP} -DPROF ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -X -r $*.o
	@mv a.out profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,long)\n' | \
	    ${CPP} ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -x -r $*.o
	@mv a.out $*.o

${ERROR}: SYS.h /usr/include/syscall.h
	@echo creating $*.o profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,error)\n' | \
	    ${CPP} -DPROF ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -X -r $*.o
	@mv a.out profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,error)\n' | \
	    ${CPP} ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -x -r $*.o
	@mv a.out $*.o

${NOERROR}: SYS.h /usr/include/syscall.h
	@echo creating $*.o profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,noerror)\n' | \
	    ${CPP} -DPROF ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -X -r $*.o
	@mv a.out profiled/$*.o
	@printf '#include "SYS.h"\nSYSCALL($*,noerror)\n' | \
	    ${CPP} ${DEFS} | sed -e 's;^#;/;' | ${AS} -o $*.o
	@${LD} -x -r $*.o
	@mv a.out $*.o

clean:
	rm -f *.o profiled/*.o a.out syslib syslib_p
