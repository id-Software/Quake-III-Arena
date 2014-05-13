#!/bin/sh

failed=0;

# check if testing mingw
if [ "$CC" = "i686-w64-mingw32-gcc" ]; then
	export PLATFORM=mingw32
	export ARCH=x86
	export CC=
	haveExternalLibs=0
elif [ "$CC" = "x86_64-w64-mingw32-gcc" ]; then
	export PLATFORM=mingw32
	export ARCH=x86_64
	export CC=
	haveExternalLibs=0
else
	haveExternalLibs=1
fi

# Default Build
(make clean release) || failed=1;

# Test additional options
if [ $haveExternalLibs -eq 1 ]; then
	(make clean release USE_CODEC_VORBIS=1 USE_FREETYPE=1) || failed=1;
fi

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "All builds successful.";
fi

exit $failed;

