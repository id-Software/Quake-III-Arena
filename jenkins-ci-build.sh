#!/bin/bash

MASTER_DIR=`dirname $0`
cd ${MASTER_DIR}

if [ "$OPTIONS" = "all_options" ];
then
    export USE_CODEC_VORBIS=1
    export USE_FREETYPE=1
fi

if [ "$PLATFORM" = "mingw32" ];
then
	MAKE=./cross-make-mingw.sh
else
	MAKE=make
fi

CORES=`awk '/^processor/ { N++} END { print N }' /proc/cpuinfo`

# Default Build
($MAKE -j${CORES} clean ${BUILD_TYPE})
SUCCESS=$?

exit ${SUCCESS}
