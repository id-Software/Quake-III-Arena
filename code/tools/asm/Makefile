# yeah, couldn't do more simple really

PLATFORM=$(shell uname|sed -e s/_.*//|tr A-Z a-z)
ifeq ($(PLATFORM),mingw32)
  BINEXT=.exe
else
  BINEXT=
endif

CC=gcc
Q3ASM_CFLAGS=-O2 -Wall -Werror -fno-strict-aliasing

default:	q3asm

q3asm:	q3asm.c cmdlib.c
	$(CC) $(Q3ASM_CFLAGS) -o $@ $^

clean:
	rm -f q3asm *~ *.o

install: default
	install -s -m 0755 q3asm$(BINEXT) ../

uninstall:
	-rm ../q3asm$(BINEXT)
