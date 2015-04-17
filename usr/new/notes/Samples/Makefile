#	Not much to this notesfile; merely copy the files 
#	and don't break for the others.
#

DISTDIR= p:/usr/src/uiuc/notesfiles
DCP=	rcp

SRC=	Archive-into Read_me Makefile access-template archlist \
	avail.notes cronarch.sh cronlocal.sh cronnet.sh \
	crontab net.how newsgroups shorten syncnews sys sysA \
	sysB cronarch2.sh cronarch-3.sh cron.convex.sh short.names \
	uiucdcs.list uiucdcsb.list uiucdcsi.list \
	cron.csh cron.sh hourly.csh cronXarch.sh moderators

all:
install:
clean:

dist:	$(SRC)
	$(DCP) $? $(DISTDIR)/Samples/.
	touch dist
