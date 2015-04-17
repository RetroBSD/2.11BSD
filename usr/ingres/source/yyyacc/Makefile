CC = cc
CFLAGS = -O
LDFLAGS = -i
OBJS = yyy0.o yyy1.o yyy2.o yyy3.o y4.o y5.o

yyyacc: $(OBJS)
	$(CC) $(LDFLAGS) -o yyyacc $(OBJS)

install: yyyacc
	install -c -m 751 yyyacc ../../bin/yyyacc

clean:
	rm -f yyyacc $(OBJS) *.core a.out
