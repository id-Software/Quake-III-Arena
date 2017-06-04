#!/bin/bash

TFMVER=0.13.1
LTCVER=1.17
set -e

OSTYPE=`uname -s`
if [ "$OSTYPE" = "Linux" ]; then
    NCPU=`cat /proc/cpuinfo |grep vendor_id |wc -l`
    let NCPU=$NCPU+1
elif [ "$OSTYPE" = "Darwin" ]; then
    NCPU=`sysctl -n hw.ncpu`
elif [ "$OSTYPE" = "SunOS" ]; then
    NCPU=`/usr/sbin/psrinfo |wc -l |sed -e 's/^ *//g;s/ *$//g'`
else
    NCPU=1
fi

if [ -z "$NCPU" ]; then
    NCPU=1
elif [ "$NCPU" = "0" ]; then
    NCPU=1
fi

if [ ! -f ./crypt-$LTCVER.tar.bz2 ]; then
    echo "Downloading LibTomCrypt $LTCVER sources..."
    curl -L -o crypt-$LTCVER.tar.bz2 https://github.com/libtom/libtomcrypt/releases/download/$LTCVER/crypt-$LTCVER.tar.bz2 || exit 1
fi

if [ ! -f tfm-$TFMVER.tar.xz ]; then
    echo "Downloading TomsFastMath $TFMVER sources..."
    curl -L -o tfm-$TFMVER.tar.xz https://github.com/libtom/tomsfastmath/releases/download/v$TFMVER/tfm-$TFMVER.tar.xz || exit 1
fi

if [ ! -d tomsfastmath-$TFMVER ]; then
    echo "Unpacking TomsFastMath $TFMVER sources..."
    tar -xJvvf ./tfm-$TFMVER.tar.xz
fi

if [ ! -d libtomcrypt-$LTCVER ]; then
    echo "Unpacking LibTomCrypt $LTCVER sources..."
    tar -xjvvf ./crypt-$LTCVER.tar.bz2
fi

echo
echo
echo "Will use make -j$NCPU. If this is wrong, check NCPU at top of script."
echo
echo

set -e
set -x

# Some compilers can't handle the ROLC inline asm; just turn it off.
cd tomsfastmath-$TFMVER
make -j$NCPU
cd ..

export CFLAGS="$CFLAGS -DTFM_DESC -DLTC_NO_ROLC -I ../tomsfastmath-$TFMVER/src/headers"
cd libtomcrypt-$LTCVER
make -j$NCPU
cd ..

set +x
echo "All done."

# end of build-libtom-unix.sh ...

