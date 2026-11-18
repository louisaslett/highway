/* Example SIMD kernels for package '{{pkg}}', created by
 * highway::use_highway(language = "c"). Implemented in C++ in
 * highway-kernels.cpp and exposed as plain C functions, so this header can be
 * included from .c files.
 *
 * Conventions for every function:
 *   - arguments are raw pointers and lengths; no R objects, no C++ types;
 *   - the return value is a status code from "highway/c_status.h":
 *     HIGHWAY_STATUS_OK (0) on success, another value on failure;
 *   - on failure a NUL-terminated message is written to err (err_len bytes;
 *     err may be NULL if err_len is 0);
 *   - no C++ exception ever escapes, and no R API is called, so the functions
 *     are safe to call from any thread and through C function pointers.
 * Turn a failure into an R error on R's main thread, for example:
 *     char err[256];
 *     if ({{prefix}}_hwy_add(a, b, out, n, err, sizeof err) != HIGHWAY_STATUS_OK)
 *       Rf_error("%s", err);
 */

#ifndef {{PREFIX}}_HIGHWAY_KERNELS_H_
#define {{PREFIX}}_HIGHWAY_KERNELS_H_

#include <stddef.h>

#include "highway/c_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* out[i] = a[i] + b[i] for i in [0, n). */
int {{prefix}}_hwy_add(const double* a, const double* b, double* out, size_t n,
                       char* err, size_t err_len);

/* out[i] = exp(x[i]) for i in [0, n); matches base R's exp() exactly for NA,
 * NaN, +-Inf and out-of-range values. */
int {{prefix}}_hwy_exp(const double* x, double* out, size_t n, char* err,
                       size_t err_len);

#ifdef __cplusplus
}
#endif

#endif /* {{PREFIX}}_HIGHWAY_KERNELS_H_ */
