# $Id: makefile 145 2001-10-17 21:53:10Z timo $
HOSTFILE=etc/linux.c
TEMPDIR=/tmp
A=.a
O=.o
E=
CC=cc
CFLAGS=-g
LDFLAGS=
LD=cc
AR=ar ruv
RANLIB=ranlib
DIFF=diff
RM=rm -f
RMDIR=rmdir
TSTDIR=$(BUILDDIR)/$(TARGET)/tst
CUSTOM=custom.mk
include $(CUSTOM)
B=$(BUILDDIR)/
T=$(TSTDIR)/

# $Id: makefile 145 2001-10-17 21:53:10Z timo $

what:
	-@echo make all rcc lburg cpp lcc bprint liblcc triple clean clobber

all::	rcc lburg cpp lcc bprint liblcc

rcc:	$Brcc$E
lburg:	$Blburg$E
cpp:	$Bcpp$E
lcc:	$Blcc$E
bprint:	$Bbprint$E
liblcc:	$Bliblcc$A

RCCOBJS=$Balloc$O \
	$Bbind$O \
	$Bdag$O \
	$Bdagcheck$O \
	$Bdecl$O \
	$Benode$O \
	$Berror$O \
	$Bexpr$O \
	$Bevent$O \
	$Binit$O \
	$Binits$O \
	$Binput$O \
	$Blex$O \
	$Blist$O \
	$Bmain$O \
	$Boutput$O \
	$Bprof$O \
	$Bprofio$O \
	$Bsimp$O \
	$Bstmt$O \
	$Bstring$O \
	$Bsym$O \
	$Btrace$O \
	$Btree$O \
	$Btypes$O \
	$Bnull$O \
	$Bsymbolic$O \
	$Bgen$O \
	$Bbytecode$O \
	$Balpha$O \
	$Bmips$O \
	$Bsparc$O \
	$Bstab$O \
	$Bx86$O \
	$Bx86linux$O

$Brcc$E::	$Bmain$O $Blibrcc$A $(EXTRAOBJS)
		$(LD) $(LDFLAGS) -o $@ $Bmain$O $(EXTRAOBJS) $Blibrcc$A $(EXTRALIBS)

$Blibrcc$A:	$(RCCOBJS)
		$(AR) $@ $(RCCOBJS); $(RANLIB) $@ || true

$(RCCOBJS):	src/c.h src/token.h src/config.h

$Balloc$O:	src/alloc.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/alloc.c
$Bbind$O:	src/bind.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/bind.c
$Bdag$O:	src/dag.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/dag.c
$Bdecl$O:	src/decl.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/decl.c
$Benode$O:	src/enode.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/enode.c
$Berror$O:	src/error.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/error.c
$Bevent$O:	src/event.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/event.c
$Bexpr$O:	src/expr.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/expr.c
$Bgen$O:	src/gen.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/gen.c
$Binit$O:	src/init.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/init.c
$Binits$O:	src/inits.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/inits.c
$Binput$O:	src/input.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/input.c
$Blex$O:	src/lex.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/lex.c
$Blist$O:	src/list.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/list.c
$Bmain$O:	src/main.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/main.c
$Bnull$O:	src/null.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/null.c
$Boutput$O:	src/output.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/output.c
$Bprof$O:	src/prof.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/prof.c
$Bprofio$O:	src/profio.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/profio.c
$Bsimp$O:	src/simp.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/simp.c
$Bstmt$O:	src/stmt.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/stmt.c
$Bstring$O:	src/string.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/string.c
$Bsym$O:	src/sym.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/sym.c
$Bsymbolic$O:	src/symbolic.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/symbolic.c
$Bbytecode$O:	src/bytecode.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/bytecode.c
$Btrace$O:	src/trace.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/trace.c
$Btree$O:	src/tree.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/tree.c
$Btypes$O:	src/types.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/types.c
$Bstab$O:	src/stab.c src/stab.h;	$(CC) $(CFLAGS) -c -Isrc -o $@ src/stab.c

$Bdagcheck$O:	$Bdagcheck.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Bdagcheck.c
$Balpha$O:	$Balpha.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Balpha.c
$Bmips$O:	$Bmips.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Bmips.c
$Bsparc$O:	$Bsparc.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Bsparc.c
$Bx86$O:	$Bx86.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Bx86.c
$Bx86linux$O:	$Bx86linux.c;	$(CC) $(CFLAGS) -c -Isrc -o $@ $Bx86linux.c

$Bdagcheck.c:	$Blburg$E src/dagcheck.md; $Blburg src/dagcheck.md $@
$Balpha.c:	$Blburg$E src/alpha.md;    $Blburg src/alpha.md    $@
$Bmips.c:	$Blburg$E src/mips.md;     $Blburg src/mips.md     $@
$Bsparc.c:	$Blburg$E src/sparc.md;    $Blburg src/sparc.md    $@
$Bx86.c:	$Blburg$E src/x86.md;      $Blburg src/x86.md      $@
$Bx86linux.c:	$Blburg$E src/x86linux.md; $Blburg src/x86linux.md $@

$Brcc.h:	src/rcc.asdl;			$(ASDL_HOME)/bin/asdlGen --c -d $B src/rcc.asdl
$Brcc$O:	$Brcc.h;			$(CC) $(CFLAGS) -c -Isrc -I$B -I$(ASDL_HOME)/include/asdlGen -o $@ $Brcc.c
$Basdl$O:	src/asdl.c $Brcc.h src/c.h;	$(CC) $(CFLAGS) -c -Isrc -I$B -I$(ASDL_HOME)/include/asdlGen -o $@ src/asdl.c
$Bpass2$O:	src/pass2.c $Brcc.h src/c.h;	$(CC) $(CFLAGS) -c -Isrc -I$B -I$(ASDL_HOME)/include/asdlGen -o $@ src/pass2.c
$B2html$O:	src/2html.c $Brcc.h src/c.h;	$(CC) $(CFLAGS) -c -Isrc -I$B -I$(ASDL_HOME)/include/asdlGen -o $@ src/2html.c

$Bpass2$E:	$Bpass2$O $(EXTRAOBJS) $Blibrcc$A;	$(LD) $(LDFLAGS) -o $@ $Bpass2$O $(EXTRAOBJS) $Blibrcc$A $(EXTRALIBS)
$B2html$E:	$B2html$O $Brcc$O;			$(LD) $(LDFLAGS) -o $@ $B2html$O $Brcc$O $(EXTRALIBS)

$Bbprint$E:	$Bbprint$O;		$(LD) $(LDFLAGS) -o $@ $Bbprint$O 

$Bbprint$O:	etc/bprint.c;		$(CC) $(CFLAGS) -c -Isrc -o $@ etc/bprint.c

$Blcc$E:	$Blcc$O $Bhost$O;	$(LD) $(LDFLAGS) -o $@ $Blcc$O $Bhost$O 

SYSTEM=$(shell cc -print-search-dirs | head -n 1 | cut -b 10-)

$Blcc$O:	etc/lcc.c;	$(CC) $(CFLAGS) -c -DTEMPDIR=\"$(TEMPDIR)\" -o $@ etc/lcc.c
$Bhost$O:	$(HOSTFILE);	$(CC) $(CFLAGS) -c -DSYSTEM=\"$(SYSTEM)\" -o $@ $(HOSTFILE)

LIBOBJS=$Bassert$O $Bbbexit$O $Byynull$O

$Bliblcc$A:	$(LIBOBJS);	$(AR) $@ $Bassert$O $Bbbexit$O $Byynull$O; $(RANLIB) $@ || true

$Bassert$O:	lib/assert.c;	$(CC) $(CFLAGS) -c -o $@ lib/assert.c
$Byynull$O:	lib/yynull.c;	$(CC) $(CFLAGS) -c -o $@ lib/yynull.c
$Bbbexit$O:	lib/bbexit.c;	$(CC) $(CFLAGS) -c -o $@ lib/bbexit.c

$Blburg$E:	$Blburg$O $Bgram$O;	$(LD) $(LDFLAGS) -o $@ $Blburg$O $Bgram$O 

$Blburg$O $Bgram$O:	lburg/lburg.h

$Blburg$O:	lburg/lburg.c;	$(CC) $(CFLAGS) -c -Ilburg -o $@ lburg/lburg.c
$Bgram$O:	lburg/gram.c;	$(CC) $(CFLAGS) -c -Ilburg -o $@ lburg/gram.c

CPPOBJS=$Bcpp$O $Blexer$O $Bnlist$O $Btokens$O $Bmacro$O $Beval$O \
	$Binclude$O $Bhideset$O $Bgetopt$O $Bunix$O

$Bcpp$E:	$(CPPOBJS)
		$(LD) $(LDFLAGS) -o $@ $(CPPOBJS) 

$(CPPOBJS):	cpp/cpp.h

$Bcpp$O:	cpp/cpp.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/cpp.c
$Blexer$O:	cpp/lex.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/lex.c
$Bnlist$O:	cpp/nlist.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/nlist.c
$Btokens$O:	cpp/tokens.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/tokens.c
$Bmacro$O:	cpp/macro.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/macro.c
$Beval$O:	cpp/eval.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/eval.c
$Binclude$O:	cpp/include.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/include.c
$Bhideset$O:	cpp/hideset.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/hideset.c
$Bgetopt$O:	cpp/getopt.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/getopt.c
$Bunix$O:	cpp/unix.c;	$(CC) $(CFLAGS) -c -Icpp -o $@ cpp/unix.c

test:	$T8q.s \
	$Tarray.s \
	$Tcf.s \
	$Tcq.s \
	$Tcvt.s \
	$Tfields.s \
	$Tfront.s \
	$Tincr.s \
	$Tinit.s \
	$Tlimits.s \
	$Tparanoia.s \
	$Tsort.s \
	$Tspill.s \
	$Tstdarg.s \
	$Tstruct.s \
	$Tswitch.s \
	$Twf1.s \
	$Tyacc.s

$T8q.s:	tst/8q.c tst/8q.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tarray.s:	tst/array.c tst/array.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tcf.s:	tst/cf.c tst/cf.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tcq.s:	tst/cq.c tst/cq.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tcvt.s:	tst/cvt.c tst/cvt.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tfields.s:	tst/fields.c tst/fields.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tfront.s:	tst/front.c tst/front.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tincr.s:	tst/incr.c tst/incr.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tinit.s:	tst/init.c tst/init.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tlimits.s:	tst/limits.c tst/limits.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tparanoia.s:	tst/paranoia.c tst/paranoia.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tsort.s:	tst/sort.c tst/sort.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tspill.s:	tst/spill.c tst/spill.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tstdarg.s:	tst/stdarg.c tst/stdarg.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tstruct.s:	tst/struct.c tst/struct.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tswitch.s:	tst/switch.c tst/switch.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Twf1.s:	tst/wf1.c tst/wf1.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@
$Tyacc.s:	tst/yacc.c tst/yacc.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) src/run.sh $@

testclean:
	$(RM) $T8q$E $T8q.s $T8q.2 $T8q.1
	$(RM) $Tarray$E $Tarray.s $Tarray.2 $Tarray.1
	$(RM) $Tcf$E $Tcf.s $Tcf.2 $Tcf.1
	$(RM) $Tcq$E $Tcq.s $Tcq.2 $Tcq.1
	$(RM) $Tcvt$E $Tcvt.s $Tcvt.2 $Tcvt.1
	$(RM) $Tfields$E $Tfields.s $Tfields.2 $Tfields.1
	$(RM) $Tfront$E $Tfront.s $Tfront.2 $Tfront.1
	$(RM) $Tincr$E $Tincr.s $Tincr.2 $Tincr.1
	$(RM) $Tinit$E $Tinit.s $Tinit.2 $Tinit.1
	$(RM) $Tlimits$E $Tlimits.s $Tlimits.2 $Tlimits.1
	$(RM) $Tparanoia$E $Tparanoia.s $Tparanoia.2 $Tparanoia.1
	$(RM) $Tsort$E $Tsort.s $Tsort.2 $Tsort.1
	$(RM) $Tspill$E $Tspill.s $Tspill.2 $Tspill.1
	$(RM) $Tstdarg$E $Tstdarg.s $Tstdarg.2 $Tstdarg.1
	$(RM) $Tstruct$E $Tstruct.s $Tstruct.2 $Tstruct.1
	$(RM) $Tswitch$E $Tswitch.s $Tswitch.2 $Tswitch.1
	$(RM) $Twf1$E $Twf1.s $Twf1.2 $Twf1.1
	$(RM) $Tyacc$E $Tyacc.s $Tyacc.2 $Tyacc.1

install::	all
		install -d /usr/local/lib/lcc
		install -d /usr/local/lib/lcc/gcc
		install $Blcc$E /usr/local/bin
		install $Bcpp$E /usr/local/lib/lcc/gcc
		install $Brcc$E /usr/local/lib/lcc
		install $Bliblcc$A /usr/local/lib/lcc
		install $Blibrcc$A /usr/local/lib/lcc

uninstall::
		$(RM) /usr/local/lib/lcc/librcc$A
		$(RM) /usr/local/lib/lcc/liblcc$A
		$(RM) /usr/local/lib/lcc/rcc$E
		$(RM) /usr/local/lib/lcc/gcc/cpp$E
		$(RM) /usr/local/bin/lcc$E
		$(RMDIR) /usr/local/lib/lcc/gcc
		$(RMDIR) /usr/local/lib/lcc

clean::		testclean
		$(RM) $B*$O
		$(RM) $Bdagcheck.c $Balpha.c $Bmips.c $Bx86.c $Bsparc.c $Bx86linux.c $Bgram.c
		$(RM) $Brcc.c $Brcc.h
		$(RM) $Brcc1$E $Brcc1$E $B1rcc$E $B2rcc$E
		$(RM) $B*.ilk

clobber::	clean
		$(RM) $Brcc$E $B2html$E $Bpass2$E $Blburg$E $Bcpp$E $Blcc$E $Bbprint$E $B*$A
		$(RM) $B*.pdb $B*.pch

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
	$Bdagcheck.c \
	$Balpha.c \
	$Bmips.c \
	$Bsparc.c \
	$Bx86linux.c \
	$Bx86.c

C=$Blcc -A -d0.6 -Wo-lccdir=$(BUILDDIR) -Isrc -I$(BUILDDIR)
triple:	$Brcc$E $Blcc$E $Bcpp$E
	$C -o $B1rcc$E -B$B  $(RCCSRCS)
	$C -o $B2rcc$E -B$B1 $(RCCSRCS)
	strip $B1rcc$E $B2rcc$E
	dd if=$B1rcc$E of=$Brcc1$E bs=512 skip=1
	dd if=$B2rcc$E of=$Brcc2$E bs=512 skip=1
	if cmp $Brcc1$E $Brcc2$E; then \
		mv $B2rcc$E $Brcc$E; \
		$(RM) $B1rcc$E $Brcc[12]$E; fi
