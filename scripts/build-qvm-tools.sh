#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
LCC_DIR="$REPO_ROOT/qvmtools/q3lcc"
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

if ! command -v git >/dev/null 2>&1; then
    echo "git is required" >&2
    exit 1
fi
if ! command -v make >/dev/null 2>&1; then
    echo "make is required" >&2
    exit 1
fi

git -C "$REPO_ROOT" submodule update --init --recursive -- qvmtools/q3lcc
if [ ! -f "$LCC_DIR/Makefile" ]; then
    echo "q3lcc submodule is missing or incomplete" >&2
    exit 1
fi

EXPECTED="814e3808346aba20f994fa83fca4f9d37124c67c"
ACTUAL=$(git -C "$LCC_DIR" rev-parse HEAD)
if [ "$ACTUAL" != "$EXPECTED" ]; then
    echo "Unexpected q3lcc revision: $ACTUAL (expected $EXPECTED)" >&2
    exit 1
fi

make -C "$LCC_DIR" clean PLATFORM="$PLATFORM" ARCH="$ARCH"
make -C "$LCC_DIR" all PLATFORM="$PLATFORM" ARCH="$ARCH"

OUT="$LCC_DIR/build-$PLATFORM-$ARCH"
for tool in q3lcc q3cpp q3rcc; do
    if [ ! -x "$OUT/$tool" ]; then
        echo "Expected QVM tool was not produced: $OUT/$tool" >&2
        exit 1
    fi
done

printf '\nQVM compiler tools built at pinned LCC 4.2 revision %s\n' "$EXPECTED"
printf 'Output directory: %s\n' "$OUT"
printf 'Assembler remains the repository q3asm tool; LCC output compatibility must be checked with the QVM regression suite.\n'
