# yeah, couldn't do more simple really

CC=gcc
CFLAGS=-O2 -Wall -Werror -fno-strict-aliasing

default:	q3asm

q3asm:	q3asm.c cmdlib.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f q3asm *~ *.o

install: default
	install -s -m 0755 q3asm ../

uninstall:
	-rm ../q3asm
