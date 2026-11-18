// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. R-specific configuration of the bundled
// Highway headers. Included by hwy/detect_targets.h (local patch 0003) right
// after hwy/detect_compiler_arch.h, so HWY_ARCH_* and HWY_COMPILER_* are
// available but no target has been selected yet. This is the single place
// where the package restricts targets.

#ifndef HIGHWAY_CONFIG_H_
#define HIGHWAY_CONFIG_H_

// ---------------------------------------------------------------------------
// 1. Architectures whose SIMD op headers are not bundled.
//
// To keep the installed package under CRAN's 5 MB size threshold (CRAN's
// Linux builds include debug information), the op headers for POWER/s390x
// (ppc_vsx-inl.h), RISC-V (rvv-inl.h), LoongArch (loongarch_*-inl.h) and
// WebAssembly (wasm_*-inl.h) are not shipped. On those architectures Highway
// is restricted to its portable EMU128 target (or SCALAR where Highway itself
// considers EMU128 broken for the compiler). Code compiles and gives the same
// results; it is simply not vectorised with native instructions.
#if HWY_ARCH_PPC || HWY_ARCH_S390X || HWY_ARCH_RISCV || HWY_ARCH_LOONGARCH || \
    HWY_ARCH_WASM
#if !defined(HWY_COMPILE_ONLY_EMU128) && !defined(HWY_COMPILE_ONLY_SCALAR)
#define HWY_COMPILE_ONLY_EMU128
#endif
#define HIGHWAY_NATIVE_SIMD_BUNDLED 0
#else
#define HIGHWAY_NATIVE_SIMD_BUNDLED 1
#endif

// ---------------------------------------------------------------------------
// 2. Toolchain-specific target restrictions.
//
// OR-ed into Highway's own HWY_BROKEN_TARGETS (local patch 0003), so it
// applies even when a consumer defines HWY_DISABLED_TARGETS. Currently none.
#ifndef HIGHWAY_R_BROKEN_TARGETS
#define HIGHWAY_R_BROKEN_TARGETS 0
#endif

// ---------------------------------------------------------------------------
// 3. Optional lean target set, to reduce install time.
//
// Dynamic dispatch compiles every kernel once per target; Highway's default
// is up to 9 targets on x86-64 and 7 on Linux arm64. Defining
// HIGHWAY_TARGETS_LEAN before the first Highway include of a translation unit
// keeps the targets that matter most for typical numeric kernels:
//   x86-64: SSE2, SSE4, AVX2, AVX3 (AVX-512 F/BW/CD/DQ/VL)
//   arm64:  NEON (with and without AES), SVE, SVE2
// and drops SSSE3, AVX3_DL, AVX3_ZEN4, AVX3_SPR, AVX10_2, NEON_BF16, SVE_256
// and SVE2_128. CPUs with those features still run the best kept target
// (e.g. AVX3 on Sapphire Rapids). The compile-time baseline is never dropped.
// Ignored if the translation unit defines HWY_DISABLED_TARGETS itself.
#define HIGHWAY_LEAN_EXCLUDED_TARGETS                                       \
  (HWY_SSSE3 | HWY_AVX3_DL | HWY_AVX3_ZEN4 | HWY_AVX3_SPR | HWY_AVX10_2 | \
   HWY_NEON_BF16 | HWY_SVE_256 | HWY_SVE2_128)
#if defined(HIGHWAY_TARGETS_LEAN) && !defined(HWY_DISABLED_TARGETS)
#define HWY_DISABLED_TARGETS \
  (HIGHWAY_LEAN_EXCLUDED_TARGETS & ~(HWY_BASELINE_TARGETS))
#endif

#endif  // HIGHWAY_CONFIG_H_
