# yeah, couldn't do more simple really

ifeq ($(PLATFORM),mingw32)
  BINEXT=.exe
else
  BINEXT=
endif

ifeq ($(PLATFORM),sunos)
  INSTALL=ginstall
else
  INSTALL=install
endif

CC=gcc
Q3ASM_CFLAGS=-O2 -Wall -Werror -fno-strict-aliasing

ifeq ($(PLATFORM),darwin)
  LCC_CFLAGS += -DMACOS_X=1
endif

ifndef USE_CCACHE
  USE_CCACHE=0
endif

ifeq ($(USE_CCACHE),1)
  CC := ccache $(CC)
  CXX := ccache $(CXX)
endif

default: q3asm

q3asm: q3asm.c cmdlib.c
	$(CC) $(Q3ASM_CFLAGS) -o $@ $^

clean:
	rm -f q3asm *~ *.o

install: default
	$(INSTALL) -s -m 0755 q3asm$(BINEXT) ../

uninstall:
	rm -f ../q3asm$(BINEXT)
