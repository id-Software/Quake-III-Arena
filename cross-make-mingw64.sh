#!/bin/sh

if [ !$CC ]
then
  export CC=amd64-mingw32msvc-gcc
fi

if [ !$WINDRES ]
then
  export WINDRES=amd64-mingw32msvc-windres
fi

export PLATFORM=mingw32
export ARCH=x86_64

exec make $*
