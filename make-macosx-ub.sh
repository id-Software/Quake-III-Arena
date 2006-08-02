#!/bin/sh

DESTDIR=build/release-darwin-ub
BASEDIR=baseq3
MPACKDIR=missionpack

BIN_OBJ="
	build/release-darwin-ppc/ioquake3.ppc
	build/release-darwin-i386/ioquake3.i386
"
BASE_OBJ="
	build/release-darwin-ppc/$BASEDIR/cgameppc.dylib
	build/release-darwin-i386/$BASEDIR/cgamei386.dylib
	build/release-darwin-ppc/$BASEDIR/uippc.dylib
	build/release-darwin-i386/$BASEDIR/uii386.dylib
	build/release-darwin-ppc/$BASEDIR/qagameppc.dylib
	build/release-darwin-i386/$BASEDIR/qagamei386.dylib
"
MPACK_OBJ="
	build/release-darwin-ppc/$MPACKDIR/cgameppc.dylib
	build/release-darwin-i386/$MPACKDIR/cgamei386.dylib
	build/release-darwin-ppc/$MPACKDIR/uippc.dylib
	build/release-darwin-i386/$MPACKDIR/uii386.dylib
	build/release-darwin-ppc/$MPACKDIR/qagameppc.dylib
	build/release-darwin-i386/$MPACKDIR/qagamei386.dylib
"
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory";
fi

if [ ! -d /Developer/SDKs/MacOSX10.2.8.sdk ]; then
	echo "
/Developer/SDKs/MacOSX10.2.8.sdk/ is missing.
The installer for this SDK is included with XCode 2.2 or newer"
	exit 1;
fi

if [ ! -d /Developer/SDKs/MacOSX10.4u.sdk ]; then
	echo "
/Developer/SDKs/MacOSX10.4u.sdk/ is missing.   
The installer for this SDK is included with XCode 2.2 or newer"
	exit 1;
fi

(BUILD_MACOSX_UB=ppc make && BUILD_MACOSX_UB=i386 make) || exit 1;

if [ ! -d $DESTDIR ]; then 
	mkdir $DESTDIR || exit 1;
fi
if [ ! -d $DESTDIR/$BASEDIR ]; then
	mkdir $DESTDIR/$BASEDIR || exit 1;
fi
if [ ! -d $DESTDIR/$MPACKDIR ]; then
	mkdir $DESTDIR/$MPACKDIR || exit 1;
fi

echo "Installing Universal Binaries in $DESTDIR"
lipo -create -o $DESTDIR/ioquake3.ub $BIN_OBJ
cp $BASE_OBJ $DESTDIR/$BASEDIR/
cp $MPACK_OBJ $DESTDIR/$MPACKDIR/
cp code/libs/macosx/*.dylib $DESTDIR/

