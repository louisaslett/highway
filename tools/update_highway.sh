#!/usr/bin/env bash
# Import a pinned, trimmed and patched copy of Google Highway into
# inst/include/hwy.
#
# Usage:
#   tools/update_highway.sh                 # re-import the tag in tools/upstream.dcf
#   tools/update_highway.sh --check         # verify inst/include/hwy is up to date
#   tools/update_highway.sh --tag 1.5.0     # move to a new upstream tag
#
# Steps:
#   1. download the upstream tag archive and verify the commit SHA (via
#      `git ls-remote` and the commit id embedded in the archive) and the
#      archive's SHA-256;
#   2. copy the files listed in tools/highway-files.txt;
#   3. apply tools/patches/*.patch in order, failing on any reject;
#   4. replace inst/include/hwy, regenerate tools/upstream.dcf and the
#      generated header of inst/COPYRIGHTS;
#   5. print a summary of what changed.
#
# Re-running at the same tag produces no changes. Needs: bash, curl, tar,
# patch, git, and sha256sum or shasum. Excluded from the package tarball.

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DCF="$ROOT/tools/upstream.dcf"
FILES="$ROOT/tools/highway-files.txt"
PATCHES="$ROOT/tools/patches"
DEST="$ROOT/inst/include/hwy"
COPYRIGHTS="$ROOT/inst/COPYRIGHTS"
CACHE="$ROOT/tools/.cache"

die() { echo "update_highway.sh: ERROR: $*" >&2; exit 1; }
field() { sed -n "s/^$1: *//p" "$DCF"; }

sha256() {
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
  else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

MODE=update
NEW_TAG=""
while [ $# -gt 0 ]; do
  case "$1" in
    --check) MODE=check; shift ;;
    --tag) NEW_TAG=${2:?--tag needs a value}; shift 2 ;;
    -h|--help) sed -n '2,22p' "$0"; exit 0 ;;
    *) die "unknown argument '$1'" ;;
  esac
done

URL=$(field URL)
TAG=$(field Tag)
COMMIT=$(field Commit)
TAR_SHA=$(field Tarball-SHA256)
[ -n "$URL" ] && [ -n "$TAG" ] && [ -n "$COMMIT" ] && [ -n "$TAR_SHA" ] ||
  die "tools/upstream.dcf is incomplete"

if [ -n "$NEW_TAG" ] && [ "$NEW_TAG" != "$TAG" ]; then
  [ "$MODE" = update ] || die "--check cannot be combined with a new --tag"
  echo "Moving from upstream tag $TAG to $NEW_TAG: commit and SHA-256 will be recorded anew."
  TAG=$NEW_TAG
  COMMIT=""
  TAR_SHA=""
fi

# --- 1. Resolve and verify the commit ---------------------------------------
REMOTE_COMMIT=$(git ls-remote "$URL" "refs/tags/$TAG^{}" "refs/tags/$TAG" |
  awk '/\^\{\}$/ {peeled=$1} {plain=$1} END {print (peeled != "" ? peeled : plain)}')
[ -n "$REMOTE_COMMIT" ] || die "tag $TAG not found at $URL"
if [ -n "$COMMIT" ] && [ "$REMOTE_COMMIT" != "$COMMIT" ]; then
  die "tag $TAG now points to $REMOTE_COMMIT, but $COMMIT is recorded (tag moved?)"
fi
COMMIT=$REMOTE_COMMIT

# --- 2. Download and verify the archive --------------------------------------
mkdir -p "$CACHE"
ARCHIVE="$CACHE/highway-$TAG.tar.gz"
TARBALL_URL="$URL/archive/refs/tags/$TAG.tar.gz"
if [ ! -s "$ARCHIVE" ]; then
  echo "Downloading $TARBALL_URL"
  curl -fsSL --retry 3 -o "$ARCHIVE.part" "$TARBALL_URL"
  mv "$ARCHIVE.part" "$ARCHIVE"
fi
GOT_SHA=$(sha256 "$ARCHIVE")
if [ -n "$TAR_SHA" ] && [ "$GOT_SHA" != "$TAR_SHA" ]; then
  die "archive SHA-256 $GOT_SHA does not match recorded $TAR_SHA"
fi
TAR_SHA=$GOT_SHA
EMBEDDED=$(gzip -dc "$ARCHIVE" | git get-tar-commit-id || true)
[ "$EMBEDDED" = "$COMMIT" ] ||
  die "archive was made from commit '$EMBEDDED', expected $COMMIT"

# --- 3. Copy the trimmed file list --------------------------------------------
WORK=$(mktemp -d "${TMPDIR:-/tmp}/highway-import.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
tar -xzf "$ARCHIVE" -C "$WORK"
SRC=$(find "$WORK" -mindepth 1 -maxdepth 1 -type d | head -n 1)
[ -d "$SRC/hwy" ] || die "unexpected archive layout"
OUT="$WORK/out"
mkdir -p "$OUT"
LIST=$(grep -v -e '^#' -e '^[[:space:]]*$' "$FILES")
for f in $LIST; do
  [ -f "$SRC/$f" ] || die "listed file '$f' does not exist upstream at $TAG"
  mkdir -p "$OUT/$(dirname "$f")"
  cp "$SRC/$f" "$OUT/$f"
done
NFILES=$(echo "$LIST" | wc -l | tr -d ' ')

# --- 4. Apply local patches -----------------------------------------------------
PATCH_LIST=""
for p in "$PATCHES"/*.patch; do
  [ -e "$p" ] || continue
  name=$(basename "$p")
  if ! patch -d "$OUT" -p1 --forward --batch --dry-run --silent < "$p" >/dev/null; then
    patch -d "$OUT" -p1 --forward --batch --dry-run < "$p" || true
    die "patch $name does not apply cleanly to upstream $TAG"
  fi
  patch -d "$OUT" -p1 --forward --batch --silent --no-backup-if-mismatch < "$p" >/dev/null
  PATCH_LIST="$PATCH_LIST $name"
done
find "$OUT" \( -name '*.orig' -o -name '*.rej' \) -print -quit | grep -q . &&
  die "patch left .orig/.rej files"
find "$OUT" -type f -exec chmod 0644 {} +

# --- 5. Install or check ----------------------------------------------------------
if [ "$MODE" = check ]; then
  if diff -r "$OUT/hwy" "$DEST" >/dev/null 2>&1; then
    echo "OK: inst/include/hwy matches upstream $TAG ($COMMIT) plus$PATCH_LIST"
    exit 0
  fi
  diff -r "$OUT/hwy" "$DEST" | head -n 40 >&2 || true
  die "inst/include/hwy differs from a fresh import; run tools/update_highway.sh"
fi

rm -rf "$DEST"
mkdir -p "$(dirname "$DEST")"
cp -R "$OUT/hwy" "$DEST"

cat > "$DCF" <<EOF
Component: Highway
URL: $URL
Tag: $TAG
Commit: $COMMIT
Tarball: $TARBALL_URL
Tarball-SHA256: $TAR_SHA
EOF

# Regenerate the block between the markers in inst/COPYRIGHTS.
if [ -f "$COPYRIGHTS" ]; then
  BEGIN='-- BEGIN generated by tools/update_highway.sh --'
  END='-- END generated by tools/update_highway.sh --'
  grep -qF -- "$BEGIN" "$COPYRIGHTS" || die "inst/COPYRIGHTS lacks the generated-block markers"
  BLOCK="$WORK/block.txt"
  {
    echo "$BEGIN"
    echo "Component:      Highway (Google)"
    echo "Upstream:       $URL"
    echo "Version/tag:    $TAG"
    echo "Commit:         $COMMIT"
    echo "Archive SHA256: $TAR_SHA"
    echo "Files bundled:  $NFILES (listed in tools/highway-files.txt of the"
    echo "                package sources), under inst/include/hwy/"
    echo "Local patches:"
    for p in $PATCH_LIST; do
      desc=$(sed -n 's/^# Subject: //p' "$PATCHES/$p" | head -n 1)
      echo "  $p"
      [ -n "$desc" ] && echo "      $desc"
    done
    echo "$END"
  } > "$BLOCK"
  awk -v begin="$BEGIN" -v end="$END" -v block="$BLOCK" '
    index($0, begin) == 1 { while ((getline line < block) > 0) print line; skip = 1; next }
    index($0, end) == 1 { skip = 0; next }
    !skip { print }
  ' "$COPYRIGHTS" > "$WORK/COPYRIGHTS.new"
  cp "$WORK/COPYRIGHTS.new" "$COPYRIGHTS"
fi

# Version fields: the package version must start with the upstream tag.
VERSION=$(sed -n 's/^Version: *//p' "$ROOT/DESCRIPTION" 2>/dev/null || true)
case "$VERSION" in
  "$TAG"-*) ;;
  *) echo
     echo "ACTION NEEDED: DESCRIPTION has 'Version: ${VERSION:-<none>}'."
     echo "  Set 'Version: $TAG-1' and add a NEWS.md entry for Highway $TAG." ;;
esac

echo
echo "Imported Highway $TAG ($COMMIT): $NFILES files, patches:$PATCH_LIST"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "Changes relative to git HEAD:"
  git -C "$ROOT" status --short -- inst/include/hwy tools/upstream.dcf inst/COPYRIGHTS
  git -C "$ROOT" diff --stat -- inst/include/hwy tools/upstream.dcf inst/COPYRIGHTS | tail -n 1
fi
