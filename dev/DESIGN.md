# highway: design notes

Status: approved 2026-11-18 with the recommended option for every
**[decision]** (diagnostic suppression compiled out; POWER/s390x, RISC-V and
WebAssembly back ends not bundled; Intel SDE used in CI; knitr + rmarkdown).
Section 15 records what implementation and CI found afterwards.

## 1. What is being built

An R package `highway` that ships a trimmed, pinned, lightly patched copy of
Google Highway so other CRAN packages can write portable SIMD kernels with runtime
dispatch by adding `LinkingTo: highway` plus one small `src/highway-impl.cpp`.
No architecture-specific compiler flags anywhere.

## 2. Sources consulted (current versions, read 2026-11-18)

| Source | Version seen | Key points used |
|---|---|---|
| CRAN Repository Policy | SVN rev 6985 | no `assert`/`abort`/`exit`/`std::terminate`; "should not attempt to disable compiler diagnostics, nor remove ... symbols in shared objects"; ≤ 2 threads; write only to `tempdir()`; tarball "should if possible not exceed 10MB"; copyright of all components "clear and unambiguous", `ctb`/`cph` in `Authors@R` or `inst/COPYRIGHTS`; public API only |
| *Writing R Extensions* (r-release 4.6.2 and r-devel) | Nov 2026 | default C++ standard is C++17 from R 4.4.0 and C++20 ("where available") from R 4.6.0; specify a standard only if code compiles *only* with it; `R CMD INSTALL` defines `NDEBUG`; no stdout/stderr, no system or C++11 or third-party RNGs; `$(CXX_VISIBILITY)`; compiled-code symbol scan |
| CRAN submission checklist | Nov 2026 | quotes for software names, `foo()` without quotes, URLs in `<>`, ORCID in `comment` |
| CRAN check flavours page | Nov 2026 | Linux: GCC 16.2 and clang 23.1 (r-devel), GCC 15.3 (release); Windows: GCC 14.3 (Rtools); macOS arm64: Apple clang 1700.3 (Xcode 17); macOS x86_64: Apple clang 1403 (Xcode 14) |
| R source `tools:::.check_pragmas` (4.6.2 and trunk) | trunk 2026-11 | flags only literal lines matching `^\s*#pragma (GCC|clang) diagnostic ignored` under `src/` and `inst/include/`; `-Wuninitialized`, `-Wformat`, ... are "important" (WARNING) |
| R source `tools:::so_symbol_names_table` | 4.6.2 | flags `abort`, `assert` (`__assert_fail`/`__assert_rtn`), `exit`, `_exit`, `_Exit`, `printf`, `puts`, `putchar`, `sprintf`, `vsprintf`, `vprintf`, `stdout`, `stderr`, `rand`, `random`, `rand_r`, `srand`, `srandom`, `srand48`, `std::cout`, `std::cerr` |
| R source `check_install_sizes` | trunk | NOTE when `du -k` of the installed package exceeds `_R_CHECK_PKG_SIZES_THRESHOLD_` (default 5 MB); counts debug info inside `libs/` |

Facts that differ from, or sharpen, the task brief:

1. **Diagnostic pragmas.** Highway contains no literal `#pragma GCC/clang diagnostic`
   lines, so `R CMD check` (release and devel) does not flag it. It *does* suppress
   diagnostics through `_Pragma` macros (`HWY_DIAGNOSTICS_OFF`, about 55 sites, including
   `-Wuninitialized`, `-Wmaybe-uninitialized`, `-Wignored-attributes`,
   `-Wsign-conversion`). The policy text forbids attempting to disable diagnostics
   regardless of mechanism, and the most recent precedent (RcppSimdJson 0.1.16, Oct 2026,
   "Do not use 'diagnostic ignored' pragmas for gcc or clang") removed even
   macro-wrapped ones. See §9 for the proposed handling. **[decision]**
2. **Installed size**, not the tarball, is the binding constraint: CRAN Linux compiles
   with `-g`, debug info lands in `libs/`, and stripping is forbidden by policy. See §4.
3. **C++ standard.** Highway needs C++17 or later and works with C++20/23. R ≥ 4.4
   defaults to ≥ C++17, so setting `CXX_STD`/`SystemRequirements` would wrongly pin
   C++17 (WRE: specify only if the code compiles only with that standard). Neither is
   set; `Depends: R (>= 4.4.0)` guarantees a ≥ C++17 default.
4. **Third-party RNGs are disallowed** (WRE §1.6.4), which excludes
   `hwy/contrib/random` irrespective of size.
5. **No CRAN machine has AVX-512.** Debian (Xeon E5-2690, Sandy Bridge) runs SSE4;
   Fedora/Windows (Broadwell, Zen 3) run AVX2. AVX-512 evidence must come from GitHub
   runners (opportunistic) and Intel SDE (deterministic).

## 3. Precedent conventions adopted

| Convention | Adopted from |
|---|---|
| Headers under `inst/include/<upstream dir>/` with the upstream layout unchanged, so upstream docs work verbatim (`#include "hwy/highway.h"`) | BH (`inst/include/boost`), RcppEigen (`inst/include/Eigen`) |
| Version `<upstream>-<packaging>` (`1.4.0-1`) | BH (`1.90.0-1`), RcppArmadillo (`15.6.2-1`), RcppXsimd (`7.1.6-2`) |
| `License: BSD_3_clause + file LICENSE` with the two-line CRAN template; per-holder detail and full texts in `inst/COPYRIGHTS` | RcppXsimd (BSD-3 template), RcppEigen (`inst/COPYRIGHTS`, `Copyright:` field pointing to it) |
| Upstream authors as a group `cph` entry plus named holders | RcppEigen (`"Authors of Eigen"`, `cph`), RcppXsimd (named upstream authors `cph, ctb`) |
| Local patches kept as files and re-applied by the import script; patches listed in ChangeLog/NEWS | BH (`local/patches/*.patch`, "comment out 'diagnostic ignored' for CRAN") |
| Diagnostic-suppression pragmas neutralised, not kept | BH, RcppEigen (`DisableStupidWarnings.h` lines commented out), RcppSimdJson (PR #106, Oct 2026) |
| tinytest with tests in `inst/tinytest/` and a guarded `tests/tinytest.R` runner, so users can re-run the suite after install | RcppEigen, RcppArmadillo, RcppSimdJson |
| Package-setup helper that edits `DESCRIPTION`/`src/`, never overwrites silently, reports each change | `usethis::use_cpp11()` (via cpp11 docs), `RcppEigen.package.skeleton()` (prints `>> added ...`) |
| `PKG_CXXFLAGS = $(CXX_VISIBILITY)` for the package's own code; `R_init_*` marked `attribute_visible` | RcppSimdJson `src/Makevars`, WRE §6.19 |
| No `-m` flags in Makevars (RcppXsimd ships commented-out `-mavx` lines; we ship none) | (counter-example) RcppXsimd |

## 4. Upstream pin and bundle

- Upstream: <https://github.com/google/highway>, tag **`1.4.0`** (latest release,
  2026-04-23; no newer tag as of 2026-11-18), commit
  **`2607d3b5b0113992fe84d3848859eae13b3b52c1`**. GitHub archive tarball SHA-256
  `e72241ac9524bb653ae52ced768b508045d4438726a303f10181a38f764a453c` (the script
  verifies the commit SHA via the API and the tarball hash).
- Licence: upstream `LICENSE` states the project is "primarily dual-licensed under your
  choice of either the Apache License 2.0 or the BSD 3-Clause License", with
  `hwy/contrib/random/random-inl.h` (CC0) the only exception. 45 of the 47 shipped
  files still carry Apache-only per-file headers that predate the relicensing (README:
  "Previously licensed under Apache 2, now dual-licensed"). We keep every notice
  untouched and elect BSD-3-Clause under the project-level grant. `inst/COPYRIGHTS`
  quotes the project statement so a reviewer sees why the per-file headers do not
  contradict the election.
- Copyright holders in shipped files: Google LLC (all files); Arm Limited and/or its
  affiliates (`abort.cc`, `abort.h`, `detect_targets.h`, `targets.cc`,
  `ops/arm_neon-inl.h`, `ops/arm_sve-inl.h`, `ops/generic_ops-inl.h`,
  `ops/set_macros-inl.h`); Matthew Kolbe (`contrib/unroller/unroller-inl.h`). The BSD
  text names "The Highway Project Authors".

### Kept

| Path | Why |
|---|---|
| `hwy/*.h` core: `abort.h aligned_allocator.h base.h bit_set.h cache_control.h detect_compiler_arch.h detect_targets.h foreach_target.h highway.h highway_export.h per_target.h targets.h x86_cpuid.h` | include closure of `highway.h`, `aligned_allocator.h`, contrib modules |
| `hwy/ops/`: `arm_neon arm_sve emu128 generic_ops inside scalar set_macros shared x86_128 x86_256 x86_512 x86_avx3` | x86 and arm64 targets plus portable fallbacks |
| `hwy/abort.cc targets.cc per_target.cc aligned_allocator.cc contrib/image/image.cc` | compiled once per consumer through `highway/impl.h` (§5) |
| `hwy/contrib/{math,algo,dot,bit_pack,unroller,image}` | header-only (image needs `image.cc`, provided by `impl.h`) |

### Excluded (each recorded in `inst/COPYRIGHTS` and `tools/highway-files.txt`)

| Path | Reason |
|---|---|
| `hwy/tests/`, `*_test.cc`, `hwy/examples/`, `g3doc/`, `docs/`, CMake/Bazel/Meson/Debian files | not needed at build time |
| `hwy/contrib/sort/` | 22 per-type `.cc` files and about 9k lines; uses its own pivot RNG |
| `hwy/contrib/thread_pool/`, `hwy/contrib/matvec/` (depends on it) | spawns threads (CRAN two-core rule), `printf`/`stderr` diagnostics |
| `hwy/contrib/random/` | third-party RNG (WRE forbids); CC0 licence |
| `nanobenchmark`, `profiler`, `perf_counters`, `stats`, `timer`, `robust_statistics`, `auto_tune`, `print`, `print-inl` | benchmarking/debug helpers that print to stdout/stderr or need extra `.cc`; not used by kept headers |
| `ops/loongarch_lsx`, `ops/loongarch_lasx` (400 KB), `ops/wasm_256` (81 KB) | size budget; no CRAN or R-hub platform; those builds fall back to EMU128 (forced in `highway/config.h`) |
| `ops/ppc_vsx` (279 KB, also used for s390x Z14/Z15), `ops/rvv` (287 KB), `ops/wasm_128` (233 KB) **[decision]** | size budget (below). ppc64le, s390x, riscv64 and webR then compile and run EMU128 (portable, correct, slower). WASM SIMD would need `-msimd128` anyway, which is forbidden |

### Size budget

Installed size must stay under 5 MB on CRAN Linux, which compiles with `-g`.

| Component | Estimate |
|---|---|
| Headers (everything above kept; 47 files today, 4.30 MB) | 4.30 MB |
| ... minus LoongArch + WASM256 | 3.82 MB |
| ... minus PPC/s390x VSX, RVV, WASM128 **[decision]** | 3.02 MB |
| `libs/highway.so` with `-g`: demo kernels for 9 x86 targets plus `impl.h` (measured 0.9 MB for exp/log/sin/dot/find alone, GCC 16 `-O2 -g`) | ~1.0–1.4 MB |
| R code, help, vignette HTML, tinytest | ~0.3 MB |
| **Total** (with the decision) | **~4.3–4.7 MB** |

The source tarball is about 0.7 MB compressed (measured on the 4.3 MB set). Without
dropping VSX/RVV/WASM128 the Linux total is estimated at 5.1–5.5 MB, which would
trigger the installed-size NOTE. Both sizes are re-measured in Phase 3; if there is
margin, VSX is the first thing to restore.

## 5. Compiled parts: `highway/impl.h`

Upstream's dispatch needs a few non-inline definitions. From `CMakeLists.txt`
(`HWY_SOURCES`) and the include graph, the minimal set is:

| File | Provides | Needed by |
|---|---|---|
| `targets.cc` | `SupportedTargets`, `GetChosenTarget`, `DisableTargets`, `SetSupportedTargetsForTest`, CPU detection (CPUID / `getauxval` / `sysctlbyname`) | every `HWY_DYNAMIC_DISPATCH` |
| `abort.cc` (patched) | `Abort`, `Warn`, `Set/GetAbortFunc`, `Set/GetWarnFunc` | any `HWY_ASSERT`/`HWY_ABORT` |
| `per_target.cc` | `VectorBytes`, `DispatchedTarget`, `Have*` | `aligned_allocator.h` (`AlignedNDArray`), `image.cc` |
| `aligned_allocator.cc` | `AllocateAlignedBytes`, `FreeAlignedBytes` | `AllocateAligned`, `image.h` |
| `contrib/image/image.cc` | `ImageBase` methods | `hwy/contrib/image` users only |

`print.cc`, `timer.cc`, `nanobenchmark.cc`, `profiler.cc`, `perf_counters.cc` are not
needed (their headers are excluded).

`inst/include/highway/impl.h` (stb style):

```cpp
#ifdef HIGHWAY_IMPL_H_INCLUDED
#error "highway/impl.h included twice in one translation unit ..."
#endif
#define HIGHWAY_IMPL_H_INCLUDED
#include "hwy/per_target.cc"   // first: it defines HWY_COMPILE_ALL_ATTAINABLE
#include "hwy/targets.cc"
#include "hwy/abort.cc"
#include "hwy/aligned_allocator.cc"
#include "hwy/contrib/image/image.cc"
#include "highway/runtime_impl.h"   // highway::warm_up() etc.
```

Including it in **two** translation units gives duplicate-symbol link errors at
install time; including it in **none** gives "undefined symbol hwy::..." at load
time. Both are documented in Troubleshooting, with the exact messages.

**Isolation between consumer packages.** Each consumer gets its own copy of the
runtime state (chosen target, handlers). Defences:

1. Patch `highway_export.h` so that `HWY_DLLEXPORT` (empty in non-shared builds) becomes
   `__attribute__((visibility("hidden")))` on ELF/Mach-O GCC/Clang. All upstream
   runtime functions get hidden visibility whatever flags the consumer uses. The
   package's own `highway::` functions use the same macro.
2. Highway ops are `HWY_API` = `static inline` (internal linkage), dispatch tables
   are `static`, and `FunctionCache<>` instantiations take internal-linkage table
   pointers, so no vague-linkage/`STB_GNU_UNIQUE` statics are shared.
3. Windows: DLLs have separate symbol namespaces; macOS: two-level namespaces.
4. Test: consumerA and consumerB loaded together, including one loaded with
   `dyn.load(local = FALSE)`. Each sets a different abort handler and test-target
   mask and checks that the other is unaffected. On Linux, `readelf --dyn-syms` asserts
   no `hwy::`/`highway::` symbols are exported.

**Static-dispatch-only use** needs no `impl.h`: the consumer defines
`HWY_HEADER_ONLY` (upstream's own switch) in `PKG_CPPFLAGS`. The patched inline
`Abort` throws `highway::abort_error` and the patched inline `Warn` does nothing;
custom handlers are unavailable in that mode.

## 6. Patches (`tools/patches/NNNN-*.patch`, applied in order; fail loudly)

| # | File(s) | Change | Why |
|---|---|---|---|
| 0001 | `hwy/abort.cc`, `hwy/base.h` (header-only branch) | default Abort: format the message, call the user handler if set, then `throw highway::abort_error(msg)` (never returns, never calls `abort`/`exit`/`fprintf`/`__sanitizer_print_stack_trace`); default Warn: no output; if a user handler is set, call it | CRAN: no `abort`/`stderr`; Warn may run on non-R threads |
| 0002 | `hwy/highway_export.h` | hidden visibility for `HWY_DLLEXPORT` in non-shared builds | consumer isolation (§5) |
| 0003 | `hwy/detect_targets.h` | `#include "highway/config.h"` first; OR `HIGHWAY_R_BROKEN_TARGETS` into `HWY_BROKEN_TARGETS` | one place for R-specific target restrictions (Windows, if needed) and EMU128 forcing for unbundled arches |
| 0004 | `hwy/base.h` | `HWY_WARN/HWY_ABORT(format, ...)` → `(...)` with `__VA_ARGS__` (no GNU `##__VA_ARGS__`) | clang `-pedantic` `-Wgnu-zero-variadic-macro-arguments` in every consumer |
| 0005 | `hwy/base.h` | `unsigned __int128` → `__uint128_t` in `Divisor64` | GCC `-Wpedantic` "ISO C++ does not support '__int128'" |
| 0006 | `hwy/ops/x86_128-inl.h` | `sizeof(RemoveCvRef<decltype(v.raw)>)` → `sizeof(v.raw)` | GCC `-Wignored-attributes` once the suppression is removed |
| 0007 | `hwy/base.h` | `HWY_DIAGNOSTICS`/`HWY_DIAGNOSTICS_OFF` expand to nothing for GCC/Clang **[decision]** | policy: do not disable diagnostics |
| 00xx | as found | further warning fixes found by compiling upstream's op tests with suppression off | zero warnings with `-Wall -Wextra -pedantic` |

Preliminary experiment (dispatched kernel using math, algo, dot, find; suppression
macros neutralised; `-O2 -Wall -Wextra -pedantic`):

| Compiler / target set | Warnings | After 0004–0006 |
|---|---|---|
| Apple clang 17, arm64 (NEON, NEON_BF16) | 4 (`##__VA_ARGS__`) | 0 (expected) |
| Apple clang 17, x86_64 (7 targets) | 19 (`##__VA_ARGS__`) | 0 (expected) |
| GCC 16.2, aarch64 (NEON×3, SVE, SVE2, SVE_256, SVE2_128) | 2 (`__int128`) | 0 (expected) |
| GCC 16.2, x86_64 (SSE2 ... AVX10_2, 9 targets) | 10 (`__int128`, `-Wignored-attributes`) | 0 (expected) |

Phase 2 widens this to every op by compiling upstream's `hwy/tests/*` with the
patched headers under GCC 16 and clang 23 on x86_64 and aarch64.

## 7. Error handling and thread safety

- `highway::abort_error : std::runtime_error` (in `highway/abort_error.h`), carrying
  `file`, `line` and Highway's message. Thrown only from `hwy::Abort` (noreturn, so
  never returns into SIMD code). No Highway API on the dispatch path is `noexcept`. The
  only `noexcept` paths are destructors (`AlignedDeleter`), which can reach `Abort`
  only through `HWY_DASSERT`, active only in debug/ASAN builds and only on an internal
  bug. Documented.
- Default `Warn`: nothing. User handlers via upstream `hwy::SetAbortFunc` /
  `hwy::SetWarnFunc` (atomic, thread-safe). If a user abort handler returns, we still
  throw (upstream would `abort()`). Handlers must be thread-safe and must not call the
  R API. Documented.
- First dispatch: `FunctionCache::ChooseAndCall` → `SupportedTargets()` (pure CPUID or
  auxv read) → `ChosenTarget::Update` (`std::atomic<int64_t>` store of the same value).
  Concurrent cold starts are benign; `GetChosenTarget()` is a constant-initialised
  function-local static. The only plain statics, `supported_mask_` and
  `supported_targets_for_test_`, are written only by `DisableTargets` /
  `SetSupportedTargetsForTest`, documented as call-before-threads test hooks.
- `highway::warm_up()` runs `GetChosenTarget().Update(SupportedTargets())` once; it
  is meant for `R_init_<pkg>` or `.onLoad`.
- Nothing in `impl.h`, the handlers or `highway/*.h` (except `r_guard.h`) includes R
  headers.
- `highway/r_guard.h`: `HIGHWAY_R_BEGIN` / `HIGHWAY_R_END` store the message in a
  stack buffer inside `catch`, leave the `try`/`catch` (so all C++ destructors have
  run) and only then call `Rf_error("%s", buf)`. Only `<R_ext/Error.h>` (public API).
- `highway/c_guard.h`: `template <class F> int highway::c_guard(F&&, char* buf, size_t len) noexcept`
  plus `HIGHWAY_C_BEGIN/END` macros. Statuses: 0 OK, 1 `abort_error`, 2 `bad_alloc`,
  3 other `std::exception`, 4 unknown. Uses only `memcpy`/`strlen`.
- TSan: R-hub has no TSan image, so a standalone C++ harness (`dev/tsan/`) builds the
  demo kernels with `impl.h` under `-fsanitize=thread` and runs concurrent cold-start
  dispatch plus a worker-thread abort. It runs in CI (Linux x86_64 and arm64) and in a
  local arm64 container.

## 8. R-level API (base R only)

Exported: `highway_version()`, `supported_targets()`, `compiled_targets()`,
`dispatched_target()` (the target the demo kernels actually use; needed for the
vignette's "which target ran" section), `use_highway(path = ".", language = c("cpp", "c"), kernel_template = TRUE, overwrite = FALSE)`,
and demo kernels `hwy_demo_exp()`, `hwy_demo_log()`, `hwy_demo_sum()`. Internal
`.Call` hooks for tests: run a kernel on a forced target, concurrent and cold-start
runs on two `std::thread`s, abort-on-worker through the C guard.

Demo kernels match base R exactly where Highway's documented range ends: `Exp` is
1 ULP on [-DBL_MAX, 706] and `Log` is 4 ULP on (0, DBL_MAX]. Vectors containing any
lane outside that range, or NA/NaN, are handled lane-wise in scalar code, so payloads
(NA vs NaN), ±Inf, 0, -0 and denormals behave exactly like `exp()`/`log()`. The
vignette explains why and shows that raw `hn::Exp` does not preserve NA.

`use_highway()`: adds `highway` to `LinkingTo` (minimal textual edit; preserves the
rest of `DESCRIPTION`), writes `src/highway-impl.cpp`, adds `-I.` to `PKG_CPPFLAGS` in
`src/Makevars` and `src/Makevars.win` (needed because `foreach_target.h` re-includes
`HWY_TARGET_INCLUDE` relative to the include path, not the includer; to be verified in
Phase 3), optionally writes a dispatched kernel template, and for `language = "c"`
writes `src/highway-kernels.cpp` (`extern "C"`, raw pointers, `int` status, every
function wrapped in `highway::c_guard`) and `src/highway-kernels.h`. Existing files
are never changed unless `overwrite = TRUE`; edits to existing `DESCRIPTION`/`Makevars`
are additive. Every action is reported with `message()`, and the function returns the
list of changed paths invisibly. It touches nothing outside `path`.

Target count and install time: upstream's default on x86 compiles 9 targets.
`highway/config.h` will offer `HIGHWAY_TARGETS_LEAN` (set before including Highway in a
kernel file), a curated subset chosen from Phase 4 timings (likely SSE2, SSE4, AVX2,
AVX3, AVX3_SPR on x86; NEON, NEON_BF16, SVE, SVE2 on arm64). The template kernel opts
in by default. The package's own demos compile all attainable targets (for SDE
coverage) unless the size budget forces otherwise.

## 9. Diagnostic suppression **[decision]**

- Option A (recommended): neutralise `HWY_DIAGNOSTICS*` for GCC/Clang (patch 0007) and
  fix whatever warnings appear with source patches, verified across all ops on GCC 16
  and clang 23. Complies with the policy text and matches BH, RcppEigen and
  RcppSimdJson. Cost: more patches to carry on each upstream update; a consumer kernel
  could in principle hit a warning path not covered by upstream's tests. Those get
  fixed in the next patch release.
- Option B: keep upstream's `_Pragma` suppressions (the brief's "patch only what is
  flagged"). Nothing is flagged by `R CMD check` today, but it conflicts with the policy
  sentence and is the kind of thing CRAN reviewers have asked recent packages to
  remove.

## 10. Tests

tinytest (zero dependencies, installed with the package so
`tinytest::test_package("highway")` re-checks a user's machine, same pattern as
RcppEigen/RcppArmadillo/RcppSimdJson). `tests/tinytest.R` skips cleanly when
tinytest is absent. CRAN-run tests: seconds, ≤ 2 threads, no network, `tempdir()`
only.

Coverage: edge values (0, -0, denormals, ±Inf, NaN, NA_real_, ±DBL_MAX, tiny), lengths
0, 1, L−1, L, L+1 and large (L from the dispatched target). ULP checks against
documented bounds (+1 ULP allowance for libm's own error). Every target in
compiled ∩ supported is forced via `SetSupportedTargetsForTest`, and the test asserts
the dispatched target equals the forced one. Concurrency (2 threads, cold start). C
guard on a worker thread.

Consumer packages in `dev/consumer-tests/` (Rbuildignored): consumerA (`use_highway()`,
dynamic, contrib/math), consumerB (independent), consumerRcpp, consumerCpp11, consumerC
(`language = "c"`). Each passes `R CMD check --as-cran` 0/0/0 (that check's symbol scan
is the proof that the abort/stderr patches work in consumer builds).

Windows spill test: a dispatched kernel that holds more than 32 live vectors (forces
spills) on AVX2 and AVX3 lanes, compared against scalar. Run on GitHub Windows
(AVX2 real; AVX-512 only if the runner has it, plus SDE for Windows) and win-builder. If
it fails: restrict via `HIGHWAY_R_BROKEN_TARGETS` in `config.h`, plus a test asserting
the restriction is active.

## 11. CI and evidence

GitHub Actions (`r-lib/actions` v2):

- R CMD check `--as-cran`: ubuntu-latest devel, release, oldrel-1, oldrel-2 (4.4,
  floor for `Depends`); ubuntu-24.04-arm release; macos-latest (arm64) release;
  macos-15-intel release; windows-latest release and devel.
- Consumers job: build and install highway, check all five consumers, load A+B
  together.
- Intel SDE job (Linux x86_64): `-skx` (AVX3), `-icx` (AVX3_DL), `-spr` (AVX3_SPR),
  `-dmr` or the newest AVX10.2 model if SDE 10.13.1 has it (AVX10_2), plus `-hsw`
  (AVX2), `-snb`/`-wsm` (SSE4/SSSE3 floor) for completeness. Pinned SDE 10.13.1
  (2026-07-28), SHA-256 `94e97d62...bf2b50` from Intel's download page, cached.
  Download from Intel's mirror, not the third-party `petarpetrovt/setup-sde`, which
  re-hosts Intel's binaries in its own repository.
- TSan job (`dev/tsan`), plus R-hub v2: `gcc-asan`, `clang-asan`, `clang-ubsan`,
  `valgrind`, `gcc16`, `clang23`, `nosuggests`, `m1-san`.
- Every job prints `supported_targets()`, `dispatched_target()` and the CPU model.
- Pre-submission: win-builder (devel, release, oldrelease), mac-builder, `urlchecker`,
  `spelling`, local `--as-cran` with `_R_CHECK_CRAN_INCOMING_REMOTE_=true`, a local
  strict-flags build via a git-ignored `dev/Makevars.strict` and `R_MAKEVARS_USER`.

Evidence rule: a target counts only if its code ran on real hardware that supports it
or under SDE. Local Rosetta/QEMU runs are build smoke tests only.

Intel SDE licence: Intel Simplified Software License (version 2023-08-23); Intel
describes SDE as "free of charge for any type of use". Downloading requires accepting
the licence on Intel's site **[decision: maintainer to accept or authorise]**. CI
downloads at run time from Intel's mirror; nothing is redistributed.

## 12. DESCRIPTION (draft)

```
Package: highway
Type: Package
Title: 'Highway' 'C++' Headers for Portable 'SIMD' with Runtime Dispatch
Version: 1.4.0-1
Authors@R: c(
    person("Louis", "Aslett", role = c("aut", "cre"),
           email = "louis.aslett@durham.ac.uk",
           comment = c(ORCID = "0000-0003-2211-233X")),
    person("Google LLC", role = "cph",
           comment = "Bundled 'Highway' library (all files under inst/include/hwy)"),
    person("Arm Limited and/or its affiliates", role = "cph",
           comment = "Parts of the bundled 'Highway' library; see inst/COPYRIGHTS"),
    person("Matthew", "Kolbe", role = "cph",
           comment = "inst/include/hwy/contrib/unroller/unroller-inl.h"),
    person("The Highway Project Authors", role = c("ctb", "cph"),
           comment = "Authors of the bundled 'Highway' library; see inst/COPYRIGHTS"))
Description: Provides the header files of 'Highway'
    <https://github.com/google/highway>, a 'C++' library for portable 'SIMD'
    (single instruction, multiple data) programming, for use by other packages
    via 'LinkingTo'. ...
License: BSD_3_clause + file LICENSE
Copyright: See inst/COPYRIGHTS for the bundled 'Highway' library.
Depends: R (>= 4.4.0)
Suggests: tinytest, knitr, rmarkdown
VignetteBuilder: knitr
URL: https://github.com/louisaslett/highway
BugReports: https://github.com/louisaslett/highway/issues
Encoding: UTF-8
```

No `SystemRequirements`/`CXX_STD` (§2.3). No Rcpp/cpp11 anywhere in Imports or
LinkingTo. README states the package is not affiliated with or endorsed by Google.

## 13. Vignette builder

knitr + `rmarkdown::html_vignette`: self-contained output with no network, available
on every CRAN flavour, and the same as cpp11. litedown was considered (lighter, no
pandoc), but its default template references CSS/JS on a CDN. C++ listings are plain
fenced code blocks, never compiled; output comes from the demo functions. Without
knitr/rmarkdown the vignette is not rebuilt (standard behaviour with
`_R_CHECK_FORCE_SUGGESTS_=false`).

## 14. Open risks

1. Installed size on CRAN Linux (-g): budgeted at ~4.5 MB; re-measured in Phase 3.
2. MinGW GCC 14 AVX spill alignment (GCC PR 54412): upstream works around vector
   *arguments* (`VecArg` by `const&`), not spills; must be tested on real Windows.
3. Warnings exposed by neutralising the suppression macros (Option A) beyond
   upstream's test coverage.
4. Consumer install time with 9 x86 targets; mitigated by `HIGHWAY_TARGETS_LEAN`.
5. win-builder results arrive by email to the maintainer; the maintainer has to
   forward the result links.
6. AVX-512 on GitHub runners is not guaranteed; SDE is the deterministic source.

## 15. Findings during implementation (2026-11-18)

1. **Patches needed beyond the plan.** 0008 (MinGW `-Wformat`): Rtools GCC
   checks `__attribute__((format(printf, ...)))` against the Microsoft format
   syntax and warned on `%zu` in `image.cc`; R CMD check reported it as a
   significant warning on Windows. `HWY_FORMAT` now uses `__gnu_printf__` on
   MinGW (the formatting itself goes through C99-conforming `vsnprintf`).
   0002 was narrowed: hiding `hwy::ImageBase` made GCC warn
   (`-Wattributes`) for every `hwy::Image<T>`; `ImageBase` is stateless, so
   only the runtime functions (`HWY_DLLEXPORT`) are hidden.
2. **Warning sweep** (upstream op and contrib tests, 86 translation units,
   `-O2 -Wall -Wextra -pedantic`, suppression compiled out): 0 warnings in
   shipped files with Apple clang 21 (arm64, x86_64), GCC 16.2 (x86_64 after
   fix 1, arm64), and in CI with GCC 16 and clang 23 on native x86_64 and
   arm64 runners (`header-warnings.yaml`).
3. **Windows stack alignment (GCC PR 54412).** Not a problem with current
   MinGW GCC: it spills 32- and 64-byte vectors with unaligned moves
   (`vmovupd`; 88 stack spills in the AVX2 spill kernel, 40 in AVX3, no
   aligned ones, inspected with MinGW GCC 16), and the spill test passed on
   real Zen 4 hardware under Rtools GCC 14.3 for SSE2, SSSE3, SSE4, AVX2,
   AVX3, AVX3_DL and AVX3_ZEN4. No Windows target restriction is applied;
   the spill test stays as a regression guard (`HIGHWAY_R_BROKEN_TARGETS`
   remains 0).
4. **RISC-V NaN payloads.** RISC-V returns a canonical NaN, so `std::exp(NA)`
   is `NaN`. Base R's `math1()` copies NaN inputs to the output for this
   reason; the demo kernels, templates and vignette now do the same.
5. **Single compiled target.** When `HWY_TARGETS` has one bit (e.g. the lean
   set on an Apple M1), Highway turns `HWY_DYNAMIC_DISPATCH` into a direct
   call that never touches the chosen-target state. Tests that inspect that
   state use multi-target kernels.
6. **Apple clang 21 crash** ("Cannot select: v4bf16 = bitcast") compiling
   `LoadN` for 2-lane `bfloat16_t` on `NEON_BF16` (upstream `memory_test`).
   Upstream issue; documented in the vignette's troubleshooting section.
7. **Runner hardware seen in CI** (real, not emulated): AMD EPYC 9V45
   (Zen 5; AVX3_ZEN4) and Intel Xeon Platinum 8573C (Emerald Rapids;
   AVX3_SPR) on ubuntu-latest; EPYC 9V74 (Zen 4) on windows-latest, with or
   without AVX-512 exposed depending on the VM; Neoverse N2 (SVE2_128) on
   ubuntu-24.04-arm; Core i7-8700B (AVX2) on macos-15-intel; Apple M1 (NEON)
   on macos-latest. Ubuntu 26.04's GCC 15.2 defaults to x86-64-v2, so SSE2 is
   not compiled there (it is on Windows and CRAN's Debian).
8. **Intel SDE licence.** The SDE 10.13.1 kit carries the Intel Simplified
   Software License (Version February 2020): use and unmodified
   redistribution permitted; no reverse engineering; no restriction on
   automated use. The download page's click-through was accepted on the
   maintainer's behalf (with permission). CI downloads the kit at run time
   from Intel's mirror (`downloadmirror.intel.com/924984/...`) and checks
   Intel's published SHA-256; nothing is redistributed. The oldest emulated
   CPU is Haswell: on Ubuntu 26.04 runners even the dynamic loader uses AVX,
   so emulating Westmere faults before R starts. Under SDE, Haswell (AVX2),
   Skylake-X (AVX3), Ice Lake (AVX3_DL), Sapphire Rapids (AVX3_SPR) and
   Diamond Rapids (AVX10_2) each dispatched the expected target and passed.
9. **`LICENSE` stub.** R's `BSD_3_clause` template has three fields (`YEAR`,
   `COPYRIGHT HOLDER`, `ORGANIZATION`), not the two in the brief; without
   `ORGANIZATION`, R CMD check gives a NOTE.
10. **`use_highway()` needs `-I.`**, confirmed: `foreach_target.h` re-includes
    `HWY_TARGET_INCLUDE` relative to its own directory and the `-I` path, not
    the including file's directory.
