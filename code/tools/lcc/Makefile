TEMPDIR=/tmp
A=.a
O=.o

ifeq ($(PLATFORM),mingw32)
  E=.exe
else
  E=
endif

CC=gcc
LCC_CFLAGS=-O2 -Wall -fno-strict-aliasing -MMD
LDFLAGS=
LD=gcc
AR=ar
ARFLAGS=cru
RANLIB=ranlib
DIFF=diff
RM=rm -f
RMDIR=rmdir
BUILDDIR=build-$(PLATFORM)-$(ARCH)
BD=$(BUILDDIR)/

ifeq ($(USE_CCACHE),1)
  CC := ccache $(CC)
endif

# Need MACOS_X defined or this won't build.
ifeq ($(PLATFORM),darwin)
  LCC_CFLAGS += -DMACOS_X
endif

ifeq ($(PLATFORM),sunos)
  INSTALL=ginstall
else
  INSTALL=install
endif

all: q3rcc lburg q3cpp q3lcc

q3rcc: makedirs $(BD)q3rcc$(E)
lburg: makedirs $(BD)lburg$(E)
q3cpp: makedirs $(BD)q3cpp$(E)
q3lcc: makedirs $(BD)q3lcc$(E)

makedirs:
	@if [ ! -d $(BD) ];then mkdir $(BD);fi
	@if [ ! -d $(BD)/etc ];then mkdir $(BD)/etc;fi
	@if [ ! -d $(BD)/rcc ];then mkdir $(BD)/rcc;fi
	@if [ ! -d $(BD)/cpp ];then mkdir $(BD)/cpp;fi
	@if [ ! -d $(BD)/lburg ];then mkdir $(BD)/lburg;fi

# ===== RCC =====
RCCOBJS= \
	$(BD)rcc/alloc$(O) \
	$(BD)rcc/bind$(O) \
	$(BD)rcc/bytecode$(O) \
	$(BD)rcc/dag$(O) \
	$(BD)rcc/dagcheck$(O) \
	$(BD)rcc/decl$(O) \
	$(BD)rcc/enode$(O) \
	$(BD)rcc/error$(O) \
	$(BD)rcc/event$(O) \
	$(BD)rcc/expr$(O) \
	$(BD)rcc/gen$(O) \
	$(BD)rcc/init$(O) \
	$(BD)rcc/inits$(O) \
	$(BD)rcc/input$(O) \
	$(BD)rcc/lex$(O) \
	$(BD)rcc/list$(O) \
	$(BD)rcc/main$(O) \
	$(BD)rcc/null$(O) \
	$(BD)rcc/output$(O) \
	$(BD)rcc/prof$(O) \
	$(BD)rcc/profio$(O) \
	$(BD)rcc/simp$(O) \
	$(BD)rcc/stmt$(O) \
	$(BD)rcc/string$(O) \
	$(BD)rcc/sym$(O) \
	$(BD)rcc/symbolic$(O) \
	$(BD)rcc/trace$(O) \
	$(BD)rcc/tree$(O) \
	$(BD)rcc/types$(O)

$(BD)q3rcc$(E): $(RCCOBJS)
	$(LD) $(LDFLAGS) -o $@ $(RCCOBJS)

$(BD)rcc/%$(O): src/%.c
	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $<

$(BD)rcc/dagcheck$(O): $(BD)rcc/dagcheck.c
	$(CC) $(LCC_CFLAGS) -Wno-unused -c -Isrc -o $@ $<

$(BD)rcc/dagcheck.c: $(BD)lburg/lburg$(E) src/dagcheck.md
	$(BD)lburg/lburg$(E) src/dagcheck.md $@


# ===== LBURG =====
LBURGOBJS= \
	$(BD)lburg/lburg$(O) \
	$(BD)lburg/gram$(O)

$(BD)lburg/lburg$(E): $(LBURGOBJS)
	$(LD) $(LDFLAGS) -o $@ $(LBURGOBJS)

$(BD)lburg/%$(O): lburg/%.c
	$(CC) $(LCC_CFLAGS) -c -Ilburg -o $@ $<


# ===== CPP =====
CPPOBJS= \
	$(BD)cpp/cpp$(O) \
	$(BD)cpp/lex$(O) \
	$(BD)cpp/nlist$(O) \
	$(BD)cpp/tokens$(O) \
	$(BD)cpp/macro$(O) \
	$(BD)cpp/eval$(O) \
	$(BD)cpp/include$(O) \
	$(BD)cpp/hideset$(O) \
	$(BD)cpp/getopt$(O) \
	$(BD)cpp/unix$(O)

$(BD)q3cpp$(E): $(CPPOBJS)
	$(LD) $(LDFLAGS) -o $@ $(CPPOBJS)

$(BD)cpp/%$(O): cpp/%.c
	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ $<


# ===== LCC =====
LCCOBJS= \
	$(BD)etc/lcc$(O) \
	$(BD)etc/bytecode$(O)

$(BD)q3lcc$(E): $(LCCOBJS)
	$(LD) $(LDFLAGS) -o $@ $(LCCOBJS)

$(BD)etc/%$(O): etc/%.c
	$(CC) $(LCC_CFLAGS) -DTEMPDIR=\"$(TEMPDIR)\" -DSYSTEM=\"\" -c -Isrc -o $@ $<


install: q3lcc q3cpp q3rcc
	$(INSTALL) -s -m 0755 $(BD)q3lcc$(E) ../
	$(INSTALL) -s -m 0755 $(BD)q3cpp$(E) ../
	$(INSTALL) -s -m 0755 $(BD)q3rcc$(E) ../

uninstall:
	-$(RM) ../q3lcc$(E)
	-$(RM) ../q3cpp$(E)
	-$(RM) ../q3rcc$(E)

clean:
	if [ -d $(BD) ];then (find $(BD) -name '*.d' -exec rm {} \;)fi
	$(RM) $(RCCOBJS) $(LBURGOBJS) $(CPPOBJS) $(LCCOBJS)
	$(RM) $(BD)rcc/dagcheck.c $(BD)lburg/lburg$(E)
	$(RM) $(BD)q3lcc$(E) $(BD)q3cpp$(E) $(BD)q3rcc$(E)
	$(RM) -r $(BD)

D_FILES=$(shell find . -name '*.d')

ifneq ($(strip $(D_FILES)),)
  include $(D_FILES)
endif
