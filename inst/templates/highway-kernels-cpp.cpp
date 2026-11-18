// Example SIMD kernels for package '{{pkg}}', created by highway::use_highway().
//
// Each kernel is compiled once per SIMD target (SSE2, AVX2, AVX-512, NEON,
// ...) and HWY_DYNAMIC_DISPATCH picks the best one for the CPU at run time.
// No CPU-specific compiler flags are needed, or allowed on CRAN.
//
// The functions below use no R API, so they may be called from any thread.
// Call them from Rcpp, cpp11 or .Call code; with plain .Call, wrap the call in
// HIGHWAY_R_BEGIN / HIGHWAY_R_END from "highway/r_guard.h" so that a C++
// exception becomes an R error.

// Optional: compile a smaller set of targets to shorten installation. Must
// come before the first Highway include. See vignette("highway").
#define HIGHWAY_TARGETS_LEAN

#include <stddef.h>

#include <cmath>

#include "highway-kernels.h"

// This file is re-included once per target; the name is resolved against the
// include path, which is why src/Makevars has PKG_CPPFLAGS = -I.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "highway-kernels.cpp"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"

HWY_BEFORE_NAMESPACE();
namespace {{prefix}}_hwy {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

// out[i] = a[i] + b[i]. R vectors are not guaranteed to be aligned for SIMD,
// so use unaligned loads and stores; LoadN/StoreN handle the last partial
// vector.
void AddImpl(const double* HWY_RESTRICT a, const double* HWY_RESTRICT b,
             double* HWY_RESTRICT out, size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= n; i += N) {
    hn::StoreU(hn::Add(hn::LoadU(d, a + i), hn::LoadU(d, b + i)), d, out + i);
  }
  if (i < n) {
    const size_t rem = n - i;
    hn::StoreN(hn::Add(hn::LoadN(d, a + i, rem), hn::LoadN(d, b + i, rem)), d,
               out + i, rem);
  }
}

// out[i] = exp(x[i]). Highway's Exp() is accurate to 1 ULP on [-708, 706]
// for double; vectors with any lane outside that range, or NA/NaN, are done
// lane by lane like base R's exp(): std::exp, but NaN inputs (NA included)
// are copied unchanged so that NA stays NA on every platform.
void ExpImpl(const double* HWY_RESTRICT x, double* HWY_RESTRICT out,
             size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, -708.0);
  const auto hi = hn::Set(d, 706.0);
  size_t i = 0;
  for (; i < n; i += N) {
    const size_t m = n - i < N ? n - i : N;
    const auto v = hn::LoadN(d, x + i, m);  // lanes past m are zero
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Exp(d, v), d, out + i, m);
    } else {
      for (size_t j = i; j < i + m; ++j) {
        out[j] = std::isnan(x[j]) ? x[j] : std::exp(x[j]);
      }
    }
  }
}

}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace {{prefix}}_hwy
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace {{prefix}}_hwy {

HWY_EXPORT(AddImpl);
HWY_EXPORT(ExpImpl);

void add_doubles(const double* a, const double* b, double* out, size_t n) {
  HWY_DYNAMIC_DISPATCH(AddImpl)(a, b, out, n);
}

void exp_doubles(const double* x, double* out, size_t n) {
  HWY_DYNAMIC_DISPATCH(ExpImpl)(x, out, n);
}

}  // namespace {{prefix}}_hwy
#endif  // HWY_ONCE
