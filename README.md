# highway: Google Highway SIMD for R packages

<!-- badges: start -->
<!-- After CRAN acceptance, link this badge to https://CRAN.R-project.org/package=highway -->
![CRAN status](https://www.r-pkg.org/badges/version/highway)
[![R-CMD-check](https://github.com/louisaslett/highway/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/louisaslett/highway/actions/workflows/R-CMD-check.yaml)
[![consumers](https://github.com/louisaslett/highway/actions/workflows/consumers.yaml/badge.svg)](https://github.com/louisaslett/highway/actions/workflows/consumers.yaml)
[![R-hub](https://github.com/louisaslett/highway/actions/workflows/rhub.yaml/badge.svg)](https://github.com/louisaslett/highway/actions/workflows/rhub.yaml)
<!-- badges: end -->

The **highway** package bundles the headers of
[Highway](https://github.com/google/highway), Google's C++ library for
portable SIMD (single instruction, multiple data) programming, so that R
packages can use it through `LinkingTo: highway`.

Kernels are written in C++, and Highway compiles them for several instruction
sets (SSE2, SSE4, AVX2 and AVX-512 on x86-64; NEON and SVE on arm64). Then, at
run time the best one supported by the user's CPU is automatically used. This
enables binary packages built by CRAN to get AVX2 or AVX-512 speed where the
hardware has it, without any CPU-specific compiler flags that would result in
rejection by CRAN.

## Quick start

1. In your package, run

   ```r
   highway::use_highway()          # or use_highway(language = "c")
   ```

   This adds `highway` to `LinkingTo`, creates `src/highway-impl.cpp` (one
   line: `#include "highway/impl.h"`), adds `-I.` to `src/Makevars` and
   `src/Makevars.win`, and writes example kernels to
   `src/highway-kernels.cpp`.

2. Write kernels with Highway's API (see the example file and the vignette):

   ```cpp
   const hn::ScalableTag<double> d;
   for (size_t i = 0; i + hn::Lanes(d) <= n; i += hn::Lanes(d)) {
     hn::StoreU(hn::Add(hn::LoadU(d, a + i), hn::LoadU(d, b + i)), d, out + i);
   }
   ```

3. Call them from R through any of Rcpp, cpp11 or plain `.Call`
   (`highway/r_guard.h` turns C++ exceptions into R errors).

The vignette, `vignette("highway")`, walks through more details such as static
and dynamic dispatch, transcendental functions, `NA` handling, integers, C
packages, threads and the CRAN checklist.

You can use functions in this package to report on the bundled Highway library
version and to find details of instruction sets supported by your CPU:

```r
library(highway)
highway_version()      # 1.4.0
supported_targets()    # e.g. "AVX3_SPR" "AVX3" "AVX2" "SSE4" "SSSE3" "SSE2" "EMU128" "SCALAR"
dispatched_target()    # the target this package's demo kernels use here
hwy_demo_exp(c(0, 1, NA))
```

## What is in the package

* `inst/include/hwy/`: Highway's headers with the upstream layout, so
  `#include "hwy/highway.h"` and upstream documentation work unchanged.
  This includes the core, all x86 and Arm back ends, the portable EMU128 and
  scalar back ends, and the header-only contrib modules `math`, `algo`,
  `dot`, `bit_pack`, `image` and `unroller`.
* `inst/include/highway/`: additions specific to this package, namely
  `impl.h` (Highway's compiled runtime, included in exactly one file per
  package), `r_guard.h` and `c_guard.h` (exception guards for `.Call` and C
  interfaces), `runtime.h` (`highway::warm_up()`), and `config.h`.
* R function `use_highway()`, plus `highway_version()`, `supported_targets()`,
  `compiled_targets()`, `dispatched_target()` and three demonstration
  kernels.

The bundled library is patched so that Highway never writes to `stderr` or
terminates R (the abort handler throws `highway::abort_error` instead), its
runtime symbols are private to each package, and it does not suppress
compiler warnings. All changes are listed in `inst/COPYRIGHTS` and kept as
patch files in `tools/patches/`.

## Platforms and targets

| Platform | Targets compiled by default |
|----------|-----------------------------|
| x86-64 (Linux, macOS, Windows) | SSE2, SSSE3, SSE4, AVX2, AVX3 (AVX-512), AVX3_DL, AVX3_ZEN4, AVX3_SPR, AVX10_2 (the last ones where the compiler supports them) |
| arm64 Linux | NEON (with and without AES), NEON_BF16, SVE, SVE2, SVE_256, SVE2_128 |
| arm64 macOS | NEON, NEON_BF16 |
| ppc64le, s390x, riscv64, other | EMU128 (portable; native back ends not bundled) |

`HIGHWAY_TARGETS_LEAN` reduces the list to the most useful targets, to
shorten installation.

## Versioning scheme

The package version is `<Highway version>-<packaging revision>`, following
BH and RcppArmadillo: `1.4.0-1` is the first release bundling Highway
1.4.0, `1.4.0-2` would be a packaging fix, and `1.5.0-1` the first release
bundling Highway 1.5.0. `highway_version()` returns the bundled Highway
version.

## Citing

If you use this package in published work, please cite both Highway itself
(upstream does not suggest a citation, so e.g. Highway, Google LLC and
contributors, <https://github.com/google/highway>), and this package using
`citation("highway")`.

## Licence

This package and the bundled Highway code are distributed under the BSD
3-Clause License. Highway is dual-licensed by its authors under the Apache
License 2.0 or the BSD 3-Clause License; this package uses the BSD 3-Clause
option, which is compatible with GPL-2 packages. Copyright in Highway belongs
to Google LLC and the other Highway authors; see `inst/COPYRIGHTS`.

This package is not affiliated with or endorsed by Google.
