#!/bin/sh

failed=0;

# check if testing mingw
if [ "$CC" = "i686-w64-mingw32-gcc" ]; then
	MAKE=./cross-make-mingw.sh
	haveExternalLibs=0
else
	MAKE=make
	haveExternalLibs=1
fi

# Default Build
($MAKE clean release) || failed=1;

# Test additional options
if [ $haveExternalLibs -eq 1 ]; then
	($MAKE clean release USE_CODEC_VORBIS=1 USE_FREETYPE=1 CFLAGS=-DRAVENMD4) || failed=1;
else
	($MAKE clean release CFLAGS=-DRAVENMD4) || failed=1;
fi

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "All builds successful.";
fi

exit $failed;

