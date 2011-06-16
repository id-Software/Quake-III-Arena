#!/bin/sh

if [ !$CC ]
then
  export CC=i586-mingw32msvc-gcc
fi

if [ !$WINDRES ]
then
  export WINDRES=i586-mingw32msvc-windres
fi

export PLATFORM=mingw32
export ARCH=x86

exec make $*
