# @(#) Makefile 1.20 1996/11/16
#  Tailored and trimmed version created for 2.11BSD on April 7, 1995.

REAL_DAEMON_DIR=/usr/libexec
RANLIB	= ranlib	# have ranlib (BSD-ish UNIX)
ARFLAGS	= rv		# most systems
AUX_OBJ= environ.o
FACILITY= LOG_DAEMON	# LOG_MAIL is what most sendmail daemons use
SEVERITY= LOG_INFO	# LOG_INFO is normally not logged to the console

# If you use /etc/hosts.{dir,pag} then leave this commented out.  However if
#   you are using the resolver routines (i.e. you have /etc/resolv.conf) then
#   uncomment the line below.
#DOT=	-DAPPEND_DOT
#AUTH	= -DALWAYS_RFC931
RFC931_TIMEOUT = 10
UMASK	= -DDAEMON_UMASK=022
ACCESS	= -DHOSTS_ACCESS
TABLES	= -DHOSTS_DENY=\"/etc/hosts.deny\" -DHOSTS_ALLOW=\"/etc/hosts.allow\"
PARANOID= -DPARANOID
HOSTNAME= -DALWAYS_HOSTNAME
KILL_OPT= -DKILL_IP_OPTIONS
SHELL	= /bin/sh

LIB	= libwrap.a
MANROFF	= /usr/man/manroff

.SUFFIXES: .8 .5 .0

.8.0:
	${MANROFF} $*.8 > $@

.5.0:
	${MANROFF} $*.5 > $@

CFLAGS	= -O -i -DFACILITY=$(FACILITY) $(ACCESS) $(PARANOID) $(AUTH) $(UMASK) \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" $(KILL_OPT) \
	-DSEVERITY=$(SEVERITY) -DRFC931_TIMEOUT=$(RFC931_TIMEOUT) \
	$(TABLES) $(STRINGS) $(DOT) \
	$(HOSTNAME)

LIB_OBJ= hosts_access.o options.o shell_cmd.o rfc931.o eval.o \
	hosts_ctl.o refuse.o percent_x.o clean_exit.o $(AUX_OBJ) \
	fix_options.o socket.o tli.o workarounds.o \
	update.o misc.o diag.o percent_m.o

all: config-check tcpd tcpdmatch try-from safe_finger tcpdchk tcpd.0 \
	hosts5access.0 hosts3access.0 hosts_options.0 tcpdchk.0 tcpdmatch.0

install: all
	install -s -m 1751 -o root -g staff tcpd /usr/libexec/tcpd
	install -s -m 751 -o root -g staff tcpdmatch /usr/ucb/tcpdmatch
	install -s -m 751 -o root -g staff try-from /usr/ucb/try-from
	install -s -m 751 -o root -g staff safe_finger /usr/ucb/safe_finger
	install -s -m 751 -o root -g staff tcpdchk /usr/ucb/tcpdchk
	install -c -m 444 -o bin -g bin hosts3access.0 /usr/man/cat3/hosts_access.0
	install -c -m 444 -o bin -g bin hosts5access.0 /usr/man/cat5/hosts_access.0
	install -c -m 444 -o bin -g bin hosts_options.0 /usr/man/cat5/hosts_options.0
	install -c -m 444 -o bin -g bin tcpd.0 /usr/man/cat8/tcpd.0
	install -c -m 444 -o bin -g bin tcpdchk.0 /usr/man/cat8/tcpdchk.0
	install -c -m 444 -o bin -g bin tcpdmatch.0 /usr/man/cat8/tcpdmatch.0

clean:
	rm -f tcpd miscd safe_finger tcpdmatch tcpdchk try-from *.[oa] *.core \
	cflags *.0

# These are special because we can't have two different hosts_access.0 files
# at the same time.  Grrr - stupid to name the things the same in the same
# directory.

hosts3access.0: hosts_access.3
	$(MANROFF) hosts_access.3 > hosts3access.0

hosts5access.0: hosts_access.5
	$(MANROFF) hosts_access.5 > hosts5access.0

# Invalidate all object files when the compiler options (CFLAGS) have changed.

config-check:
	@set +e; test -n "$(REAL_DAEMON_DIR)" || { make; exit 1; }
	-@set +e; echo $(CFLAGS) >/tmp/cflags.$$$$ ; \
	if cmp cflags /tmp/cflags.$$$$ ; \
	then rm /tmp/cflags.$$$$ ; \
	else mv /tmp/cflags.$$$$ cflags ; \
	fi >/dev/null 2>/dev/null

$(LIB):	$(LIB_OBJ)
	rm -f $(LIB)
	ar $(ARFLAGS) $(LIB) $(LIB_OBJ)
	-$(RANLIB) $(LIB)

tcpd:	tcpd.o $(LIB)
	$(CC) $(CFLAGS) -o $@ tcpd.o $(LIB) $(LIBS)

miscd:	miscd.o $(LIB)
	$(CC) $(CFLAGS) -o $@ miscd.o $(LIB) $(LIBS)

safe_finger: safe_finger.o $(LIB)
	$(CC) $(CFLAGS) -o $@ safe_finger.o $(LIB) $(LIBS)

TCPDMATCH_OBJ = tcpdmatch.o fakelog.o inetcf.o scaffold.o

tcpdmatch: $(TCPDMATCH_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(TCPDMATCH_OBJ) $(LIB) $(LIBS)

try-from: try-from.o fakelog.o $(LIB)
	$(CC) $(CFLAGS) -o $@ try-from.o fakelog.o $(LIB) $(LIBS)

TCPDCHK_OBJ = tcpdchk.o fakelog.o inetcf.o scaffold.o

tcpdchk: $(TCPDCHK_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(TCPDCHK_OBJ) $(LIB) $(LIBS)

# Enable all bells and whistles for linting.

lint: tcpd_lint miscd_lint match_lint chk_lint

tcpd_lint:
	lint -ha -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" \
	tcpd.c fromhost.c socket.c tli.c hosts_access.c \
	shell_cmd.c refuse.c rfc931.c eval.c percent_x.c clean_exit.c \
	options.c setenv.c fix_options.c workarounds.c update.c misc.c \
	diag.c percent_m.c

miscd_lint:
	lint -ha -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID -DNETGROUP \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" \
	miscd.c fromhost.c socket.c tli.c hosts_access.c \
	shell_cmd.c refuse.c rfc931.c eval.c percent_x.c clean_exit.c \
	options.c setenv.c fix_options.c workarounds.c update.c misc.c \
	diag.c percent_m.c

match_lint:
	lint -ha -DFACILITY=LOG_MAIL -DSEVERITY=$(SEVERITY) -DHOSTS_ACCESS \
	-DPARANOID $(TABLES) -DNETGROUP -DPROCESS_OPTIONS -DRFC931_TIMEOUT=10 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" \
	tcpdmatch.c hosts_access.c eval.c percent_x.c options.c workarounds.c \
	update.c socket.c misc.c diag.c percent_m.c setenv.c \
	inetcf.c scaffold.c

chk_lint:
	lint -ha -DFACILITY=LOG_MAIL -DSEVERITY=$(SEVERITY) -DHOSTS_ACCESS \
	-DPARANOID $(TABLES) -DNETGROUP -DPROCESS_OPTIONS -DRFC931_TIMEOUT=10 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" \
	tcpdchk.c eval.c percent_x.c options.c update.c workarounds.c \
	setenv.c misc.c diag.c percent_m.c inetcf.c scaffold.c

printfck:
	printfck -f printf.ck \
	tcpd.c fromhost.c socket.c tli.c hosts_access.c \
	shell_cmd.c refuse.c rfc931.c eval.c percent_x.c clean_exit.c \
	options.c setenv.c fix_options.c workarounds.c update.c misc.c \
	diag.c percent_m.c >aap.c
	lint -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID -DNETGROUP \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" aap.c
	printfck -f printf.ck \
	tcpdchk.c eval.c percent_x.c options.c update.c workarounds.c \
	setenv.c misc.c diag.c percent_m.c inetcf.c scaffold.c \
	>aap.c
	lint -DFACILITY=LOG_MAIL -DSEVERITY=$(SEVERITY) -DHOSTS_ACCESS \
	-DPARANOID $(TABLES) -DNETGROUP -DPROCESS_OPTIONS -DRFC931_TIMEOUT=10 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\"

# Internal compilation dependencies.

clean_exit.o: cflags
clean_exit.o: tcpd.h
diag.o: cflags
diag.o: mystdarg.h
diag.o: tcpd.h
environ.o: cflags
eval.o: cflags
eval.o: tcpd.h
fakelog.o: cflags
fakelog.o: mystdarg.h
fix_options.o: cflags
fix_options.o: tcpd.h
fromhost.o: cflags
fromhost.o: tcpd.h
hosts_access.o: cflags
hosts_access.o: tcpd.h
hosts_ctl.o: cflags
hosts_ctl.o: tcpd.h
inetcf.o: cflags
inetcf.o: inetcf.h
inetcf.o: tcpd.h
misc.o: cflags
misc.o: tcpd.h
miscd.o: cflags
miscd.o: patchlevel.h
miscd.o: tcpd.h
ncr.o: cflags
ncr.o: tcpd.h
options.o: cflags
options.o: tcpd.h
percent_m.o: cflags
percent_m.o: mystdarg.h
percent_x.o: cflags
percent_x.o: tcpd.h
ptx.o: cflags
ptx.o: tcpd.h
refuse.o: cflags
refuse.o: tcpd.h
rfc931.o: cflags
rfc931.o: tcpd.h
safe_finger.o: cflags
scaffold.o: cflags
scaffold.o: scaffold.h
scaffold.o: tcpd.h
setenv.o: cflags
shell_cmd.o: cflags
shell_cmd.o: tcpd.h
socket.o: cflags
socket.o: tcpd.h
strcasecmp.o: cflags
tcpd.o: cflags
tcpd.o: patchlevel.h
tcpd.o: tcpd.h
tcpdchk.o: cflags
tcpdchk.o: inetcf.h
tcpdchk.o: scaffold.h
tcpdchk.o: tcpd.h
tcpdmatch.o: cflags
tcpdmatch.o: scaffold.h
tcpdmatch.o: tcpd.h
tli-sequent.o: cflags
tli-sequent.o: tcpd.h
tli-sequent.o: tli-sequent.h
tli.o: cflags
tli.o: tcpd.h
try-from.o: cflags
try-from.o: tcpd.h
update.o: cflags
update.o: mystdarg.h
update.o: tcpd.h
vfprintf.o: cflags
workarounds.o: cflags
workarounds.o: tcpd.h
