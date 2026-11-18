// Static dispatch: the code is compiled once, for the compiler's baseline
// target (SSE2 on x86-64, NEON on arm64), with no runtime dispatch and no
// compiled Highway runtime (HWY_HEADER_ONLY, see src/Makevars).
#include <cpp11.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"

namespace hn = hwy::HWY_NAMESPACE;

[[cpp11::register]]
cpp11::writable::doubles cpp11_hwy_exp(cpp11::doubles x) {
  const size_t n = static_cast<size_t>(x.size());
  cpp11::writable::doubles out(x.size());
  const double* px = REAL(x.data());
  double* po = REAL(out.data());
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, -708.0);
  const auto hi = hn::Set(d, 706.0);
  for (size_t i = 0; i < n; i += N) {
    const size_t m = n - i < N ? n - i : N;
    const auto v = hn::LoadN(d, px + i, m);
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Exp(d, v), d, po + i, m);
    } else {
      for (size_t j = i; j < i + m; ++j) {
        po[j] = std::isnan(px[j]) ? px[j] : std::exp(px[j]);
      }
    }
  }
  return out;
}

[[cpp11::register]]
std::string cpp11_hwy_target() { return hwy::TargetName(HWY_TARGET); }

// In header-only mode the patched Abort() also throws highway::abort_error;
// cpp11 turns it into an R error.
[[cpp11::register]]
void cpp11_hwy_abort(int code) {
  if (code != 0) HWY_ABORT("consumerCpp11 abort (code %d)", code);
}

// The integer example from the vignette (section 6): sum skipping NA
// (INT_MIN), accumulated in 64-bit integers.
double sum_int_na_rm(const int32_t* x, size_t n) {
  const hn::ScalableTag<int32_t> d32;
  const hn::Repartition<int64_t, decltype(d32)> d64;  // half as many lanes
  const size_t N = hn::Lanes(d32);
  const auto na = hn::Set(d32, std::numeric_limits<int32_t>::min());
  auto acc = hn::Zero(d64);
  size_t i = 0;
  for (; i + N <= n; i += N) {
    const auto v = hn::LoadU(d32, x + i);
    const auto clean = hn::IfThenZeroElse(hn::Eq(v, na), v);  // NA -> 0
    acc = hn::Add(acc, hn::SumsOf2(clean));  // adjacent pairs, as int64
  }
  int64_t total = hn::ReduceSum(d64, acc);
  for (; i < n; ++i) {
    if (x[i] != std::numeric_limits<int32_t>::min()) total += x[i];
  }
  return static_cast<double>(total);
}

[[cpp11::register]]
double cpp11_hwy_sum_int(cpp11::integers x) {
  return sum_int_na_rm(INTEGER(x.data()), static_cast<size_t>(x.size()));
}
