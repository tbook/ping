CFLAGS=-Wall -Wshadow -Wcast-qual -pedantic -ansi -O3 -g

all: libmagic

libmagic: libmagic.a

libmagic.a: magic.o
	ar cru libmagic.a magic.o
	ranlib libmagic.a

magic.o: magic.c sysdep.h magic.h

