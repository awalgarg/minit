all: minit msvc pidfilehack

#CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
CFLAGS=-pipe -fomit-frame-pointer -Os -I../dietlibc/include
CROSS=arm-linux-
LDFLAGS=../dietlibc/start.o ../dietlibc/dietlibc.a -lgcc-sf

minit: minit.o split.o openreadclose.o
	gcc -g $(LDFLAGS) -o minit $^

msvc: msvc.o
	gcc -g $(LDFLAGS) -o msvc msvc.o

%.o: %.c
	gcc $(CFLAGS) -c $^

diet: minit.c split.o openreadclose.o
	$(CROSS)gcc -nostdlib -o minit $(CFLAGS) minit.c split.c openreadclose.c $(LDFLAGS)
	$(CROSS)gcc -nostdlib -o msvc $(CFLAGS) msvc.c $(LDFLAGS)
	$(CROSS)gcc -nostdlib -o pidfilehack $(CFLAGS) pidfilehack.c $(LDFLAGS)
	strip -R .note -R .comment minit msvc pidfilehack

diet2: minit.c split.o openreadclose.o
	gcc -nostdlib -g -o minit -pipe minit.c split.c openreadclose.c ../dietlibc/start.o ../dietlibc/dietlibc.a

clean:
	rm -f *.o minit msvc pidfilehack

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a

pidfilehack: pidfilehack.c
	gcc -nostdlib -pipe -g -o $@ $^ ../dietlibc/start.o ../dietlibc/dietlibc.a

install:
	install minit msvc pidfilehack /usr/sbin
	test -d /etc/minit || mkdir /etc/minit
	mkfifo -m 600 /etc/minit/in /etc/minit/out

tar: clean
	cd .. && tar cvvf minit.tar.bz2 minit --use=bzip2 --exclude CVS
