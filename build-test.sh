#!/bin/sh

failed=0;

# Default Build
(make clean release) || failed=1;

# Test additional options
(make clean release USE_CODEC_VORBIS=1 USE_FREETYPE=1 CFLAGS=-DRAVENMD4) || failed=1;

# Test mingw
if [ "$CC" = "clang" ]; then
	# skip mingw if travis-ci clang build
	echo "Skipping mingw build because there is no mingw clang compiler available.";
else
	# clear CC so cross-make-mingw script will set it.
	export CC=
	(exec ./cross-make-mingw.sh clean release) || failed=1;
fi

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "All builds successful.";
fi

exit $failed;

