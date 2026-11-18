## Submission

This is a new submission.

'highway' bundles the C++ header library 'Highway' 1.4.0 by Google
(<https://github.com/google/highway>, tag 1.4.0, commit 2607d3b5) so that
other packages can use it through 'LinkingTo', in the same way as BH and
RcppEigen. `inst/COPYRIGHTS` records every copyright holder of the bundled
files, the licence election, the list of files bundled and not bundled, and
every local modification:

* Highway is dual-licensed by its authors under Apache-2.0 or BSD-3-Clause
  (top-level upstream LICENSE). The package uses the BSD-3-Clause option, so
  that GPL-2 packages can link to it. Many bundled files still carry
  Apache-only per-file headers from before upstream's relicensing; all
  notices are kept unchanged and the project-level grant is quoted in
  `inst/COPYRIGHTS`.
* Google LLC, Arm Limited and Matthew Kolbe (the holders named in the
  bundled files) and "The Highway Project Authors" are listed as `cph` in
  Authors@R.

### Compiled code and compiler flags

The package uses no architecture-specific compiler flags. Highway compiles a
kernel once per instruction set inside one shared object, using per-function
target attributes, and selects the best version at run time from CPUID (x86)
or `getauxval()`/`sysctlbyname()` (Arm). CRAN's baseline binaries therefore
use SSE4/AVX2/AVX-512 where available and still run on any CPU.

Local changes made for CRAN (patch files in `tools/patches/`):

* Highway's abort handler no longer writes to `stderr` or calls
  `abort()`/`exit()`; it throws a C++ exception that the package's `.Call`
  boundary turns into an R error. The warning handler is silent.
* Highway's diagnostic-suppression pragmas (`_Pragma("GCC diagnostic ...")`
  inside macros) are compiled out; the warnings they hid were fixed at
  source, verified by compiling upstream's own op tests with GCC 16 and
  clang 23 at `-Wall -Wextra -pedantic` (no warnings in bundled files).
* Runtime symbols have hidden visibility, so that packages that each compile
  Highway's small runtime cannot interfere with each other.

### Size

Source tarball about 0.56 MB. The SIMD back ends for POWER, s390x, RISC-V,
LoongArch and WebAssembly are not bundled, which keeps the installed size
below 5 MB on Linux with debug information; on those architectures the
package uses Highway's portable target (tested under emulation on ppc64le,
s390x and riscv64).

### Tests

The tests take a few seconds, use at most two threads, need no network and
write only to `tempdir()`.

## Test environments

(Filled in from `dev/check-results/` at hand-off.)

## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new submission.
