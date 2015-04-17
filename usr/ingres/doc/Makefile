# tbl is used just in case the files are changed to use tables in the future

TBL=	tbl

NROFF=	nroff
NMACS=	naa

TROFF=	troff
TMACS=	taa

GMACS=	naa
GROFF=	groff -t -mps -Tps

ref:
	$(TBL) $(NMACS) 0aa.nr 1aa.nr toc.nr | $(NROFF) > ref.NROFF
	chdir quel; $(TBL) ../$(NMACS) *.nr  | $(NROFF) >> ../ref.NROFF
	chdir unix; $(TBL) ../$(NMACS) *.nr  | $(NROFF) >> ../ref.NROFF
	chdir files; $(TBL) ../$(NMACS) *.nr | $(NROFF) >> ../ref.NROFF
	chdir error; $(TBL) ../$(NMACS) *.nr | $(NROFF) >> ../ref.NROFF
	chdir access; make ref
	chdir geoquel; make ref
	chdir other; make ref

tref:
	$(TBL) $(TMACS) 0aa.nr 1aa.nr toc.nr | $(TROFF) > ref.TROFF
	chdir quel; $(TBL) ../$(TMACS) *.nr | $(TROFF) >> ../ref.TROFF
	chdir unix; $(TBL) ../$(TMACS) *.nr | $(TROFF) >> ../ref.TROFF
	chdir files; $(TBL) ../$(TMACS) *.nr | $(TROFF) >> ../ref.TROFF
	chdir error; $(TBL) ../$(TMACS) *.nr | $(TROFF) >> ../ref.TROFF
	chdir access; make tref
	chdir geoquel; make tref
	chdir other; make tref

gref:
	$(GROFF) $(GMACS) 0aa.nr 1aa.nr toc.nr > ref.PS
	chdir quel; $(GROFF) ../$(GMACS) *.nr >> ../ref.PS
	chdir unix; $(GROFF) ../$(GMACS) *.nr >> ../ref.PS
	chdir files; $(GROFF) ../$(GMACS) *.nr >> ../ref.PS
	chdir error; $(GROFF) ../$(GMACS) *.nr >> ../ref.PS
	chdir access; make gref
	chdir geoquel; make gref
	chdir other; make gref

clean:
	-rm -f *.NROFF *.TROFF *.PS
	cd access; make clean
	cd geoquel; make clean
	cd other; make clean
