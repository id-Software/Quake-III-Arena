#!/bin/sh

failed=0;

# Default Build
(make clean release) || failed=1;

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "Build successful.";
fi

exit $failed;

