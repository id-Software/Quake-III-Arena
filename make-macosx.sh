#!/bin/bash
#

# Let's make the user give us a target build system

if [ $# -ne 1 ]; then
	echo "Usage:   $0 target_architecture"
	echo "Example: $0 x86"
	echo "other valid options are x86_64 or ppc"
	echo
	echo "If you don't know or care about architectures please consider using make-macosx-ub.sh instead of this script."
	exit 1
fi

if [ "$1" == "x86" ]; then
	BUILDARCH=x86
	DARWIN_GCC_ARCH=i386
elif [ "$1" == "x86_64" ]; then
	BUILDARCH=x86_64
elif [ "$1" == "ppc" ]; then
	BUILDARCH=ppc
else
	echo "Invalid architecture: $1"
	echo "Valid architectures are x86, x86_64 or ppc"
	exit 1
fi

if [ -z "$DARWIN_GCC_ARCH" ]; then
	DARWIN_GCC_ARCH=${BUILDARCH}
fi

CC=gcc-4.0
DESTDIR=build/release-darwin-${BUILDARCH}

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

unset ARCH_SDK
unset ARCH_CFLAGS
unset ARCH_LDFLAGS

if [ -d /Developer/SDKs/MacOSX10.5.sdk ]; then
	ARCH_SDK=/Developer/SDKs/MacOSX10.5.sdk
	ARCH_CFLAGS="-arch ${DARWIN_GCC_ARCH} -isysroot /Developer/SDKs/MacOSX10.5.sdk \
			-DMAC_OS_X_VERSION_MIN_REQUIRED=1050"
	ARCH_LDFLAGS=" -mmacosx-version-min=10.5"
fi


echo "Building ${BUILDARCH} Client/Dedicated Server against \"$ARCH_SDK\""
sleep 3

if [ ! -d $DESTDIR ]; then
	mkdir -p $DESTDIR
fi

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`


# intel client and server
#if [ -d build/release-darwin-${BUILDARCH} ]; then
#	rm -r build/release-darwin-${BUILDARCH}
#fi
(ARCH=${BUILDARCH} CFLAGS=$ARCH_CFLAGS LDFLAGS=$ARCH_LDFLAGS make -j$NCPU) || exit 1;

# use the following shell script to build an application bundle
"./make-macosx-app.sh" release ${BUILDARCH}
