#!/bin/bash

UNAME=`uname`
MASTER_DIR=`dirname $0`
BUILD_DEFAULT="release"

cd ${MASTER_DIR}

if [ "$OPTIONS" = "all_options" ];
then
    export USE_CODEC_VORBIS=1
    export USE_FREETYPE=1
fi

if [ "$UNAME" == "Darwin" ]; then
	CORES=`sysctl -n hw.ncpu`
elif [ "$UNAME" == "Linux" ]; then
	CORES=`awk '/^processor/ { N++} END { print N }' /proc/cpuinfo`
fi

echo "platform      : ${UNAME}"
echo "cores         : ${CORES}"
if [ "${BUILD_TYPE}" == "" ]; then
	BUILD_TYPE="${BUILD_DEFAULT}"
	echo "build type    : defaulting to ${BUILD_TYPE}"
else
	echo "build type    : ${BUILD_TYPE}"
fi

make -j${CORES} distclean ${BUILD_TYPE}

exit $?
