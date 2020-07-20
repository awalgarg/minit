all: minit msvc pidfilehack hard-reboot write_proc killall5 shutdown \
minit-update serdo ftrigger waitinterface waitport powersave # governor

#CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
CC=gcc
PIE=
CFLAGS=-Wall -W -pipe -fomit-frame-pointer -Os $(PIE)
CROSS=
#CROSS=arm-linux-
LDFLAGS=-s
MANDIR=/usr/man

path = $(subst :, ,$(PATH))
diet_path = $(foreach dir,$(path),$(wildcard $(dir)/diet))
ifeq ($(strip $(diet_path)),)
ifneq ($(wildcard /opt/diet/bin/diet),)
DIET=/opt/diet/bin/diet
else
DIET=
endif
else
DIET:=$(strip $(diet_path))
endif

ifneq ($(DEBUG),)
CFLAGS+=-g
LDFLAGS+=-g
else
CFLAGS+=-O2 -fomit-frame-pointer
LDFLAGS+=-s
ifneq ($(DIET),)
DIET+=-Os
endif
endif

ifneq ($(MINITROOT),)
CFLAGS+="-DMINITROOT=\"$(MINITROOT)\""
else
MINITROOT=/etc/minit
endif

LDLIBS=-lowfat

libowfat_path = $(strip $(foreach dir,../libowfat*,$(wildcard $(dir)/textcode.h)))
ifneq ($(libowfat_path),)
CFLAGS+=$(foreach fnord,$(libowfat_path),-I$(dir $(fnord)))
LDFLAGS+=$(foreach fnord,$(libowfat_path),-L$(dir $(fnord)))
endif

minit: minit.o split.o openreadclose.o opendevconsole.o
msvc: msvc.o
ftrigger: ftrigger.o
minit-update: minit-update.o split.o openreadclose.o
serdo: serdo.o
waitinterface: waitinterface.o
waitport: waitport.o
powersave: powersave.o
governor: governor.o

sepcode:
	echo "int main() { return 0; }" > true.c
	if $(DIET) $(CROSS)$(CC) $(CFLAGS) $(FLAGS) -Wl,-z,noseparate-code -o true true.c ; then echo -Wl,-z,noseparate-code 2>/dev/null; fi > sepcode
	rm -f true true.c

shutdown: shutdown.o split.o openreadclose.o opendevconsole.o sepcode
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o shutdown $(subst sepcode,,$^) $(shell cat sepcode)

%.o: %.c
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $<

%: %.o sepcode
	$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $(subst sepcode,,$^) $(LDLIBS) $(shell cat sepcode)

clean:
	rm -f *.o minit msvc pidfilehack hard-reboot write_proc killall5 \
	shutdown minit-update serdo ftrigger waitinterface waitport \
	governor powersave sepcode

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a

pidfilehack: pidfilehack.c sepcode
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $< $(shell cat sepcode)

hard-reboot: hard-reboot.c sepcode
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $< $(shell cat sepcode)

write_proc: write_proc.c sepcode
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $< $(shell cat sepcode)

killall5: killall5.c sepcode
	$(DIET) $(CROSS)$(CC) $(CFLAGS) -o $@ $< $(shell cat sepcode)

install-files:
	install -d $(DESTDIR)$(MINITROOT) $(DESTDIR)/sbin $(DESTDIR)/bin $(DESTDIR)$(MANDIR)/man8 $(DESTDIR)$(MANDIR)/man1
	install minit pidfilehack $(DESTDIR)/sbin
	install write_proc hard-reboot minit-update $(DESTDIR)/sbin
	install msvc serdo ftrigger waitinterface waitport $(DESTDIR)/bin
	if test -f $(DESTDIR)/sbin/shutdown; then install shutdown $(DESTDIR)/sbin/mshutdown; else install shutdown $(DESTDIR)/sbin/shutdown; fi
	test -f $(DESTDIR)/sbin/init || ln $(DESTDIR)/sbin/minit $(DESTDIR)/sbin/init
	install -m 644 hard-reboot.8 minit-list.8 minit-shutdown.8 minit-update.8 minit.8 msvc.8 pidfilehack.8 serdo.8 $(DESTDIR)$(MANDIR)/man8
	install -m 644 waitinterface.1 waitport.1 ftrigger.1 $(DESTDIR)$(MANDIR)/man1

install-fifos:
	-mkfifo -m 600 $(DESTDIR)$(MINITROOT)/in $(DESTDIR)$(MINITROOT)/out

install: install-files install-fifos

VERSION=minit-$(shell head -n 1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

tar: clean rename
	cd ..; tar cvvf $(VERSION).tar.bz2 --use=bzip2 --exclude CVS $(VERSION)

rename:
	if test $(CURNAME) != $(VERSION); then cd .. && mv $(CURNAME) $(VERSION); fi

pie:
	$(MAKE) all PIE=-fpie
