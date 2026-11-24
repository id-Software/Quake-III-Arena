#!/bin/bash
#
# Build the baseq3 game / cgame / ui QVMs from this source tree and pack them
# into run/baseq3/zz_vm_from_source.pk3.
#
# Needed on x86_64, where there is no JIT and no native 64-bit game DLLs: the
# QVMs shipped with old data (e.g. the original Q3A demo) are for an older
# engine API than this 1.32 source, so they have to be rebuilt from this tree
# with the tree's own lcc (bytecode compiler) and q3asm.
#
# usage: ./build_qvms.sh            (from code/unix; needs gcc, make, python3)
#
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
CODE=$(cd "$HERE/.." && pwd)
ROOT=$(cd "$CODE/.." && pwd)
WORK=${WORK:-$ROOT/build_qvm}          # scratch build directory
PK3=${PK3:-$ROOT/run/baseq3/zz_vm_from_source.pk3}
BIN=$WORK/bin
OUT=$WORK/out

mkdir -p "$BIN" "$OUT"

# ---- tools: lcc driver (q3lcc/q3rcc/q3cpp) and q3asm ------------------------
if [ ! -x "$BIN/q3lcc" ] || [ ! -x "$BIN/q3asm" ]; then
    echo "== building lcc and q3asm"
    rm -rf "$WORK/lcc" "$WORK/q3asm"
    cp -r "$ROOT/lcc" "$WORK/lcc"
    cp -r "$ROOT/q3asm" "$WORK/q3asm"
    mkdir -p "$WORK/lcc/out"
    # lburg/gram.c is shipped pre-generated; stop make from trying to run yacc
    touch "$WORK/lcc/lburg/gram.c"
    ( cd "$WORK/lcc" && make BUILDDIR="$WORK/lcc/out" CC=gcc LD=gcc \
        CFLAGS="-g -O0 -std=gnu89 -fcommon -w" HOSTFILE=etc/linux.c TEMPDIR=/tmp all >/dev/null )
    cp "$WORK/lcc/out/lcc" "$BIN/q3lcc"
    cp "$WORK/lcc/out/rcc" "$BIN/q3rcc"
    cp "$WORK/lcc/out/cpp" "$BIN/q3cpp"
    gcc -std=gnu89 -fcommon -w -O1 -o "$BIN/q3asm" "$WORK/q3asm/q3asm.c" "$WORK/q3asm/cmdlib.c"
fi
export PATH=$BIN:$PATH

# ---- modules ------------------------------------------------------------------
# build <name> <source dir> <.bat listing the sources> <.q3asm link list> <extra lcc defines> <ui include dir>
build() {
    name=$1; sd=$2; bat=$3; asmf=$4; defs=$5; uiinc=$6
    echo "== $name"
    rm -rf "$WORK/$name"; mkdir -p "$WORK/$name/vm"
    for src in $(grep -oE '\.\./[A-Za-z0-9_/.]+\.c' "$CODE/$sd/$bat" | sort -u); do
        ( cd "$WORK/$name/vm" && q3lcc -DQ3_VM $defs -S -Wf-target=bytecode -Wf-g \
            -I"$CODE/cgame" -I"$CODE/game" -I"$uiinc" "$(realpath -m "$CODE/$sd/vm/$src")" 2>&1 \
            | grep -v "warning:" ) || true
        [ -f "$WORK/$name/vm/$(basename "${src%.c}").asm" ] || { echo "FAILED to compile $src"; exit 1; }
    done
    sed -e 's#\\#/#g' -e "s#^-o .*#-o $OUT/$name#" "$CODE/$sd/$asmf" > "$WORK/$name/$name.q3asm"
    ln -sf "$CODE/$sd"/*.asm "$CODE"/ui/*.asm "$WORK/$name/" 2>/dev/null || true
    ( cd "$WORK/$name/vm" && q3asm -f "../$name" | tail -3 )
}

build qagame game   game.bat    game.q3asm    ""        "$CODE/ui"
build cgame  cgame  cgame.bat   cgame.q3asm   "-DCGAME" "$CODE/ui"
build ui     q3_ui  q3_ui.bat   q3_ui.q3asm   ""        "$CODE/q3_ui"

# ---- pack ------------------------------------------------------------------------
mkdir -p "$(dirname "$PK3")"
OUT="$OUT" PK3="$PK3" python3 -I - <<'EOF'
import os, zipfile
out, pk3 = os.environ["OUT"], os.environ["PK3"]
with zipfile.ZipFile(pk3, "w", zipfile.ZIP_DEFLATED) as z:
    for n in ("qagame", "cgame", "ui"):
        z.write(os.path.join(out, n + ".qvm"), "vm/%s.qvm" % n)
print("wrote", pk3)
EOF
