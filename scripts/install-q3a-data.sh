#!/usr/bin/env sh
# Stage legally obtained Quake III Arena retail data into a game directory.
# Commercial PK3 archives are never downloaded or committed by this script.
set -eu

usage() {
  echo "Usage: $0 <source-data-root> <game-install-root>" >&2
  echo "Expected source layout: <source>/baseq3/pak0.pk3 ... pak8.pk3" >&2
  echo "Optional: <source>/missionpack/pak*.pk3" >&2
  exit 2
}
[ "$#" -eq 2 ] || usage
SRC=$(CDPATH= cd -- "$1" 2>/dev/null && pwd) || { echo "Source directory not found: $1" >&2; exit 1; }
DEST=$2
mkdir -p "$DEST/baseq3"
DEST=$(CDPATH= cd -- "$DEST" && pwd)
PYTHON=$(command -v python3 || true)
[ -n "$PYTHON" ] || { echo "python3 is required to validate PK3 archives." >&2; exit 1; }

validate_pk3() {
  "$PYTHON" - "$1" <<'PY'
import sys, zipfile
p = sys.argv[1]
try:
    with zipfile.ZipFile(p, "r") as z:
        bad = z.testzip()
        if bad:
            raise SystemExit("Corrupt member: " + bad)
        if not z.infolist():
            raise SystemExit("Archive is empty")
except (OSError, zipfile.BadZipFile) as e:
    raise SystemExit(str(e))
PY
}

for n in 0 1 2 3 4 5 6 7 8; do
  file="$SRC/baseq3/pak$n.pk3"
  [ -f "$file" ] || { echo "Missing required retail/1.32 data archive: $file" >&2; exit 1; }
  validate_pk3 "$file" || { echo "Invalid PK3: $file" >&2; exit 1; }
done

mkdir -p "$DEST/baseq3"
for n in 0 1 2 3 4 5 6 7 8; do
  cp -f "$SRC/baseq3/pak$n.pk3" "$DEST/baseq3/pak$n.pk3"
  echo "Installed baseq3/pak$n.pk3"
done

if [ -d "$SRC/missionpack" ]; then
  set -- "$SRC"/missionpack/pak*.pk3
  if [ -f "$1" ]; then
    mkdir -p "$DEST/missionpack"
    for file do
      validate_pk3 "$file" || { echo "Invalid PK3: $file" >&2; exit 1; }
      cp -f "$file" "$DEST/missionpack/"
      echo "Installed missionpack/$(basename "$file")"
    done
  fi
fi

echo "Quake III Arena data staged at: $DEST"
echo "Base game data validated. Team Arena is included only if missionpack PK3 files were supplied."
