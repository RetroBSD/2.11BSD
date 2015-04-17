# tbl is used because a few of these files have been converted to use
# tables instead of handcrafted boxes.

TBL=	tbl

NROFF=	nroff
# NMACS=	../naa

TROFF=	troff
TMACS=	../taa

GROFF=	groff -t -mps -Tps
GMACS=	../naa

FILES= convert.nr fileio.nr howto_setup.nr indconv.nr \
	lockdev.nr maintain.nr setup_geo.nr tutorial.nr v5v6diff.nr \
	v61v62diff.nr v6v61diff.nr

SPECIAL= design.nr

ref:
	for i in $(FILES); \
	do \
	    $(TBL) $(NMACS) $$i | $(NROFF) > `basename $$i .nr`.NROFF; \
	done
	$(TBL) $(NMACS) design.nr | $(NROFF) -me > design.NROFF

tref:
	for i in $(FILES); \
	do \
	    $(TBL) $(TMACS) $$i | $(TROFF) > `basename $$i .nr`.TROFF; \
	done
	$(TBL) $(TMACS) design.nr | $(TROFF) -me > design.TROFF

gref:
	for i in $(FILES); \
	do \
	    $(GROFF) $(GMACS) $$i > `basename $$i .nr`.PS; \
	done
	$(GROFF) -me design.nr > design.PS

clean:
	-rm -f *.NROFF *.TROFF *.PS
