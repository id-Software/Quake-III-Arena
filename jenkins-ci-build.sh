#!/bin/bash

MASTER_DIR=`dirname $0`
cd ${MASTER_DIR}

if [ "$OPTIONS" = "all_options" ];
then
    export USE_CODEC_VORBIS=1
    export USE_FREETYPE=1
fi

CORES=`awk '/^processor/ { N++} END { print N }' /proc/cpuinfo`

make -j${CORES} clean ${BUILD_TYPE}

exit $?
