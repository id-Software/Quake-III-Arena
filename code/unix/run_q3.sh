#!/bin/bash
# Launch the SDL2 client from the repo's run/ directory.
# Extra arguments are passed to the game, e.g.:
#   ./run_q3.sh +set r_fullscreen 0 +devmap q3dm1
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

# newest build for this OS (built with: make -f Makefile.sdl)
OS=$(uname -s | tr 'A-Z' 'a-z')
BIN=
for cand in "$ROOT"/build/$OS-*-release/quake3 "$ROOT"/build/$OS-*-debug/quake3; do
    [ -x "$cand" ] && { BIN=$cand; break; }
done
[ -n "$BIN" ] || { echo "no build found; run: (cd $HERE && make -f Makefile.sdl)"; exit 1; }

exec "$BIN" +set fs_basepath "$ROOT/run" +set sv_pure 0 "$@"
