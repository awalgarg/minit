all: minit msvc pidfilehack

CFLAGS=-pipe -march=i386 -fomit-frame-pointer -Os -I../dietlibc/include
#LDFLAGS=../dietlibc/start.o ../dietlibc/dietlibc.a

minit: minit.o split.o openreadclose.o
	gcc -g $(LDFLAGS) -o minit $^

msvc: msvc.o
	gcc -g $(LDFLAGS) -o msvc msvc.o

%.o: %.c
	gcc $(CFLAGS) -c $^

diet: minit.c split.o openreadclose.o
	gcc -nostdlib -o minit -pipe -Os -m386 -I../dietlibc/include minit.c split.c openreadclose.c ../dietlibc/start.o ../dietlibc/dietlibc.a
	gcc -nostdlib -o msvc -pipe -Os -m386 -I../dietlibc/include msvc.c ../dietlibc/start.o ../dietlibc/dietlibc.a
	gcc -nostdlib -o pidfilehack -pipe -Os -m386 -I../dietlibc/include pidfilehack.c ../dietlibc/start.o ../dietlibc/dietlibc.a
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
