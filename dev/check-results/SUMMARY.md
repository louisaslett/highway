# Pre-submission summary: highway 1.4.0-1

Final commit checked: `bc38cec` (2026-11-18); later commits change only documentation and the DESCRIPTION wording. CI runs:
<https://github.com/louisaslett/highway/actions>.

## Check matrix

`R CMD check --as-cran`; GitHub jobs fail on any NOTE (incoming checks off, as
the only incoming NOTE is "New submission").

| Environment | R | Compiler | Result |
|---|---|---|---|
| local macOS 26 arm64 (Apple M1), incoming checks on | 4.6.2 | Apple clang 21 | 0 / 0 / 1 NOTE (New submission) |
| local strict build `-Wall -Wextra -pedantic -Werror` | 4.6.2 | Apple clang 21 | clean |
| GitHub ubuntu-latest (Ubuntu 26.04, x86-64) | devel, release, 4.5 (oldrel-1), 4.4 (oldrel-2) | GCC 15.2 | 0 / 0 / 0 |
| GitHub ubuntu-24.04-arm (Neoverse N2) | release | GCC 13 | 0 / 0 / 0 |
| GitHub macos-latest (Apple M1) | release | Apple clang 21 | 0 / 0 / 0 |
| GitHub macos-15-intel (Core i7-8700B) | release | Apple clang 17 | 0 / 0 / 0 |
| GitHub windows-latest (EPYC 9V74) | release, devel | Rtools GCC 14.3 | 0 / 0 / 0 |
| R-hub v2: gcc-asan, clang-asan, clang-ubsan, valgrind, gcc16, clang23, lto, donttest, nosuggests, m1-san | devel | various | Status: OK (all ten) |
| QEMU: ppc64le, s390x, riscv64 (Debian testing) | 4.6.2 | GCC 16 | build + tests pass (EMU128) |
| webR (Emscripten 5.0.7) | 4.6.0 | em++ | builds and packages (tests not run) |
| Intel SDE 10.13.1: Haswell, Skylake-X, Ice Lake server, Sapphire Rapids, Diamond Rapids | release | GCC 15.2 | expected target dispatched, full test suite passes on each |
| Sanitizer harness (TSan; ASan+UBSan+LSan) on x86-64 and arm64 Linux, clang and GCC; macOS arm64 locally | n/a | various | no reports |
| Header warning sweep: upstream op and contrib tests (86 TUs), `-O2 -Wall -Wextra -pedantic` | n/a | GCC 16.2, clang 23 (x86-64 C++17/C++20, arm64 C++20) | 0 diagnostics in shipped files or system headers, all six configurations |
| Consumers A, B, C, Rcpp, cpp11 (`R CMD check --as-cran`, incoming off) | release | Linux x86-64/arm64, macOS arm64/x86-64, Windows | Status: OK, all 25 |
| Two consumers in one session (normal, A with RTLD_GLOBAL, B with RTLD_GLOBAL) | release | all five OS runners | pass |
| win-builder: R-devel (r90926), R 4.6.2, R 4.5.3 (tarball from `ba0c234`) | as listed | Rtools GCC 14.3 | 1 NOTE each: "New submission" and "Possibly misspelled words in DESCRIPTION: R's". The wording was changed afterwards (no apostrophe); re-run win-builder R-devel before submitting. Tests dispatched AVX2 and forced AVX2, SSE4, SSSE3, SSE2. |
| mac-builder | | | not run: submission API returned HTTP 502 on 2026-11-18 and 2026-11-19 |
| `spelling::spell_check_package()`, `urlchecker::url_check()` | | | clean |

## Target coverage

"Real" means the code ran on a CPU that supports the target; "SDE" means it
ran under Intel SDE emulation. "Dispatched" means dynamic dispatch chose the
target by itself; "forced" means the tests restricted dispatch to it with
Highway's `SetSupportedTargetsForTest()` and asserted that it ran.

| Target | Executed on |
|---|---|
| SSE2 | Real: windows-latest (EPYC 9V74), forced. (Not compiled on Ubuntu 26.04 or macOS, whose baselines are higher.) |
| SSSE3 | Real: ubuntu-latest (EPYC 9V45, 9V74; Xeon Platinum 8573C), windows-latest, macos-15-intel, forced. SDE: every emulated CPU, forced. |
| SSE4 | As SSSE3. |
| AVX2 | Real: macos-15-intel and one windows-latest VM, dispatched; Ubuntu runners, forced. SDE Haswell, dispatched. |
| AVX3 (AVX-512) | Real: ubuntu-latest (Zen 4/5, Emerald Rapids), windows-latest (Zen 4), forced. SDE Skylake-X, dispatched. |
| AVX3_DL | Real: same CPUs, forced. SDE Ice Lake server, dispatched. |
| AVX3_ZEN4 | Real: ubuntu-latest EPYC 9V45/9V74 and windows-latest EPYC 9V74, dispatched. (SDE cannot emulate AMD.) |
| AVX3_SPR | Real: ubuntu-latest Xeon Platinum 8573C, dispatched. SDE Sapphire Rapids, dispatched. |
| AVX10_2 | SDE Diamond Rapids only, dispatched. No runner has AVX10.2. |
| NEON_WITHOUT_AES | Real: ubuntu-24.04-arm (Neoverse N2), forced. |
| NEON | Real: Apple M1 (local, macos-latest), dispatched; Neoverse N2, forced. |
| NEON_BF16 | Real: Neoverse N2, forced. (M1 has no BF16.) |
| SVE | Real: Neoverse N2, forced. |
| SVE2 | Real: Neoverse N2, forced. |
| SVE2_128 | Real: Neoverse N2, dispatched. |
| SVE_256 | Compiled, **not executed** (needs 256-bit SVE, e.g. Neoverse V1 / Graviton3). |
| EMU128 | QEMU user emulation of ppc64le, s390x, riscv64 (portable code only, so the emulation is of the base ISA). |
| SCALAR | **Not executed** (not compiled into the package's kernels; Highway uses it only where EMU128 is broken). |

The register-spill test (40 live vectors) passed on every executed x86 target
on Windows, Linux and macOS.

## Sizes

| | |
|---|---|
| Source tarball | 0.56 MB |
| Installed, Linux x86-64 (`-g`) | 4.6 MB (`du -sk` 4708 to 4720) |
| Installed, Linux arm64 (`-g`) | 4.7 MB (4840) |
| Installed, Windows | 3.5 MB (3616) |
| Installed, macOS | 3.3 MB (3392 to 3404; `include/` 3.0 MB, `libs/` 0.1 MB) |

CRAN's installed-size NOTE starts at 5 MB. R-hub's gcc16, clang23, lto and
sanitizer images (all with `-g`) reported no size NOTE.

## Consumer install times (seconds, GitHub runners)

| Runner | consumerA | consumerB | consumerC | consumerRcpp | consumerCpp11 |
|---|---|---|---|---|---|
| ubuntu-latest (4 vCPU, x86-64) | 8 | 10 | 7 | 9 | 2 |
| ubuntu-24.04-arm | 11 | 12 | 10 | 12 | 3 |
| macos-latest (M1) | 12 | 13 | 9 | 12 | 6 |
| macos-15-intel | 24 | 25 | 25 | 35 | 8 |
| windows-latest | 26 | 25 | 21 | 23 | 5 |

consumerA uses `HIGHWAY_TARGETS_LEAN` for its generated kernels plus one
test file with all default targets; consumerB uses all default targets
everywhere; consumerCpp11 uses static dispatch only.
