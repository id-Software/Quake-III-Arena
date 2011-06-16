#!/bin/sh

export CC=amd64-mingw32msvc-gcc
export WINDRES=amd64-mingw32msvc-windres
export PLATFORM=mingw32
if [ !$ARCH ]
then
export ARCH=x86_64
fi
exec make $*
