#!/bin/sh

export CC=i586-mingw32msvc-gcc
export WINDRES=i586-mingw32msvc-windres
export PLATFORM=mingw32
if [ !$ARCH ]
then
export ARCH=x86
fi
exec make $*
