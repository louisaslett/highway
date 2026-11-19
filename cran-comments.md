## Submission

This is a new submission.

'highway' bundles the C++ header library 'Highway' 1.4.0 by Google
(<https://github.com/google/highway>, tag 1.4.0, commit 2607d3b5) so that
other packages can use it through 'LinkingTo', in the same way as BH and
RcppEigen. I mention this because it naturally makes the licensing issues
important to get right. This is my first time wrapping a library.

`inst/COPYRIGHTS` records every copyright holder of the bundled
files, the licences, the list of files bundled and not bundled, and
every local modification:

* Highway is dual-licensed by its authors under Apache-2.0 or BSD-3-Clause
  (top-level upstream LICENSE). For the package I'm using the BSD-3-Clause
  option, so that GPL-2 packages can link to it. Many bundled files still carry
  Apache-only per-file headers which come from before the upstream chose to
  relicense (as the upstream README states). I kept all notices unchanged, as
  both licences require, and `inst/COPYRIGHTS` quotes upstream's project-wide
  licence statement.
* Google LLC, Arm Limited and Matthew Kolbe (the holders named in the
  bundled files) and "The Highway Project Authors" are listed as `cph` in
  Authors@R.

I've tried to follow what I saw as best practice from other wrapper packages,
but please do let me know if I've not interpreted correctly.

### Notable changes in wrapped library

I made changes which I believed necessary for CRAN (patch files are in
`tools/patches/`):

* Highway's abort handler no longer writes to `stderr` or calls
  `abort()`/`exit()`; it throws a C++ exception that the package's `.Call`
  boundary turns into an R error. The warning handler is silent.
* Highway's diagnostic-suppression pragmas (`_Pragma("GCC diagnostic ...")`
  inside macros) are compiled out. The warnings they hid were fixed at
  source, verified by compiling upstream's own op tests with GCC 16 and
  clang 23 at `-Wall -Wextra -pedantic` (so no warnings in bundled files
  in those tests).
* Runtime symbols have hidden visibility, so that packages that each compile
  Highway's small runtime cannot interfere with each other.

### Size

I'm mentioning this because post install it gets close to CRAN limits. The
source tarball is about 0.56 MB. I stripped the SIMD back ends for POWER, s390x,
RISC-V, LoongArch and WebAssembly since these are not platforms CRAN checks or
builds binaries for. This keeps the installed size below 5 MB on Linux with debug
information. The stripped architectures will still fall back to Highway's
portable target (tested under emulation on ppc64le, s390x and riscv64).

## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new submission.
