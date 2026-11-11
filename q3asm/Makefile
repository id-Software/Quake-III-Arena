# Portable Q3ASM build (C99).
CC ?= cc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra
LDFLAGS ?=

.PHONY: all clean test
all: q3asm

q3asm: q3asm.c cmdlib.c cmdlib.h mathlib.h qfiles.h opstrings.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ q3asm.c cmdlib.c $(LDFLAGS)

clean:
	rm -f q3asm *.o

test: q3asm
	python3 tests/test_q3asm.py
