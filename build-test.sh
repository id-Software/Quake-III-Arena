#!/bin/sh

failed=0;

# Default Build
(make) || failed=1;

# Test additional options
(make USE_CODEC_VORBIS=1 USE_FREETYPE=1 CFLAGS=-DRAVENMD4) || failed=1;

# Test mingw
(exec ./cross-make-mingw.sh) || failed=1;

if [ $failed -eq 1 ]; then
  echo "Build failure.";
else
  echo "All builds successful.";
fi

exit $failed;

