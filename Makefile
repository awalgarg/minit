all: minit msvc

minit: minit.o split.o openreadclose.o
	gcc -g -o minit $^

msvc: msvc.o
	gcc -g -o msvc msvc.o

%.o: %.c
	gcc -pipe -g -c $^

diet: minit.c split.o openreadclose.o
	gcc -nostdlib -o minit -pipe -Os -m386 minit.c split.c openreadclose.c ../dietlibc/start.o ../dietlibc/dietlibc.a
	gcc -nostdlib -o msvc -pipe -Os -m386 msvc.c ../dietlibc/start.o ../dietlibc/dietlibc.a
	strip -R .note -R .comment minit msvc

diet2: minit.c split.o openreadclose.o
	gcc -nostdlib -g -o minit -pipe minit.c split.c openreadclose.c ../dietlibc/start.o ../dietlibc/dietlibc.a

clean:
	rm -f *.o minit msvc

test: test.c
	gcc -nostdlib -o $@ $^ -I../dietlibc/include ../dietlibc/start.o ../dietlibc/dietlibc.a
