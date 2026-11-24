#!/bin/bash
# Launch the 64-bit SDL2 client from the repo's run/ directory.
# extra arguments are passed to the game, e.g.:  ./run_q3.sh +set r_fullscreen 0 +map q3dm1
HERE=$(cd "$(dirname "$0")" && pwd)
BIN=$HERE/releasex86_64/linuxquake3
[ -x "$BIN" ] || { echo "build first: (cd $HERE && make release)"; exit 1; }
exec "$BIN" +set fs_basepath "$HERE/../../run" +set sv_pure 0 "$@"
