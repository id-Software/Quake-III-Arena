#!/bin/bash
CC=gcc-4.0

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory"
	exit 1
fi

# This script is to build a Universal 2 binary
# (Apple's term for an x86_64 and arm64 binary)

unset X86_64_SDK
unset X86_64_CFLAGS
unset X86_64_MACOSX_VERSION_MIN
unset ARM64_SDK
unset ARM64_CFLAGS
unset ARM64_MACOSX_VERSION_MIN

X86_64_MACOSX_VERSION_MIN="10.7"
ARM64_MACOSX_VERSION_MIN="11.0"

echo "Building X86_64 Client/Dedicated Server"
echo "Building ARM64 Client/Dedicated Server"
echo

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`

# x86_64 client and server
#if [ -d build/release-release-x86_64 ]; then
#	rm -r build/release-darwin-x86_64
#fi
(ARCH=x86_64 CFLAGS=$X86_64_CFLAGS MACOSX_VERSION_MIN=$X86_64_MACOSX_VERSION_MIN make -j$NCPU) || exit 1;

echo;echo

# arm64 client and server
#if [ -d build/release-release-arm64 ]; then
#	rm -r build/release-darwin-arm64
#fi
(ARCH=arm64 CFLAGS=$ARM64_CFLAGS MACOSX_VERSION_MIN=$ARM64_MACOSX_VERSION_MIN make -j$NCPU) || exit 1;

echo

# use the following shell script to build a universal 2 application bundle
export MACOSX_DEPLOYMENT_TARGET="10.7"
export MACOSX_DEPLOYMENT_TARGET_X86_64="$X86_64_MACOSX_VERSION_MIN"
export MACOSX_DEPLOYMENT_TARGET_ARM64="$ARM64_MACOSX_VERSION_MIN"
"./make-macosx-app.sh" release
