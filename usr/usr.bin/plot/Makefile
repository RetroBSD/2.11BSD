#	Makefile	4.7	85/11/11
#
CFLAGS=	-O
SEPFLAG= -i
ALL=	tek t4013 t300 t300s t450 aedplot bgplot crtplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa

all:	${ALL} debug

tek:	driver.o
	cc ${SEPFLAG} -o tek  driver.o -l4014 -lm

t4013:	driver.o
	cc ${SEPFLAG} -o t4013  driver.o -l4013 -lm

t300:	driver.o 
	cc ${SEPFLAG} -o t300 driver.o -l300 -lm

t300s:	driver.o 
	cc ${SEPFLAG} -o t300s driver.o -l300s -lm

t450:	driver.o 
	cc ${SEPFLAG} -o t450 driver.o -l450 -lm

aedplot: driver.o
	cc ${SEPFLAG} -o aedplot driver.o -lplotaed

bgplot: driver.o
	cc ${SEPFLAG} -o bgplot driver.o -lplotbg -lm

crtplot: crtdriver.o crtplot.o
	cc ${SEPFLAG} -o crtplot crtdriver.o crtplot.o -lcurses -ltermcap -lm

dumbplot: driver.o
	cc ${SEPFLAG} -o dumbplot driver.o -lplotdumb -ltermcap -lm

gigiplot: driver.o
	cc ${SEPFLAG} -o gigiplot driver.o -lplotgigi -lm

hpplot: driver.o
	cc ${SEPFLAG} -o hpplot driver.o -lplot2648 -lm

hp7221plot: driver.o
	cc ${SEPFLAG} -o hp7221plot driver.o -lplot7221 -lm

implot: driver.o
	cc ${SEPFLAG} -o implot driver.o -lplotimagen -lm

atoplot: atoplot.o
	cc ${SEPFLAG} -o atoplot atoplot.o -lplot -lm

plottoa: plottoa.o
	cc ${SEPFLAG} -o plottoa plottoa.o

debug: debug.o
	cc ${SEPFLAG} -o debug debug.o

install: all
	-for i in ${ALL}; do \
		(install -s $$i ${DESTDIR}/usr/bin/$$i); done
	install -c plot.sh ${DESTDIR}/usr/bin/plot

clean:
	rm -f *.o ${ALL} a.out core errs debug
