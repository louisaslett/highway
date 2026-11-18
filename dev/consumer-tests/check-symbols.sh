#!/usr/bin/env bash
# Checks the dynamic symbol tables of installed consumer packages (Linux, ELF).
# Highway's runtime (functions compiled from highway/impl.h) and any Highway
# data object must not be exported, or two packages loaded with RTLD_GLOBAL
# could bind to each other's copies. Exported inline functions from headers
# (stateless code, identical in every copy) are listed but allowed.
#
#   check-symbols.sh <library> <package>...
set -euo pipefail
LIB=$1
shift
status=0
for pkg in "$@"; do
  so="$LIB/$pkg/libs/$pkg.so"
  [ -f "$so" ] || { echo "missing $so"; status=1; continue; }
  # Columns: value size type bind vis ndx name
  syms=$(readelf --dyn-syms --wide -C "$so" | awk '$7 != "UND" && $8 ~ /hwy::|highway::/')
  data=$(echo "$syms" | awk '$4 == "OBJECT" || $4 == "TLS"' | grep . || true)
  runtime=$(echo "$syms" | grep -E 'hwy::(SupportedTargets|GetChosenTarget|DisableTargets|SetSupportedTargetsForTest|Abort|Warn|SetAbortFunc|SetWarnFunc|GetAbortFunc|GetWarnFunc|VectorBytes|DispatchedTarget|Have(Integer64|Float16|Float64)|AllocateAlignedBytes|FreeAlignedBytes)\(' || true)
  n=$(echo "$syms" | grep -c . || true)
  echo "$pkg: $n exported hwy::/highway:: symbols (inline header code allowed)"
  if [ -n "$data" ] || [ -n "$runtime" ]; then
    echo "$pkg: FAIL: exported Highway runtime state or functions:"
    echo "$data"
    echo "$runtime"
    status=1
  fi
done
exit $status
