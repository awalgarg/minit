all: minit msvc pidfilehack

#CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
DIET=diet
CC=gcc
CFLAGS=-pipe -fomit-frame-pointer -Os
CROSS=
#CROSS=arm-linux-
LDFLAGS=-s

minit: minit.o split.o openreadclose.o fmt_ulong.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o minit $^

msvc: msvc.o fmt_ulong.o buffer_1.o buffer_2.o buffer_puts.o \
buffer_putsflush.o buffer_putulong.o buffer_put.o byte_copy.o \
buffer_flush.o buffer_stubborn.o buffer_putflush.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o msvc $^

%.o: %.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o minit msvc pidfilehack

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a

pidfilehack: pidfilehack.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

install:
	install minit msvc pidfilehack /usr/sbin
	test -d /etc/minit || mkdir /etc/minit
	-mkfifo -m 600 /etc/minit/in /etc/minit/out

tar: clean
	cd .. && tar cvvf minit.tar.bz2 minit --use=bzip2 --exclude CVS
