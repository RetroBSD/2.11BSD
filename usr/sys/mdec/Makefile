#
# Copyright (c) 1980, 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1995/12/05
#
ALL=	bruboot dvhpuboot hkuboot hpuboot rauboot rkuboot rluboot \
	rm03uboot rm05uboot si51uboot si94uboot si95uboot rx02uboot \
	rx01uboot

all: ${ALL}

${ALL}:
	/lib/cpp -E $*.s | sed -e 's;^#;/;' | as -o a.out
	nm -u a.out
	size a.out
	strip a.out
	dd if=a.out of=b.out bs=16 skip=1
	dd if=b.out of=$@ conv=sync

bruboot:	bruboot.s
dvhpuboot:	dvhpuboot.s
hkuboot:	hkuboot.s
hpuboot:	hpuboot.s
rauboot:	rauboot.s
rkuboot:	rkuboot.s
rluboot:	rluboot.s
rm03uboot:	rm03uboot.s
rm05uboot:	rm05uboot.s
si51uboot:	si51uboot.s
si94uboot:	si94uboot.s
si95uboot:	si95uboot.s
rx02uboot:	rx02uboot.s
rx01uboot:	rx01uboot.s

clean:
	rm -f x.s a.out b.out ${ALL}

install:
	cp *boot ${DESTDIR}/mdec
