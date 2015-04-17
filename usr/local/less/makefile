# Revised 1996/1/28
#
# Makefile for "less"
#
# Invoked as:
#	make all
#   or	make install
# Plain "make" is equivalent to "make all".
#
# If you add or delete functions, remake funcs.h by doing:
#	make newfuncs
# This depends on the coding convention of function headers looking like:
#	" \t public <function-type> \n <function-name> ( ... ) "
#
# Also provided:
#	make lint	# Runs "lint" on all the sources.
#	make clean	# Removes "less" and the .o files.


##########################################################################
# System-specific parameters
##########################################################################

# Define XENIX if running under XENIX 3.0
XENIX = 0

# VOID is 1 if your C compiler supports the "void" type,
# 0 if it does not.
VOID = 1

# off_t is the type which lseek() returns.
# It is also the type of lseek()'s second argument.
off_t = long

# TERMIO is 1 if your system has /usr/include/termio.h.
# This is normally the case for System 5.
# If TERMIO is 0 your system must have /usr/include/sgtty.h.
# This is normally the case for BSD.
TERMIO = 0

# SIGSETMASK is 1 if your system has the sigsetmask() call.
# This is normally the case only for BSD 4.2,
# not for BSD 4.1 or System 5.
SIGSETMASK = 1

##########################################################################
# Optional and semi-optional features
##########################################################################

# REGCMP is 1 if your system has the regcmp() function.
# This is normally the case for System 5.
# RECOMP is 1 if your system has the re_comp() function.
# This is normally the case for BSD.
# If neither is 1, pattern matching is supported, but without metacharacters.
REGCMP = 0
RECOMP = 1

# SHELL_ESCAPE is 1 if you wish to allow shell escapes.
# (This is possible only if your system supplies the system() function.)
SHELL_ESCAPE = 1

# EDITOR is 1 if you wish to allow editor invocation (the "v" command).
# (This is possible only if your system supplies the system() function.)
# EDIT_PGM is the name of the (default) editor to be invoked.
EDITOR = 1
EDIT_PGM = vi

# GLOB is 1 if you wish to have shell metacharacters expanded in filenames.
# This will generally work if your system provides the "popen" function
# and the "echo" shell command.
GLOB = 1

# LOGFILE is 1 if you wish to allow the -l option (to create log files).
LOGFILE = 1

# ONLY_RETURN is 1 if you want RETURN to be the only input which
# will continue past an error message.
# Otherwise, any key will continue past an error message.
ONLY_RETURN = 0


##########################################################################
# Compilation environment.
##########################################################################

# LIBS is the list of libraries needed.
LIBS = -ltermcap

# INSTALL_LESS is a list of the public versions of less.
# INSTALL_HELP is a list of the public version of the help file.
# INSTALL_MAN is a list of the public versions of the manual page.
INSTALL_LESS =	/usr/local/less
INSTALL_HELP =	/usr/local/lib/less.help
INSTALL_MAN =	/usr/local/man/cat1/less.0
MANUAL =	less.nro
HELPFILE =	/usr/local/lib/less.help


# OPTIM is passed to the compiler and the loader.
# It is normally "-O" but may be, for example, "-g".
OPTIM = -O -i


##########################################################################
# Files
##########################################################################

SRC1 =	main.c option.c prim.c ch.c position.c input.c output.c 
SRC2 =	screen.c prompt.c line.c signal.c help.c ttyin.c command.c version.c
SRC =	$(SRC1) $(SRC2)
OBJ =	main.o option.o prim.o ch.o position.o input.o output.o screen.o \
	prompt.o line.o signal.o help.o ttyin.o command.o version.o


##########################################################################
# Rules
##########################################################################

DEFS =	"-DTERMIO=$(TERMIO)" \
	"-DSIGSETMASK=$(SIGSETMASK)" \
	"-Doff_t=$(off_t)" "-DVOID=$(VOID)" \
	"-DREGCMP=$(REGCMP)" "-DRECOMP=$(RECOMP)" \
	"-DSHELL_ESCAPE=$(SHELL_ESCAPE)" \
	"-DEDITOR=$(EDITOR)" "-DEDIT_PGM=\"$(EDIT_PGM)\"" \
	"-DHELPFILE=\"$(HELPFILE)\"" \
	"-DLOGFILE=$(LOGFILE)" \
	"-DONLY_RETURN=$(ONLY_RETURN)" \
	"-DGLOB=$(GLOB)" \
	"-DXENIX=$(XENIX)"

CFLAGS = $(OPTIM) $(DEFS)


all: less less.0

less: $(OBJ)
	cc $(OPTIM) -o less $(OBJ) $(LIBS)

less.0:	less.nro
	/usr/man/manroff $(MANUAL) > less.0

install: less less.0 less.help
	install -s -m 751 less ${INSTALL_LESS}
	install -c -m 444 less.help ${INSTALL_HELP}
	install -c -m 444 -o bin -g bin less.0 $(INSTALL_MAN)

$(OBJ): less.h funcs.h

# help.o depends on makefile for the definition of HELPFILE.
help.o: makefile

lint:
	lint -hp $(DEFS) $(SRC)

newfuncs:
	mv funcs.h funcs.h.OLD
	awk -f mkfuncs.awk $(SRC) >funcs.h

clean:
	rm -f $(OBJ) less less.0

shar:
	shar -v README install less.man less.help makefile.* *.h *.awk > less.shar.a
	shar -v less.nro $(SRC1) > less.shar.b
	shar -v $(SRC2) > less.shar.c
