#!/bin/bash

export TFMDIR="tomsfastmath-0.13.1"
export LTCDIR="libtomcrypt-1.17"

OSTYPE=`uname -s`
if [ -z "$CC" ]; then
    if [ "`uname -o`" = "Cygwin" ]; then
        export CC=/usr/bin/i686-w64-mingw32-gcc
    else
        export CC=cc
    fi
fi

function build {
    if [ "$OSTYPE" = "Darwin" ]; then
        $CC -mmacosx-version-min=10.7 -DMAC_OS_X_VERSION_MIN_REQUIRED=1070 -I $TFMDIR/src/headers -I $LTCDIR/src/headers -o "$1" -Wall -O3 "$1.c" rsa_common.c $LTCDIR/libtomcrypt.a $TFMDIR/libtfm.a
    else
        $CC -I $TFMDIR/src/headers -I $LTCDIR/src/headers -o "$1" -Wall -O3 "$1.c" rsa_common.c $LTCDIR/libtomcrypt.a $TFMDIR/libtfm.a
    fi
}

set -e
set -x

./build-libtom-unix.sh
build rsa_make_keys
build rsa_sign
build rsa_verify

set +x
echo "rsa_tools are compiled!"

