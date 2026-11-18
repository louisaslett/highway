// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. Turns C++ exceptions into R errors at a
// .Call boundary, for packages that use the plain R C API rather than Rcpp or
// cpp11 (which do this themselves).
//
//     #include "highway/r_guard.h"
//
//     extern "C" SEXP mypkg_exp(SEXP x) {
//       const R_xlen_t n = Rf_xlength(x);
//       SEXP out = PROTECT(Rf_allocVector(REALSXP, n));   // R API outside
//       const double* px = REAL(x);
//       double* pout = REAL(out);
//       HIGHWAY_R_BEGIN
//         CallExpKernel(px, pout, static_cast<size_t>(n)); // no R API inside
//       HIGHWAY_R_END
//       UNPROTECT(1);
//       return out;
//     }
//
// Rules:
// * Only main-thread code may use these macros (Rf_error() is R API).
// * Do not call the R API between the macros: an R error there would
//   longjmp over C++ destructors.
// * C++ objects with destructors that must run belong between the macros (they
//   are destroyed before Rf_error() is called), not in the enclosing scope.
// * Rf_error() is called only after the try/catch has been left, never from
//   inside a catch block.

#ifndef HIGHWAY_R_GUARD_H_
#define HIGHWAY_R_GUARD_H_

#include <R_ext/Error.h>  // Rf_error (public R API)

#include "highway/c_guard.h"

#define HIGHWAY_R_BEGIN                                          \
  {                                                              \
    char highway_r_buf_[1024];                                   \
    int highway_r_status_ = HIGHWAY_STATUS_OK;                   \
    HIGHWAY_C_BEGIN(highway_r_buf_, sizeof(highway_r_buf_))
#define HIGHWAY_R_END                                            \
    HIGHWAY_C_END(highway_r_status_)                             \
    if (highway_r_status_ != HIGHWAY_STATUS_OK)                  \
      Rf_error("%s", highway_r_buf_);                            \
  }

#endif  // HIGHWAY_R_GUARD_H_
