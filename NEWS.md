# highway 1.4.0-1

* First release, bundling Google Highway 1.4.0 (commit `2607d3b`).
* Headers for `LinkingTo: highway`, with the upstream layout
  (`#include "hwy/highway.h"`), plus `highway/impl.h` (the dispatch runtime,
  compiled once per package), `highway/r_guard.h` (C++ exceptions to R errors
  at `.Call`) and `highway/c_guard.h` (C++ exceptions to status codes at C
  interfaces).
* `use_highway()` sets up a package, for C++ or C code.
* `highway_version()`, `supported_targets()`, `compiled_targets()` and
  `dispatched_target()` report on the bundled library and the CPU;
  `hwy_demo_exp()`, `hwy_demo_log()` and `hwy_demo_sum()` are demonstration
  kernels.
* Local changes to Highway (details in `inst/COPYRIGHTS`; patches in
  `tools/patches/`):
  - 0001: the default abort handler throws `highway::abort_error` instead of
    writing to `stderr` and calling `abort()`; the default warning handler is
    silent.
  - 0002: Highway's runtime symbols have hidden visibility, so packages that
    each compile the runtime cannot interfere with each other.
  - 0003: `highway/config.h` is included by `hwy/detect_targets.h`.
  - 0004, 0005, 0006: source fixes for `-pedantic` and `-Wignored-attributes`
    warnings.
  - 0007: Highway's diagnostic-suppression pragmas are compiled out (CRAN
    policy).
* Not bundled: tests, sorting (`vqsort`), the thread pool, the random number
  generator, benchmarking helpers, and the SIMD back ends for POWER, s390x,
  RISC-V, LoongArch and WebAssembly (those architectures use Highway's
  portable EMU128 target).
