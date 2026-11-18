#!/usr/bin/env bash
# Builds and runs dev/sanitizers/harness.cpp under ThreadSanitizer and under
# AddressSanitizer + UndefinedBehaviorSanitizer. Any report fails the run.
#   CXX=clang++ dev/sanitizers/run.sh [outdir]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$(mktemp -d "${TMPDIR:-/tmp}/highway-san.XXXXXX")}
mkdir -p "$OUT"
CXX=${CXX:-clang++}
SRC="$ROOT/src/demo_kernels.cpp $ROOT/src/highway-impl.cpp $ROOT/dev/sanitizers/harness.cpp"
FLAGS="-std=gnu++17 -O1 -g -fno-omit-frame-pointer -pthread -I$ROOT/inst/include -I$ROOT/src"
export TSAN_OPTIONS="halt_on_error=1 second_deadlock_stack=1"
LEAKS=0; [ "$(uname -s)" = Linux ] && LEAKS=1  # LeakSanitizer: Linux only
export ASAN_OPTIONS="halt_on_error=1 detect_leaks=$LEAKS"
export UBSAN_OPTIONS="halt_on_error=1 print_stacktrace=1"
for san in thread address,undefined; do
  exe="$OUT/harness-${san/,/-}"
  echo "== $CXX -fsanitize=$san"
  # shellcheck disable=SC2086
  $CXX $FLAGS -fsanitize=$san -fno-sanitize-recover=all $SRC -o "$exe"
  "$exe"
done
