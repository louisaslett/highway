// Test kernels for consumer package {{P}}. Unlike highway-kernels.cpp they use
// Highway's default target set, so the all-targets test covers every target
// the compiler can generate.

#include <stddef.h>
#include <stdint.h>

#include <cmath>

#include "{{P}}_extra.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "{{P}}_extra.cpp"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"

HWY_BEFORE_NAMESPACE();
namespace {{P}}_extra {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

int64_t TargetImpl() { return HWY_TARGET; }

void AbortImpl(int code) {
  if (code != 0) HWY_ABORT("{{P}} kernel abort (code %d)", code);
}

// log() with Highway's Log for normal positive lanes, std::log otherwise.
void LogImpl(const double* HWY_RESTRICT in, double* HWY_RESTRICT out,
             size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, 2.2250738585072014e-308);
  const auto hi = hn::Set(d, 1.7976931348623157e308);
  for (size_t i = 0; i < n; i += N) {
    const size_t m = n - i < N ? n - i : N;
    const auto v = hn::LoadNOr(hn::Set(d, 1.0), d, in + i, m);
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Log(d, v), d, out + i, m);
    } else {
      for (size_t j = i; j < i + m; ++j) out[j] = std::log(in[j]);
    }
  }
}

}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace {{P}}_extra
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace {{P}}_extra {
HWY_EXPORT(TargetImpl);
HWY_EXPORT(AbortImpl);
HWY_EXPORT(LogImpl);

int64_t dispatched_target() { return HWY_DYNAMIC_DISPATCH(TargetImpl)(); }
int64_t compiled_targets() { return HWY_TARGETS; }
void abort_kernel(int code) { HWY_DYNAMIC_DISPATCH(AbortImpl)(code); }
void log_doubles(const double* in, double* out, size_t n) {
  HWY_DYNAMIC_DISPATCH(LogImpl)(in, out, n);
}
}  // namespace {{P}}_extra
#endif  // HWY_ONCE
