#!/usr/bin/env bash
# Integration tests for the 'highway' package, outside the CRAN tarball.
#
#   dev/consumer-tests/run-all.sh [workdir]
#
# 1. builds and installs highway from this repository into a private library;
# 2. generates consumerA, consumerB (C++) and consumerC (C) from scratch with
#    use_highway(), copies consumerRcpp and consumerCpp11;
# 3. runs R CMD check --as-cran on each (incoming checks off: they are never
#    submitted) and fails unless Status is OK;
# 4. records each consumer's install time;
# 5. loads consumerA and consumerB together (normally and with RTLD_GLOBAL);
# 6. on Linux, checks that no Highway runtime symbols are exported.
#
# Uses only the given working directory (default: a new temporary directory).

set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
WORK=${1:-$(mktemp -d "${TMPDIR:-/tmp}/highway-consumers.XXXXXX")}
mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
# R on Windows needs Windows-style paths, not Git Bash ones.
rpath() { if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
LIB=$(rpath "$WORK/lib")
mkdir -p "$WORK/lib"
export R_LIBS="$LIB"
export _R_CHECK_CRAN_INCOMING_=FALSE
export _R_CHECK_CRAN_INCOMING_REMOTE_=FALSE
cd "$WORK"
echo "work directory: $WORK"

R CMD build --no-build-vignettes --no-manual "$(rpath "$ROOT")" > build-highway.log 2>&1
TARBALL=$(ls highway_*.tar.gz | tail -n 1)
R CMD INSTALL -l "$LIB" "$TARBALL" > install-highway.log 2>&1
Rscript -e 'library(highway); cat("highway", format(packageVersion("highway")), "\nsupported:", supported_targets(), "\ncompiled:", compiled_targets(), "\ndispatched:", dispatched_target(), "\n")'

for c in consumerA consumerB consumerC; do
  Rscript "$(rpath "$HERE/build-consumer.R")" "$c" "$(rpath "$WORK")" > "build-$c.log" 2>&1
done
for c in consumerRcpp consumerCpp11; do
  rm -rf "$c"
  cp -R "$HERE/$c" "$c"
done

status=0
printf '%-15s %-8s %s\n' consumer check "install seconds" > summary.txt
for c in consumerA consumerB consumerC consumerRcpp consumerCpp11; do
  R CMD build --no-manual "$c" > "rbuild-$c.log" 2>&1
  tb=$(ls ${c}_*.tar.gz | tail -n 1)
  start=$(date +%s)
  R CMD INSTALL -l "$LIB" "$tb" > "install-$c.log" 2>&1
  secs=$(( $(date +%s) - start ))
  if R CMD check --as-cran --no-manual "$tb" > "check-$c.log" 2>&1 &&
     grep -q '^Status: OK' "check-$c.log"; then
    result=OK
  else
    result=FAILED
    status=1
    tail -n 40 "check-$c.log"
  fi
  printf '%-15s %-8s %s\n' "$c" "$result" "$secs" >> summary.txt
  grep -h -E 'dispatched|static target|target .* OK' "$c.Rcheck/tests/"*.Rout 2>/dev/null |
    grep -v '^[>+]' || true
done

for mode in normal B-global A-global; do
  Rscript "$(rpath "$HERE/isolation-test.R")" "$mode" > "isolation-$mode.log" 2>&1 || {
    status=1; cat "isolation-$mode.log"; }
  tail -n 1 "isolation-$mode.log"
done

if [ "$(uname -s)" = Linux ]; then
  "$HERE/check-symbols.sh" "$LIB" consumerA consumerB consumerC consumerRcpp || status=1
fi

cat summary.txt
exit $status
