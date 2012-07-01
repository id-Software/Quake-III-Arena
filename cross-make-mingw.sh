#!/bin/sh

# Note: This works in Linux and cygwin

CMD_PREFIX="i586-mingw32msvc i686-w64-mingw32";

if [ "X$CC" = "X" ]; then
    for check in $CMD_PREFIX; do
        full_check="${check}-gcc"
        which "$full_check" > /dev/null 2>&1
        if [ "$?" = "0" ]; then
            export CC="$full_check"
        fi
    done
fi

if [ "X$WINDRES" = "X" ]; then
    for check in $CMD_PREFIX; do
        full_check="${check}-windres"
        which "$full_check" > /dev/null 2>&1
        if [ "$?" = "0" ]; then
            export WINDRES="$full_check"
        fi
    done
fi

if [ "X$WINDRES" = "X" -o "X$CC" = "X" ]; then
    echo "Error: Must define or find WINDRES and CC"
    exit 1
fi

export PLATFORM=mingw32
export ARCH=x86

exec make $*
