all: minit msvc pidfilehack hard-reboot write_proc

#CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
DIET=diet
CC=gcc
CFLAGS=-pipe -fomit-frame-pointer -Os
CROSS=
#CROSS=arm-linux-
LDFLAGS=-s

minit: minit.o split.o openreadclose.o fmt_ulong.o str_len.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o minit $^

msvc: msvc.o fmt_ulong.o buffer_1.o buffer_2.o buffer_puts.o \
buffer_putsflush.o buffer_putulong.o buffer_put.o byte_copy.o \
buffer_flush.o buffer_stubborn.o buffer_putflush.o str_len.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o msvc $^

%.o: %.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o minit msvc pidfilehack hard-reboot

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a

pidfilehack: pidfilehack.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

hard-reboot: hard-reboot.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

write_proc: write_proc.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

install-files:
	install minit pidfilehack $(DESTDIR)/sbin
	install write_proc hard-reboot $(DESTDIR)/sbin
	install msvc $(DESTDIR)/bin
	test -d $(DESTDIR)/etc/minit || mkdir $(DESTDIR)/etc/minit

install-fifos:
	-mkfifo -m 600 $(DESTDIR)/etc/minit/in $(DESTDIR)/etc/minit/out

install: install-files install-fifos

VERSION=minit-$(shell head -1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

tar: clean rename
	cd .. && tar cvvf $(VERSION).tar.bz2 $(VERSION) --use=bzip2 --exclude CVS

rename:
	if test $(CURNAME) != $(VERSION); then cd .. && mv $(CURNAME) $(VERSION); fi

