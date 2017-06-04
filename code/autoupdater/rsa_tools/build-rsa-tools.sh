#!/bin/bash

# You don't need these to be built with the autoupdater, so here's a simple
#  shell file to make them on a Mac.

export TFMDIR="tomsfastmath-0.13.1"
export LTCDIR="libtomcrypt-1.17"

function build {
    if [ "$OSTYPE" = "Darwin" ]; then
        clang -mmacosx-version-min=10.7 -DMAC_OS_X_VERSION_MIN_REQUIRED=1070 -I $TFMDIR/src/headers -I $LTCDIR/src/headers -o "$1" -Wall -O3 "$1.c" rsa_common.c $LTCDIR/libtomcrypt.a $TFMDIR/libtfm.a
    else
        gcc -I $TFMDIR/src/headers -I $LTCDIR/src/headers -o "$1" -Wall -O3 "$1.c" rsa_common.c $LTCDIR/libtomcrypt.a $TFMDIR/libtfm.a
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

