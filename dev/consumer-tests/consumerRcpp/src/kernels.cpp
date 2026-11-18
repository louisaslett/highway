// Rcpp and Highway in one translation unit: Rcpp.h is included at the top
// (it is re-included per target, which its include guard makes harmless) and
// the Rcpp-exported functions live in the HWY_ONCE section.
#include <Rcpp.h>

#include <cmath>
#include <string>

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "kernels.cpp"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"
#include "highway/runtime.h"

HWY_BEFORE_NAMESPACE();
namespace kernels {
namespace HWY_NAMESPACE {
namespace {
namespace hn = hwy::HWY_NAMESPACE;

void ExpImpl(const double* HWY_RESTRICT x, double* HWY_RESTRICT out,
             size_t n) {
  const hn::ScalableTag<double> d;
  const size_t N = hn::Lanes(d);
  const auto lo = hn::Set(d, -708.0);
  const auto hi = hn::Set(d, 706.0);
  for (size_t i = 0; i < n; i += N) {
    const size_t m = n - i < N ? n - i : N;
    const auto v = hn::LoadN(d, x + i, m);
    if (hn::AllTrue(d, hn::And(hn::Ge(v, lo), hn::Le(v, hi)))) {
      hn::StoreN(hn::Exp(d, v), d, out + i, m);
    } else {
      for (size_t j = i; j < i + m; ++j) {
        out[j] = std::isnan(x[j]) ? x[j] : std::exp(x[j]);
      }
    }
  }
}

int64_t TargetImpl() { return HWY_TARGET; }

void AbortImpl(int code) {
  if (code != 0) HWY_ABORT("consumerRcpp abort (code %d)", code);
}

}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace kernels
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace kernels {
HWY_EXPORT(ExpImpl);
HWY_EXPORT(TargetImpl);
HWY_EXPORT(AbortImpl);
}  // namespace kernels

// [[Rcpp::export]]
Rcpp::NumericVector rcpp_hwy_exp(Rcpp::NumericVector x) {
  Rcpp::NumericVector out(x.size());
  using namespace kernels;
  HWY_DYNAMIC_DISPATCH(ExpImpl)(x.begin(), out.begin(),
                                static_cast<size_t>(x.size()));
  return out;
}

// [[Rcpp::export]]
std::string rcpp_hwy_target() {
  using namespace kernels;
  return hwy::TargetName(HWY_DYNAMIC_DISPATCH(TargetImpl)());
}

// Rcpp turns the highway::abort_error (a std::exception) into an R error.
// [[Rcpp::export]]
void rcpp_hwy_abort(int code) {
  using namespace kernels;
  HWY_DYNAMIC_DISPATCH(AbortImpl)(code);
}
#endif  // HWY_ONCE
