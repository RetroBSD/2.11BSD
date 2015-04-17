
#
# Makefile for NNTP intstallation
#

DISTFILES = Makefile README CHANGES rrnpatches

DISTDIRS = common doc inews server support xfer xmit

DIST = $(DISTFILES) $(DISTDIRS)

# Make this either "server" or "client" depending on whether
# you want this package compiled to be a server or a client
# (You can always say "make server" or "make client" if you want.

TYPE = 

all: make_$(TYPE)

make_:
	@ echo "If you want to make a server version or a client version,"
	@ echo "type \"make server\" or \"make client\" respectively."
	@ echo "See README for details."

make_server: /nowhere
	cd server; make
	cd support; make
	cd doc; make
	cd xmit; make

server: make_server

make_client: /nowhere
	cd inews; make
	cd doc; make

client: make_client

install: install_$(TYPE)

install_:
	@ echo "If you want to install a server version or"
	@ echo "a client version, type \"make install_server\""
	@ echo "or \"make install_client\" respectively."
	@ echo "See README for more details."

install_server:
	cd server; make install
	cd support; make install
	cd doc; make install

install_client:
	cd inews; make install
	cd doc; make install

clean:
	for i in $(DISTDIRS); do cd $$i; make clean; cd ..; done

check:
	for i in $(DISTDIRS); do cd $$i; make check; cd ..; done

distrib: check
	mkdir ../nntp.dist
	cp -rp $(DIST) ../nntp.dist
	cd ../nntp.dist; for i in $(DISTDIRS); do cd $$i; make distrib; cd ..; done

rpt: /nowhere
	tar cf rrnpatches.tar rrnpatches; compress rrnpatches.tar

server: /nowhere
client: /nowhere

/nowhere: ;
