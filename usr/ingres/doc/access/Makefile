# tbl is used just in case the files are changed to use tables in the future

TBL=	tbl

NROFF=	nroff
NMACS=	../naa

TROFF=	troff
TMACS=	../taa

GROFF=	groff -t -mps -Tps
GMACS=	../naa

FILES= 1aa.nr addbatch.nr cleanrel.nr clearkeys.nr closebatch.nr closer.nr \
	delete.nr find.nr formatpg.nr get.nr getequal.nr icompare.nr \
	inpcloser.nr insert.nr internal.nr kcompare.nr noclose.nr \
	openbatch.nr openr.nr replace.nr rmbatch.nr setkey.nr

ref:
	$(TBL) $(NMACS) $(FILES) | $(NROFF) > access.NROFF

tref:
	$(TBL) $(TMACS) $(FILES) | $(TROFF) > access.TROFF

gref:
	$(GROFF) $(GMACS) $(FILES) > access.PS

clean:
	-rm -f *.NROFF *.TROFF *.PS
