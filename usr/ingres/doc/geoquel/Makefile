# tbl is used just in case the files are changed to use tables in the future

TBL=	tbl

NROFF=	nroff
NMACS=	../naa

TROFF=	troff
TMACS=	../taa

GROFF=	groff -t -mps -Tps
GMACS=	../naa

FILES= 0aa.nr 1aa.nr center.nr deletemap.nr errors.nr help.nr histogram.nr \
	linegraph.nr map.nr monitor.nr overlay.nr pointgraph.nr print.nr \
	savemap.nr shade.nr

ref:
	$(TBL) $(NMACS) $(FILES) | $(NROFF) > geoquel.NROFF

tref:
	$(TBL) $(TMACS) $(FILES) | $(TROFF) > geoquel.TROFF

gref:
	$(GROFF) $(GMACS) $(FILES) > geoquel.PS

clean:
	-rm -f *.NROFF *.TROFF *.PS
