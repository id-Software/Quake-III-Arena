#!/bin/sh

failed=0;

# Default Build
(make) || failed=1;

# Test additional options
make clean
(make USE_CODEC_VORBIS=1 USE_FREETYPE=1 CFLAGS=-DRAVENMD4) || failed=1;

# Test mingw (gcc)
if [ "$CC" = "gcc" ]; then
	# clear CC so script will set it.
    export CC=
    (exec ./cross-make-mingw.sh) || failed=1;
fi

if [ $failed -eq 1 ]; then
  echo "Build failure.";
else
  echo "All builds successful.";
fi

exit $failed;

