// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Demo and test kernels for the R package 'highway', compiled once per target
// through hwy/foreach_target.h and selected at run time with
// HWY_DYNAMIC_DISPATCH. No R API is used here, so everything is callable from
// any thread.

#include <float.h>
#include <stddef.h>
#include <stdint.h>

#include <cmath>

#include "demo_kernels.h"

// Re-include this file once per target. The path is relative to an include
// directory, which is why src/Makevars has -I. in PKG_CPPFLAGS.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "demo_kernels.cpp"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"

HWY_BEFORE_NAMESPACE();
namespace highway_demo {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

// hn::Exp is documented as accurate to 1 ULP on [-DBL_MAX, 706]. Below -708
// the result is subnormal, so the SIMD path is used on [-708, 706] only and
// any other lane (including NaN and NA, for which every comparison is false)
// is computed with std::exp, which is what R's exp() calls.
constexpr double kExpLo = -708.0;
constexpr double kExpHi = 706.0;

// hn::Log is documented as accurate to 4 ULP on (0, DBL_MAX]. The SIMD path is
// used for normal positive inputs only; 0, negatives, subnormals, +Inf, NaN
// and NA go through std::log.
constexpr double kLogLo = DBL_MIN;
constexpr double kLogHi = DBL_MAX;

void ExpImpl(const double* HWY_RESTRICT in, double* HWY_RESTRICT out,
             size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, kExpLo);
  const auto hi = hn::Set(d, kExpHi);
  size_t i = 0;
  for (; i + N <= n; i += N) {
    const auto v = hn::LoadU(d, in + i);  // R vectors need not be aligned
    if (HWY_LIKELY(hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi))))) {
      hn::StoreU(hn::Exp(d, v), d, out + i);
    } else {
      for (size_t j = i; j < i + N; ++j) out[j] = std::exp(in[j]);
    }
  }
  if (i < n) {
    const size_t rem = n - i;
    // Lanes past `rem` are zero, which is in range; they are not stored.
    const auto v = hn::LoadN(d, in + i, rem);
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Exp(d, v), d, out + i, rem);
    } else {
      for (size_t j = i; j < n; ++j) out[j] = std::exp(in[j]);
    }
  }
}

void LogImpl(const double* HWY_RESTRICT in, double* HWY_RESTRICT out,
             size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, kLogLo);
  const auto hi = hn::Set(d, kLogHi);
  size_t i = 0;
  for (; i + N <= n; i += N) {
    const auto v = hn::LoadU(d, in + i);
    if (HWY_LIKELY(hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi))))) {
      hn::StoreU(hn::Log(d, v), d, out + i);
    } else {
      for (size_t j = i; j < i + N; ++j) out[j] = std::log(in[j]);
    }
  }
  if (i < n) {
    const size_t rem = n - i;
    // Lanes past `rem` are 1.0 (in range); they are not stored.
    const auto v = hn::LoadNOr(hn::Set(d, 1.0), d, in + i, rem);
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Log(d, v), d, out + i, rem);
    } else {
      for (size_t j = i; j < n; ++j) out[j] = std::log(in[j]);
    }
  }
}

// The same loop as R's own sum() for doubles (src/main/summary.c: rsum),
// including its long double accumulator, so NA/NaN/Inf give R's answer.
double SumLikeR(const double* in, size_t n, bool na_rm) {
  long double s = 0.0;
  for (size_t i = 0; i < n; ++i) {
    if (!na_rm || !std::isnan(in[i])) s += in[i];
  }
  return static_cast<double>(s);
}

// SIMD sum with four independent accumulators. If any element is not finite,
// the result is recomputed with SumLikeR so that NA vs NaN and Inf behave as
// in R. For finite input the result agrees with sum() up to rounding: R
// accumulates in long double where the platform has one.
double SumImpl(const double* HWY_RESTRICT in, size_t n, bool na_rm) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  auto s0 = hn::Zero(d);
  auto s1 = hn::Zero(d);
  auto s2 = hn::Zero(d);
  auto s3 = hn::Zero(d);
  auto bad = hn::MaskFalse(d);
  size_t i = 0;
  for (; i + 4 * N <= n; i += 4 * N) {
    const auto v0 = hn::LoadU(d, in + i);
    const auto v1 = hn::LoadU(d, in + i + N);
    const auto v2 = hn::LoadU(d, in + i + 2 * N);
    const auto v3 = hn::LoadU(d, in + i + 3 * N);
    bad = hn::Or(bad, hn::Or(hn::Or(hn::Not(hn::IsFinite(v0)),
                                    hn::Not(hn::IsFinite(v1))),
                             hn::Or(hn::Not(hn::IsFinite(v2)),
                                    hn::Not(hn::IsFinite(v3)))));
    s0 = hn::Add(s0, v0);
    s1 = hn::Add(s1, v1);
    s2 = hn::Add(s2, v2);
    s3 = hn::Add(s3, v3);
  }
  for (; i + N <= n; i += N) {
    const auto v = hn::LoadU(d, in + i);
    bad = hn::Or(bad, hn::Not(hn::IsFinite(v)));
    s0 = hn::Add(s0, v);
  }
  if (i < n) {
    const auto v = hn::LoadN(d, in + i, n - i);  // zero-filled
    bad = hn::Or(bad, hn::Not(hn::IsFinite(v)));
    s1 = hn::Add(s1, v);
  }
  if (!hn::AllFalse(d, bad)) return SumLikeR(in, n, na_rm);
  return hn::ReduceSum(d, hn::Add(hn::Add(s0, s1), hn::Add(s2, s3)));
}

int64_t TargetImpl() { return HWY_TARGET; }

size_t LanesF64Impl() { return hn::Lanes(hn::ScalableTag<double>()); }

// --- Test kernels ----------------------------------------------------------

#define HIGHWAY_DEMO_REPEAT40(M)                                       \
  M(0) M(1) M(2) M(3) M(4) M(5) M(6) M(7) M(8) M(9) M(10) M(11) M(12) \
  M(13) M(14) M(15) M(16) M(17) M(18) M(19) M(20) M(21) M(22) M(23)   \
  M(24) M(25) M(26) M(27) M(28) M(29) M(30) M(31) M(32) M(33) M(34)   \
  M(35) M(36) M(37) M(38) M(39)

// 40 accumulators stay live across every loop iteration, more than the 16
// (AVX2, NEON) or 32 (AVX-512) vector registers, so the compiler must spill
// vectors to the stack. This exercises the stack alignment of 32- and 64-byte
// vectors (GCC PR 54412 on Windows). With integer-valued input every
// operation is exact, so the result must equal SpillChecksumScalar exactly.
HWY_NOINLINE double SpillImpl(const double* HWY_RESTRICT in, size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
#define HIGHWAY_DEMO_DECL(k) auto acc##k = hn::Zero(d);
  HIGHWAY_DEMO_REPEAT40(HIGHWAY_DEMO_DECL)
#undef HIGHWAY_DEMO_DECL
  for (size_t i = 0; i < n; i += N) {
    const auto x = hn::LoadN(d, in + i, n - i);
#define HIGHWAY_DEMO_ACC(k) \
  acc##k = hn::MulAdd(x, hn::Set(d, static_cast<double>((k) + 1)), acc##k);
    HIGHWAY_DEMO_REPEAT40(HIGHWAY_DEMO_ACC)
#undef HIGHWAY_DEMO_ACC
  }
  auto total = hn::Zero(d);
#define HIGHWAY_DEMO_SUM(k) \
  total = hn::MulAdd(acc##k, hn::Set(d, static_cast<double>((k) % 3 + 1)), total);
  HIGHWAY_DEMO_REPEAT40(HIGHWAY_DEMO_SUM)
#undef HIGHWAY_DEMO_SUM
  return hn::ReduceSum(d, total);
}

void AbortImpl(int code) {
  if (code != 0) {
    HWY_ABORT("highway demo kernel: abort requested (code %d)", code);
  }
}

}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace highway_demo
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace highway_demo {

HWY_EXPORT(ExpImpl);
HWY_EXPORT(LogImpl);
HWY_EXPORT(SumImpl);
HWY_EXPORT(TargetImpl);
HWY_EXPORT(LanesF64Impl);
HWY_EXPORT(SpillImpl);
HWY_EXPORT(AbortImpl);

void Exp(const double* in, double* out, size_t n) {
  HWY_DYNAMIC_DISPATCH(ExpImpl)(in, out, n);
}

void Log(const double* in, double* out, size_t n) {
  HWY_DYNAMIC_DISPATCH(LogImpl)(in, out, n);
}

double Sum(const double* in, size_t n, bool na_rm) {
  return HWY_DYNAMIC_DISPATCH(SumImpl)(in, n, na_rm);
}

int64_t DispatchedTarget() { return HWY_DYNAMIC_DISPATCH(TargetImpl)(); }

size_t LanesF64() { return HWY_DYNAMIC_DISPATCH(LanesF64Impl)(); }

int64_t CompiledTargets() { return HWY_TARGETS; }

double SpillChecksum(const double* in, size_t n) {
  return HWY_DYNAMIC_DISPATCH(SpillImpl)(in, n);
}

double SpillChecksumScalar(const double* in, size_t n) {
  double total = 0.0;
  for (int k = 0; k < 40; ++k) {
    double acc = 0.0;
    for (size_t i = 0; i < n; ++i) acc += in[i] * static_cast<double>(k + 1);
    total += acc * static_cast<double>(k % 3 + 1);
  }
  return total;
}

void AbortInKernel(int code) { HWY_DYNAMIC_DISPATCH(AbortImpl)(code); }

}  // namespace highway_demo
#endif  // HWY_ONCE
