all: minit msvc pidfilehack hard-reboot write_proc killall5 shutdown

#CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
DIET=/opt/diet/bin/diet
CC=gcc
CFLAGS=-Wall -W -pipe -fomit-frame-pointer -Os
CROSS=
#CROSS=arm-linux-
LDFLAGS=-s

minit: minit.o split.o openreadclose.o fmt_ulong.o str_len.o opendevconsole.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o minit $^

msvc: msvc.o fmt_ulong.o buffer_1.o buffer_2.o buffer_puts.o \
buffer_putsflush.o buffer_putulong.o buffer_put.o byte_copy.o \
buffer_flush.o buffer_stubborn.o buffer_putflush.o str_len.o \
str_start.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o msvc $^

shutdown: shutdown.o split.o openreadclose.o opendevconsole.o
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o shutdown $^

%.o: %.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o minit msvc pidfilehack hard-reboot write_proc killall5 \
	shutdown

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a

pidfilehack: pidfilehack.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

hard-reboot: hard-reboot.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

write_proc: write_proc.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

killall5: killall5.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $^

install-files:
	install minit pidfilehack $(DESTDIR)/sbin
	install write_proc hard-reboot $(DESTDIR)/sbin
	install msvc $(DESTDIR)/bin
	install -m 4750 shutdown $(DESTDIR)/sbin
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

buffer_1.o: buffer_1.c buffer.h
buffer_2.o: buffer_2.c buffer.h
buffer_flush.o: buffer_flush.c buffer.h
buffer_put.o: buffer_put.c byte.h buffer.h
buffer_putflush.o: buffer_putflush.c buffer.h
buffer_puts.o: buffer_puts.c str.h buffer.h
buffer_putsflush.o: buffer_putsflush.c str.h buffer.h
buffer_putulong.o: buffer_putulong.c buffer.h fmt.h
buffer_stubborn.o: buffer_stubborn.c buffer.h
byte_copy.o: byte_copy.c byte.h
fmt_ulong.o: fmt_ulong.c fmt.h
hard-reboot.o: hard-reboot.c
killall5.o: killall5.c
minit.o: minit.c fmt.h str.h
msvc.o: msvc.c str.h fmt.h buffer.h
openreadclose.o: openreadclose.c
pidfilehack.o: pidfilehack.c
shutdown.o: shutdown.c
split.o: split.c
str_len.o: str_len.c str.h
str_start.o: str_start.c str.h
t.o: t.c
write_proc.o: write_proc.c
opendevconsole.o: opendevconsole.c
