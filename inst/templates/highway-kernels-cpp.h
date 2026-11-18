// Example SIMD kernels for package '{{pkg}}', created by highway::use_highway().
// Defined in highway-kernels.cpp. They use no R API and may be called from
// any thread. They may throw highway::abort_error (a std::exception) if
// Highway detects an internal error, so call them where C++ exceptions are
// caught: Rcpp and cpp11 do this for you; with plain .Call use
// HIGHWAY_R_BEGIN / HIGHWAY_R_END from "highway/r_guard.h".

#ifndef {{PREFIX}}_HIGHWAY_KERNELS_H_
#define {{PREFIX}}_HIGHWAY_KERNELS_H_

#include <stddef.h>

namespace {{prefix}}_hwy {

// out[i] = a[i] + b[i] for i in [0, n).
void add_doubles(const double* a, const double* b, double* out, size_t n);

// out[i] = exp(x[i]) for i in [0, n); matches base R's exp() exactly for
// NA, NaN, +-Inf and out-of-range values.
void exp_doubles(const double* x, double* out, size_t n);

}  // namespace {{prefix}}_hwy

#endif  // {{PREFIX}}_HIGHWAY_KERNELS_H_
