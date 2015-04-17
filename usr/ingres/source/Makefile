INGRES=..
BIN=$(INGRES)/bin
DEMO=$(INGRES)/demo
LIB=$(INGRES)/lib
DESTDIR=/usr
DESTBIN=$(DESTDIR)/bin
DESTLIB=$(DESTDIR)/lib
INSTALL=install
INSTALLQ=installq
SYSALL = creatdb ingres destroydb sysmod purge printr helpr restore equel
MAKE=	make

install:
	chdir gutil; $(MAKE) $(INSTALL)
	chdir iutil; $(MAKE) $(INSTALL)
	chdir access; $(MAKE) $(INSTALL)
	chdir dbu; $(MAKE) $(INSTALL)
	chdir support; $(MAKE) $(INSTALL)
	chdir monitor; $(MAKE) $(INSTALL)
	chdir scanner; $(MAKE) $(INSTALL)
	chdir yyyacc; $(MAKE) $(INSTALL)
	chdir parser; $(MAKE) $(INSTALL)
	chdir qrymod; $(MAKE) $(INSTALL)
	chdir ovqp; $(MAKE) $(INSTALL)
	chdir decomp; $(MAKE) $(INSTALL)
	chdir equel; $(MAKE) $(INSTALL)
	chdir equel/runtime; $(MAKE) $(INSTALL)
	chdir support; $(MAKE) $(INSTALLQ)

sysinstall:
	@echo "I hope you're doing this as root!"
	install -s -o ingres -m 4751 $(BIN)/creatdb $(DESTBIN)/creatdb
	install -s -o ingres -m 4751 $(BIN)/ingres $(DESTBIN)/ingres
	install -s -o ingres -m 4751 $(BIN)/destroydb $(DESTBIN)/destroydb
	install -s -o ingres -m 4751 $(BIN)/sysmod $(DESTBIN)/sysmod
	install -s -o ingres -m 4751 $(BIN)/purge $(DESTBIN)/purge
	install -s -o ingres -m 4751 $(BIN)/printr $(DESTBIN)/printr
	install -s -o ingres -m 4751 $(BIN)/helpr $(DESTBIN)/helpr
	install -s -o ingres -m 4751 $(BIN)/restore $(DESTBIN)/restore
	install -s -m 751 $(BIN)/equel $(DESTBIN)/equel
	install -s -m 751 $(BIN)/copydb $(DESTBIN)/copydb
	install -c -m 755 $(DEMO)/demodb $(DESTBIN)/demodb
	install -c -m 644 $(LIB)/libq.a $(DESTLIB)/libq.a
	ranlib $(DESTLIB)/libq.a
	chmod 1700 $(BIN)/ksort

clean:
	chdir gutil; $(MAKE) clean
	chdir iutil; $(MAKE) clean
	chdir access; $(MAKE) clean
	chdir dbu; $(MAKE) clean
	chdir support; $(MAKE) clean
	chdir monitor; $(MAKE) clean
	chdir scanner; $(MAKE) clean
	chdir yyyacc; $(MAKE) clean
	chdir parser; $(MAKE) clean
	chdir qrymod; $(MAKE) clean
	chdir ovqp; $(MAKE) clean
	chdir decomp; $(MAKE) clean
	chdir equel; $(MAKE) clean
	chdir equel/runtime; $(MAKE) clean
