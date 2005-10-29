# $Id: makefile 145 2001-10-17 21:53:10Z timo $
PLATFORM=$(shell uname|sed -e s/_.*//|tr A-Z a-z)
HOSTFILE=etc/linux.c
TEMPDIR=/tmp
A=.a
O=.o

ifeq ($(PLATFORM),mingw32)
  E=.exe
else
  E=
endif

CC=gcc
LCC_CFLAGS=-O2 -Wall -fno-strict-aliasing
LDFLAGS=
LD=gcc
AR=ar
ARFLAGS=cru
RANLIB=ranlib
DIFF=diff
RM=rm -f
RMDIR=rmdir
BUILDDIR=build
TSTDIR=$(BUILDDIR)/$(TARGET)/tst
BD=$(BUILDDIR)/
T=$(TSTDIR)/
ifeq ($(PLATFORM),SunOS)
  INSTALL=ginstall
HOSTFILE=etc/gcc-solaris.c
else
  INSTALL=install
endif

# $Id: makefile 145 2001-10-17 21:53:10Z timo $

what:
	-@echo make all q3rcc lburg q3cpp q3lcc bprint liblcc triple clean clobber

makedirs:
	@if [ ! -d $(BD) ];then mkdir $(BD);fi
	@if [ ! -d $(T) ];then mkdir $(T);fi
	
all::	q3rcc lburg q3cpp q3lcc bprint liblcc

q3rcc:	makedirs $(BD)q3rcc$(E)
lburg:	makedirs $(BD)lburg$(E)
q3cpp:	makedirs $(BD)q3cpp$(E)
q3lcc:	makedirs $(BD)q3lcc$(E)
bprint:	makedirs $(BD)bprint$(E)
liblcc:	makedirs $(BD)liblcc$(A)

RCCOBJS=$(BD)alloc$(O) \
	$(BD)bind$(O) \
	$(BD)dag$(O) \
	$(BD)dagcheck$(O) \
	$(BD)decl$(O) \
	$(BD)enode$(O) \
	$(BD)error$(O) \
	$(BD)expr$(O) \
	$(BD)event$(O) \
	$(BD)init$(O) \
	$(BD)inits$(O) \
	$(BD)input$(O) \
	$(BD)lex$(O) \
	$(BD)list$(O) \
	$(BD)main$(O) \
	$(BD)output$(O) \
	$(BD)prof$(O) \
	$(BD)profio$(O) \
	$(BD)simp$(O) \
	$(BD)stmt$(O) \
	$(BD)string$(O) \
	$(BD)sym$(O) \
	$(BD)trace$(O) \
	$(BD)tree$(O) \
	$(BD)types$(O) \
	$(BD)null$(O) \
	$(BD)symbolic$(O) \
	$(BD)gen$(O) \
	$(BD)bytecode$(O)

$(BD)q3rcc$(E)::	$(BD)main$(O) $(BD)librcc$(A) $(EXTRAOBJS)
		$(LD) $(LDFLAGS) -o $@ $(BD)main$(O) $(EXTRAOBJS) $(BD)librcc$(A) $(EXTRALIBS)

$(BD)librcc$(A):	$(RCCOBJS)
		$(AR) $(ARFLAGS) $@ $(RCCOBJS); $(RANLIB) $@ || true

$(RCCOBJS):	src/c.h src/token.h src/config.h

$(BD)alloc$(O):	src/alloc.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/alloc.c
$(BD)bind$(O):	src/bind.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/bind.c
$(BD)dag$(O):	src/dag.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/dag.c
$(BD)decl$(O):	src/decl.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/decl.c
$(BD)enode$(O):	src/enode.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/enode.c
$(BD)error$(O):	src/error.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/error.c
$(BD)event$(O):	src/event.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/event.c
$(BD)expr$(O):	src/expr.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/expr.c
$(BD)gen$(O):	src/gen.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/gen.c
$(BD)init$(O):	src/init.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/init.c
$(BD)inits$(O):	src/inits.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/inits.c
$(BD)input$(O):	src/input.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/input.c
$(BD)lex$(O):	src/lex.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/lex.c
$(BD)list$(O):	src/list.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/list.c
$(BD)main$(O):	src/main.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/main.c
$(BD)null$(O):	src/null.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/null.c
$(BD)output$(O):	src/output.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/output.c
$(BD)prof$(O):	src/prof.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/prof.c
$(BD)profio$(O):	src/profio.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/profio.c
$(BD)simp$(O):	src/simp.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/simp.c
$(BD)stmt$(O):	src/stmt.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/stmt.c
$(BD)string$(O):	src/string.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/string.c
$(BD)sym$(O):	src/sym.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/sym.c
$(BD)symbolic$(O):	src/symbolic.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/symbolic.c
$(BD)bytecode$(O):	src/bytecode.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/bytecode.c
$(BD)trace$(O):	src/trace.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/trace.c
$(BD)tree$(O):	src/tree.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/tree.c
$(BD)types$(O):	src/types.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/types.c
$(BD)stab$(O):	src/stab.c src/stab.h;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ src/stab.c

$(BD)dagcheck$(O):	$(BD)dagcheck.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)dagcheck.c
$(BD)alpha$(O):	$(BD)alpha.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)alpha.c
$(BD)mips$(O):	$(BD)mips.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)mips.c
$(BD)sparc$(O):	$(BD)sparc.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)sparc.c
$(BD)x86$(O):	$(BD)x86.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)x86.c
$(BD)x86linux$(O):	$(BD)x86linux.c;	$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ $(BD)x86linux.c

$(BD)dagcheck.c:	$(BD)lburg$(E) src/dagcheck.md; $(BD)lburg src/dagcheck.md $@
$(BD)alpha.c:	$(BD)lburg$(E) src/alpha.md;    $(BD)lburg src/alpha.md    $@
$(BD)mips.c:	$(BD)lburg$(E) src/mips.md;     $(BD)lburg src/mips.md     $@
$(BD)sparc.c:	$(BD)lburg$(E) src/sparc.md;    $(BD)lburg src/sparc.md    $@
$(BD)x86.c:	$(BD)lburg$(E) src/x86.md;      $(BD)lburg src/x86.md      $@
$(BD)x86linux.c:	$(BD)lburg$(E) src/x86linux.md; $(BD)lburg src/x86linux.md $@

$(BD)q3rcc.h:	src/rcc.asdl;			$(ASDL_HOME)/bin/asdlGen --c -d $(BD) src/rcc.asdl
$(BD)q3rcc$(O):	$(BD)rcc.h;			$(CC) $(LCC_CFLAGS) -c -Isrc -I$(BD) -I$(ASDL_HOME)/include/asdlGen -o $@ $(BD)rcc.c
$(BD)asdl$(O):	src/asdl.c $(BD)rcc.h src/c.h;	$(CC) $(LCC_CFLAGS) -c -Isrc -I$(BD) -I$(ASDL_HOME)/include/asdlGen -o $@ src/asdl.c
$(BD)pass2$(O):	src/pass2.c $(BD)rcc.h src/c.h;	$(CC) $(LCC_CFLAGS) -c -Isrc -I$(BD) -I$(ASDL_HOME)/include/asdlGen -o $@ src/pass2.c
$(BD)2html$(O):	src/2html.c $(BD)rcc.h src/c.h;	$(CC) $(LCC_CFLAGS) -c -Isrc -I$(BD) -I$(ASDL_HOME)/include/asdlGen -o $@ src/2html.c

$(BD)pass2$(E):	$(BD)pass2$(O) $(EXTRAOBJS) $(BD)librcc$(A);	$(LD) $(LDFLAGS) -o $@ $(BD)pass2$(O) $(EXTRAOBJS) $(BD)librcc$(A) $(EXTRALIBS)
$(BD)2html$(E):	$(BD)2html$(O) $(BD)q3rcc$(O);			$(LD) $(LDFLAGS) -o $@ $(BD)2html$(O) $(BD)q3rcc$(O) $(EXTRALIBS)

$(BD)bprint$(E):	$(BD)bprint$(O);		$(LD) $(LDFLAGS) -o $@ $(BD)bprint$(O) 

$(BD)bprint$(O):	etc/bprint.c;		$(CC) $(LCC_CFLAGS) -c -Isrc -o $@ etc/bprint.c

$(BD)q3lcc$(E):	$(BD)q3lcc$(O) $(BD)host$(O);	$(LD) $(LDFLAGS) -o $@ $(BD)q3lcc$(O) $(BD)host$(O) 

$(BD)q3lcc$(O):	etc/lcc.c;	$(CC) $(LCC_CFLAGS) -c -DTEMPDIR=\"$(TEMPDIR)\" -o $@ etc/lcc.c
$(BD)host$(O):	$(HOSTFILE);	$(CC) $(LCC_CFLAGS) -c -DSYSTEM=\"\" -o $@ $(HOSTFILE)

LIBOBJS=$(BD)assert$(O) $(BD)bbexit$(O) $(BD)yynull$(O)

$(BD)liblcc$(A):	$(LIBOBJS);	$(AR) $(ARFLAGS) $@ $(BD)assert$(O) $(BD)bbexit$(O) $(BD)yynull$(O); $(RANLIB) $@ || true

$(BD)assert$(O):	lib/assert.c;	$(CC) $(LCC_CFLAGS) -c -o $@ lib/assert.c
$(BD)yynull$(O):	lib/yynull.c;	$(CC) $(LCC_CFLAGS) -c -o $@ lib/yynull.c
$(BD)bbexit$(O):	lib/bbexit.c;	$(CC) $(LCC_CFLAGS) -c -o $@ lib/bbexit.c

$(BD)lburg$(E):	$(BD)lburg$(O) $(BD)gram$(O);	$(LD) $(LDFLAGS) -o $@ $(BD)lburg$(O) $(BD)gram$(O) 

$(BD)lburg$(O) $(BD)gram$(O):	lburg/lburg.h

$(BD)lburg$(O):	lburg/lburg.c;	$(CC) $(LCC_CFLAGS) -c -Ilburg -o $@ lburg/lburg.c
$(BD)gram$(O):	lburg/gram.c;	$(CC) $(LCC_CFLAGS) -c -Ilburg -o $@ lburg/gram.c

CPPOBJS=$(BD)q3cpp$(O) $(BD)lexer$(O) $(BD)nlist$(O) $(BD)tokens$(O) $(BD)macro$(O) $(BD)eval$(O) \
	$(BD)include$(O) $(BD)hideset$(O) $(BD)getopt$(O) $(BD)unix$(O)

$(BD)q3cpp$(E):	$(CPPOBJS)
		$(LD) $(LDFLAGS) -o $@ $(CPPOBJS) 

$(CPPOBJS):	cpp/cpp.h

$(BD)q3cpp$(O):	cpp/cpp.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/cpp.c
$(BD)lexer$(O):	cpp/lex.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/lex.c
$(BD)nlist$(O):	cpp/nlist.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/nlist.c
$(BD)tokens$(O):	cpp/tokens.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/tokens.c
$(BD)macro$(O):	cpp/macro.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/macro.c
$(BD)eval$(O):	cpp/eval.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/eval.c
$(BD)include$(O):	cpp/include.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/include.c
$(BD)hideset$(O):	cpp/hideset.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/hideset.c
$(BD)getopt$(O):	cpp/getopt.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/getopt.c
$(BD)unix$(O):	cpp/unix.c;	$(CC) $(LCC_CFLAGS) -c -Icpp -o $@ cpp/unix.c

test:	$(T)8q.s \
	$(T)array.s \
	$(T)cf.s \
	$(T)cq.s \
	$(T)cvt.s \
	$(T)fields.s \
	$(T)front.s \
	$(T)incr.s \
	$(T)init.s \
	$(T)limits.s \
	$(T)paranoia.s \
	$(T)sort.s \
	$(T)spill.s \
	$(T)stdarg.s \
	$(T)struct.s \
	$(T)switch.s \
	$(T)wf1.s \
	$(T)yacc.s

$(T)8q.s:	tst/8q.c tst/8q.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)array.s:	tst/array.c tst/array.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)cf.s:	tst/cf.c tst/cf.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)cq.s:	tst/cq.c tst/cq.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)cvt.s:	tst/cvt.c tst/cvt.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)fields.s:	tst/fields.c tst/fields.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)front.s:	tst/front.c tst/front.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)incr.s:	tst/incr.c tst/incr.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)init.s:	tst/init.c tst/init.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)limits.s:	tst/limits.c tst/limits.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)paranoia.s:	tst/paranoia.c tst/paranoia.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)sort.s:	tst/sort.c tst/sort.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)spill.s:	tst/spill.c tst/spill.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)stdarg.s:	tst/stdarg.c tst/stdarg.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)struct.s:	tst/struct.c tst/struct.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)switch.s:	tst/switch.c tst/switch.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)wf1.s:	tst/wf1.c tst/wf1.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$(T)yacc.s:	tst/yacc.c tst/yacc.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@

testclean:
	$(RM) $(T)8q$(E) $(T)8q.s $(T)8q.2 $(T)8q.1
	$(RM) $(T)array$(E) $(T)array.s $(T)array.2 $(T)array.1
	$(RM) $(T)cf$(E) $(T)cf.s $(T)cf.2 $(T)cf.1
	$(RM) $(T)cq$(E) $(T)cq.s $(T)cq.2 $(T)cq.1
	$(RM) $(T)cvt$(E) $(T)cvt.s $(T)cvt.2 $(T)cvt.1
	$(RM) $(T)fields$(E) $(T)fields.s $(T)fields.2 $(T)fields.1
	$(RM) $(T)front$(E) $(T)front.s $(T)front.2 $(T)front.1
	$(RM) $(T)incr$(E) $(T)incr.s $(T)incr.2 $(T)incr.1
	$(RM) $(T)init$(E) $(T)init.s $(T)init.2 $(T)init.1
	$(RM) $(T)limits$(E) $(T)limits.s $(T)limits.2 $(T)limits.1
	$(RM) $(T)paranoia$(E) $(T)paranoia.s $(T)paranoia.2 $(T)paranoia.1
	$(RM) $(T)sort$(E) $(T)sort.s $(T)sort.2 $(T)sort.1
	$(RM) $(T)spill$(E) $(T)spill.s $(T)spill.2 $(T)spill.1
	$(RM) $(T)stdarg$(E) $(T)stdarg.s $(T)stdarg.2 $(T)stdarg.1
	$(RM) $(T)struct$(E) $(T)struct.s $(T)struct.2 $(T)struct.1
	$(RM) $(T)switch$(E) $(T)switch.s $(T)switch.2 $(T)switch.1
	$(RM) $(T)wf1$(E) $(T)wf1.s $(T)wf1.2 $(T)wf1.1
	$(RM) $(T)yacc$(E) $(T)yacc.s $(T)yacc.2 $(T)yacc.1

install::	all
	$(INSTALL) -s -m 0755 $(BD)q3lcc$(E) ../
	$(INSTALL) -s -m 0755 $(BD)q3cpp$(E) ../
	$(INSTALL) -s -m 0755 $(BD)q3rcc$(E) ../

uninstall::
	-$(RM) ../q3lcc$(E)
	-$(RM) ../q3cpp$(E)
	-$(RM) ../q3rcc$(E)

clean::		testclean
		$(RM) $(BD)*$(O)
		$(RM) $(BD)dagcheck.c $(BD)alpha.c $(BD)mips.c $(BD)x86.c $(BD)sparc.c $(BD)x86linux.c $(BD)gram.c
		$(RM) $(BD)rcc.c $(BD)rcc.h
		$(RM) $(BD)rcc1$(E) $(BD)rcc1$(E) $(BD)1rcc$(E) $(BD)2rcc$(E)
		$(RM) $(BD)*.ilk
		$(RM) -r $(BD)

clobber::	clean
		$(RM) $(BD)q3rcc$(E) $(BD)2html$(E) $(BD)pass2$(E) $(BD)lburg$(E) $(BD)q3cpp$(E) $(BD)q3lcc$(E) $(BD)bprint$(E) $(BD)*$(A)
		$(RM) $(BD)*.pdb $(BD)*.pch

RCCSRCS=src/alloc.c \
	src/bind.c \
	src/dag.c \
	src/decl.c \
	src/enode.c \
	src/error.c \
	src/expr.c \
	src/event.c \
	src/init.c \
	src/inits.c \
	src/input.c \
	src/lex.c \
	src/list.c \
	src/main.c \
	src/output.c \
	src/prof.c \
	src/profio.c \
	src/simp.c \
	src/stmt.c \
	src/string.c \
	src/sym.c \
	src/trace.c \
	src/tree.c \
	src/types.c \
	src/null.c \
	src/symbolic.c \
	src/bytecode.c \
	src/gen.c \
	src/stab.c \
	$(BD)dagcheck.c \
	$(BD)alpha.c \
	$(BD)mips.c \
	$(BD)sparc.c \
	$(BD)x86linux.c \
	$(BD)x86.c

C=$(BD)q3lcc -A -d0.6 -Wo-lccdir=$(BUILDDIR) -Isrc -I$(BUILDDIR)
triple:	$(BD)q3rcc$(E) $(BD)q3lcc$(E) $(BD)q3cpp$(E)
	$C -o $(BD)1rcc$(E) -B$(BD)  $(RCCSRCS)
	$C -o $(BD)2rcc$(E) -B$(BD)1 $(RCCSRCS)
	strip $(BD)1rcc$(E) $(BD)2rcc$(E)
	dd if=$(BD)1rcc$(E) of=$(BD)rcc1$(E) bs=512 skip=1
	dd if=$(BD)2rcc$(E) of=$(BD)rcc2$(E) bs=512 skip=1
	if cmp $(BD)rcc1$(E) $(BD)rcc2$(E); then \
		mv $(BD)2rcc$(E) $(BD)q3rcc$(E); \
		$(RM) $(BD)1rcc$(E) $(BD)q3rcc[12]$(E); fi
