# yeah, couldn't do more simple really

CC=gcc
CFLAGS=-g -Wall

default:	q3asm

q3asm:	q3asm.c cmdlib.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f q3asm *~ *.o

