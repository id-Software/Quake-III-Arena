#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SOURCE_DIR="$REPO_ROOT/build/legacy-opengl-source"
BUILD_DIR="$REPO_ROOT/build-opengl-linux"
UPSTREAM_URL="https://github.com/fknfilewalker/Quake-III-Arena-R.git"
PINNED_COMMIT="c95f4cecf7e0fec92e275d06645d5f2d14db286c"
BUILD_TYPE="${BUILD_TYPE:-Release}"

command -v git >/dev/null 2>&1 || { echo "git is required" >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "cmake is required" >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { echo "sha256sum is required" >&2; exit 1; }

mkdir -p "$REPO_ROOT/build"
if [ ! -d "$SOURCE_DIR/.git" ]; then
    rm -rf "$SOURCE_DIR"
    git clone --no-checkout "$UPSTREAM_URL" "$SOURCE_DIR"
fi

git -C "$SOURCE_DIR" fetch origin "$PINNED_COMMIT"
git -C "$SOURCE_DIR" checkout --detach "$PINNED_COMMIT"
git -C "$SOURCE_DIR" submodule update --init --recursive

cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel
(
    cd "$BUILD_DIR"
    find . -type f ! -name SHA256SUMS.txt -print0 | sort -z | xargs -0 -r sha256sum > SHA256SUMS.txt
)
echo "Generated SHA-256 manifest: $BUILD_DIR/SHA256SUMS.txt"
echo "Verify with: (cd \"$BUILD_DIR\" && sha256sum --check SHA256SUMS.txt)"
echo "OpenGL-only legacy build finished. Check $SOURCE_DIR/bin and $BUILD_DIR."
