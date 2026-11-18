#!/usr/bin/env bash
# Compiles upstream Highway's own op and contrib tests (which exercise nearly
# every op on every target) against the bundled, patched headers with
#   -O2 -Wall -Wextra -pedantic
# and fails if any warning or error points into a file this package ships.
# Upstream sources come from the pinned tag in tools/upstream.dcf (SHA-256
# verified); GoogleTest headers from a pinned release.
#
#   CXX="g++-16" STD=gnu++20 JOBS=4 dev/header-warnings/run.sh [workdir]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${1:-$(mktemp -d "${TMPDIR:-/tmp}/highway-warn.XXXXXX")}
mkdir -p "$WORK"; WORK=$(cd "$WORK" && pwd)
CXX=${CXX:-c++}; STD=${STD:-gnu++20}; JOBS=${JOBS:-2}
GTEST_TAG=v1.18.0
GTEST_SHA=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
field() { sed -n "s/^$1: *//p" "$ROOT/tools/upstream.dcf"; }
sha256() { if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

cd "$WORK"
[ -s upstream.tar.gz ] || curl -fsSL -o upstream.tar.gz "$(field Tarball)"
[ "$(sha256 upstream.tar.gz)" = "$(field Tarball-SHA256)" ] || { echo "upstream SHA-256 mismatch"; exit 1; }
[ -s gtest.tar.gz ] || curl -fsSL -o gtest.tar.gz "https://github.com/google/googletest/archive/refs/tags/$GTEST_TAG.tar.gz"
[ "$(sha256 gtest.tar.gz)" = "$GTEST_SHA" ] || { echo "googletest SHA-256 mismatch"; exit 1; }
rm -rf src inc && mkdir -p src inc
tar -xzf upstream.tar.gz -C src --strip-components=1
tar -xzf gtest.tar.gz -C src
# Tree: upstream hwy/ (for tests and unbundled helpers), minus the SIMD back
# ends the package does not ship, overlaid with the shipped (patched) files.
cp -R src/hwy inc/hwy
rm -f inc/hwy/ops/ppc_vsx-inl.h inc/hwy/ops/rvv-inl.h inc/hwy/ops/loongarch_* inc/hwy/ops/wasm_*
cp -R "$ROOT/inst/include/hwy/." inc/hwy/
cp -R "$ROOT/inst/include/highway" inc/
cp -R src/googletest-*/googletest/include/gtest inc/
grep -v -e '^#' -e '^[[:space:]]*$' "$ROOT/tools/highway-files.txt" > shipped.txt

cd inc
{
  ls hwy/tests/*_test.cc
  ls hwy/*_test.cc | grep -v -e nanobenchmark -e perf_counters -e auto_tune -e profiler -e timer -e stats
  ls hwy/contrib/algo/*_test.cc hwy/contrib/bit_pack/*_test.cc hwy/contrib/dot/*_test.cc \
     hwy/contrib/image/*_test.cc hwy/contrib/math/*_test.cc hwy/contrib/unroller/*_test.cc
  printf '%s\n' hwy/targets.cc hwy/abort.cc hwy/per_target.cc hwy/aligned_allocator.cc hwy/contrib/image/image.cc
} > ../files.txt
mkdir -p ../logs
export CXX STD
# shellcheck disable=SC2016
xargs -P "$JOBS" -I{} sh -c 'f={}; b=$(echo "$f" | tr / _); $CXX -std=$STD -O2 -Wall -Wextra -pedantic -I. -DHWY_IS_TEST=1 -DNDEBUG -c "$f" -o /dev/null > ../logs/$b.log 2>&1 || echo "$f" >> ../failed.txt' < ../files.txt
cd ..
cat logs/*.log | grep -E '^(\./)?hwy/[^:]+:[0-9]+:[0-9]+: (warning|error):' | sed 's|^\./||' > diag.txt || true
awk -F: 'NR==FNR{s[$0]=1; next} ($1 in s)' shipped.txt diag.txt | sort | uniq -c | sort -rn > shipped-diag.txt
echo "$CXX -std=$STD: $(wc -l < files.txt) files compiled, $(wc -l < shipped-diag.txt | tr -d ' ') distinct diagnostics in shipped files"
status=0
if [ -s failed.txt ]; then echo "compile failures:"; cat failed.txt; for f in $(cat failed.txt); do tail -n 20 "logs/$(echo "$f" | tr / _).log"; done; status=1; fi
if [ -s shipped-diag.txt ]; then cat shipped-diag.txt; status=1; fi
exit $status
