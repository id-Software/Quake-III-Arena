#!/bin/sh
# run .../target/os/tst/foo.s [ remotehost ]

# set -x
target=`echo $1 | awk -F/ '{ print $(NF-3) }'`
os=`echo $1 | awk -F/ '{ print $(NF-2) }'`
dir=$target/$os

case "$1" in
*symbolic/irix*)	idir=include/mips/irix; remotehost=noexecute ;;
*symbolic/osf*)		idir=include/alpha/osf;	remotehost=noexecute ;;
*)			idir=include/$dir;      remotehost=${2-$REMOTEHOST} ;;
esac

if [ ! -d "$target/$os" -o ! -d "$idir" ]; then
	echo 2>&1 $0: unknown combination '"'$target/$os'"'
	exit 1
fi

C=`basename $1 .s`
BUILDDIR=${BUILDDIR-.} LCC="${LCC-${BUILDDIR}/lcc} -Wo-lccdir=$BUILDDIR"
TSTDIR=${TSTDIR-${BUILDDIR}/$dir/tst}
if [ ! -d $TSTDIR ]; then mkdir -p $TSTDIR; fi

echo ${BUILDDIR}/rcc$EXE -target=$target/$os $1: 1>&2
$LCC -S -I$idir -Ualpha -Usun -Uvax -Umips -Ux86 \
	-Wf-errout=$TSTDIR/$C.2 -D$target -Wf-g0 \
	-Wf-target=$target/$os -o $1 tst/$C.c
if [ $? != 0 ]; then remotehost=noexecute; fi
if [ -r $dir/tst/$C.2bk ]; then
	diff $dir/tst/$C.2bk $TSTDIR/$C.2
fi
if [ -r $dir/tst/$C.sbk ]; then
	if diff $dir/tst/$C.sbk $TSTDIR/$C.s; then exit 0; fi
fi

case "$remotehost" in
noexecute)	exit 0 ;;
""|"-")	$LCC -o $TSTDIR/$C$EXE $1; $TSTDIR/$C$EXE <tst/$C.0 >$TSTDIR/$C.1 ;;
*)	rcp $1 $remotehost:
	if expr "$remotehost" : '.*@' >/dev/null ; then
		remotehost="`expr $remotehost : '.*@\(.*\)'` -l `expr $remotehost : '\(.*\)@'`"
	fi
	rsh $remotehost "cc -o $C$EXE $C.s -lm;./$C$EXE;rm -f $C$EXE $C.[so]" <tst/$C.0 >$TSTDIR/$C.1
	;;
esac
if [ -r $dir/tst/$C.1bk ]; then
	diff $dir/tst/$C.1bk $TSTDIR/$C.1
	exit $?
fi
exit 0
