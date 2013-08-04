#!/bin/bash
CC=gcc-4.0

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory"
	exit 1
fi

# we want to use the oldest available SDK for max compatiblity. However 10.4 and older
# can not build 64bit binaries, making 10.5 the minimum version.   This has been tested 
# with xcode 3.1 (xcode31_2199_developerdvd.dmg).  It contains the 10.5 SDK and a decent
# enough gcc to actually compile ioquake3
# For PPC macs, G4's or better are required to run ioquake3.

unset X86_64_SDK
unset X86_64_CFLAGS
unset X86_64_LDFLAGS
unset X86_SDK
unset X86_CFLAGS
unset X86_LDFLAGS
unset PPC_64_SDK
unset PPC_CFLAGS
unset PPC_LDFLAGS

if [ -d /Developer/SDKs/MacOSX10.5.sdk ]; then
	X86_64_SDK=/Developer/SDKs/MacOSX10.5.sdk
	X86_64_CFLAGS="-arch x86_64 -isysroot /Developer/SDKs/MacOSX10.5.sdk \
			-DMAC_OS_X_VERSION_MIN_REQUIRED=1050"
	X86_64_LDFLAGS=" -mmacosx-version-min=10.5"

	X86_SDK=/Developer/SDKs/MacOSX10.5.sdk
	X86_CFLAGS="-arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk \
			-DMAC_OS_X_VERSION_MIN_REQUIRED=1050"
	X86_LDFLAGS=" -mmacosx-version-min=10.5"

	PPC_SDK=/Developer/SDKs/MacOSX10.5.sdk
	PPC_CFLAGS="-arch ppc -isysroot /Developer/SDKs/MacOSX10.5.sdk \
			-DMAC_OS_X_VERSION_MIN_REQUIRED=1050"
	PPC_LDFLAGS=" -mmacosx-version-min=10.5"
fi

if [ -z $X86_64_SDK ] || [ -z $X86_SDK ] || [ -z $PPC_SDK ]; then
	echo "\
ERROR: This script is for building a Universal Binary.  You cannot build
       for a different architecture unless you have the proper Mac OS X SDKs
       installed.  If you just want to to compile for your own system run
       'make-macosx.sh' instead of this script."
	exit 1
fi

echo "Building X86_64 Client/Dedicated Server against \"$X86_64_SDK\""
echo "Building X86 Client/Dedicated Server against \"$X86_SDK\""
echo "Building PPC Client/Dedicated Server against \"$PPC_SDK\""
echo

if [ "$X86_64_SDK" != "/Developer/SDKs/MacOSX10.5.sdk" ] || \
        [ "$X86_SDK" != "/Developer/SDKs/MacOSX10.5.sdk" ]; then
	echo "\
WARNING: in order to build a binary with maximum compatibility you must
         build on Mac OS X 10.5 using Xcode 3.1 and have the MacOSX10.5
         SDKs installed from the Xcode install disk Packages folder."
sleep 3
fi

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`

# x86_64 client and server
#if [ -d build/release-release-x86_64 ]; then
#	rm -r build/release-darwin-x86_64
#fi
(ARCH=x86_64 CC=gcc-4.0 CFLAGS=$X86_64_CFLAGS LDFLAGS=$X86_64_LDFLAGS make -j$NCPU) || exit 1;

echo;echo

# x86 client and server
#if [ -d build/release-darwin-x86 ]; then
#	rm -r build/release-darwin-x86
#fi
(ARCH=x86 CC=gcc-4.0 CFLAGS=$X86_CFLAGS LDFLAGS=$X86_LDFLAGS make -j$NCPU) || exit 1;

echo;echo

# PPC client and server
#if [ -d build/release-darwin-ppc ]; then
#	rm -r build/release-darwin-ppc
#fi
(ARCH=ppc CC=gcc-4.0 CFLAGS=$PPC_CFLAGS LDFLAGS=$PPC_LDFLAGS make -j$NCPU) || exit 1;

echo

# use the following shell script to build a universal application bundle
"./make-macosx-app.sh" release
