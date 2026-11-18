// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// .Call entry points of the R package 'highway'. Every entry point allocates
// its R objects first and then runs C++ between HIGHWAY_R_BEGIN and
// HIGHWAY_R_END, so a C++ exception (including highway::abort_error) becomes
// an R error without unwinding through R or longjmp-ing over C++ frames.

#define R_NO_REMAP
#include <Rinternals.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <atomic>
#include <thread>
#include <vector>

#include "demo_kernels.h"
#include "highway/c_guard.h"
#include "highway/r_guard.h"
#include "highway/runtime.h"
#include "hwy/base.h"
#include "hwy/targets.h"

namespace {

SEXP target_names(int64_t targets) {
  const R_xlen_t n = static_cast<R_xlen_t>(highway::count_targets(targets));
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  R_xlen_t i = 0;
  highway::for_each_target(targets, [&](int64_t, const char* name) {
    SET_STRING_ELT(out, i++, Rf_mkChar(name));
  });
  UNPROTECT(1);
  return out;
}

int64_t target_bit(const char* name) {
  int64_t bit = 0;
  highway::for_each_target(INT64_MAX, [&](int64_t b, const char* nm) {
                             if (strcmp(nm, name) == 0) bit = b;
                           });
  return bit;
}

size_t checked_length(SEXP x) {
  if (TYPEOF(x) != REALSXP) Rf_error("'x' must be a double vector");
  return static_cast<size_t>(Rf_xlength(x));
}

}  // namespace

extern "C" {

SEXP hwy_version(void) {
  SEXP out = PROTECT(Rf_allocVector(INTSXP, 3));
  INTEGER(out)[0] = HWY_MAJOR;
  INTEGER(out)[1] = HWY_MINOR;
  INTEGER(out)[2] = HWY_PATCH;
  UNPROTECT(1);
  return out;
}

SEXP hwy_supported_targets(void) {
  int64_t targets = 0;
  HIGHWAY_R_BEGIN
    targets = hwy::SupportedTargets();
  HIGHWAY_R_END
  return target_names(targets);
}

SEXP hwy_compiled_targets(void) {
  return target_names(highway_demo::CompiledTargets());
}

SEXP hwy_dispatched_target(void) {
  int64_t target = 0;
  HIGHWAY_R_BEGIN
    target = highway_demo::DispatchedTarget();
  HIGHWAY_R_END
  return target_names(target);
}

SEXP hwy_lanes_f64(void) {
  size_t lanes = 0;
  HIGHWAY_R_BEGIN
    lanes = highway_demo::LanesF64();
  HIGHWAY_R_END
  return Rf_ScalarInteger(static_cast<int>(lanes));
}

SEXP hwy_demo_exp(SEXP x) {
  const size_t n = checked_length(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, static_cast<R_xlen_t>(n)));
  const double* px = REAL(x);
  double* pout = REAL(out);
  HIGHWAY_R_BEGIN
    highway_demo::Exp(px, pout, n);
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

SEXP hwy_demo_log(SEXP x) {
  const size_t n = checked_length(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, static_cast<R_xlen_t>(n)));
  const double* px = REAL(x);
  double* pout = REAL(out);
  HIGHWAY_R_BEGIN
    highway_demo::Log(px, pout, n);
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

SEXP hwy_demo_sum(SEXP x, SEXP na_rm) {
  const size_t n = checked_length(x);
  const bool rm = Rf_asLogical(na_rm) == TRUE;
  const double* px = REAL(x);
  double result = 0.0;
  HIGHWAY_R_BEGIN
    result = highway_demo::Sum(px, n, rm);
  HIGHWAY_R_END
  return Rf_ScalarReal(result);
}

// --- Test hooks (not exported; used by inst/tinytest) -----------------------

// Restricts dynamic dispatch to the named target (Highway's
// SetSupportedTargetsForTest); "" restores normal detection. Returns TRUE if
// the target is both compiled and supported. Test-only: not thread-safe.
SEXP hwy_test_force_target(SEXP name) {
  if (!Rf_isString(name) || Rf_xlength(name) != 1)
    Rf_error("'name' must be a single string");
  const char* nm = CHAR(STRING_ELT(name, 0));
  int ok = 0;
  HIGHWAY_R_BEGIN
    if (nm[0] == '\0') {
      hwy::SetSupportedTargetsForTest(0);
      ok = 1;
    } else {
      hwy::SetSupportedTargetsForTest(0);
      const int64_t bit = target_bit(nm);
      const int64_t usable =
          hwy::SupportedTargets() & highway_demo::CompiledTargets();
      if (bit != 0 && (usable & bit) != 0) {
        hwy::SetSupportedTargetsForTest(bit);
        ok = 1;
      }
    }
  HIGHWAY_R_END
  return Rf_ScalarLogical(ok);
}

// Returns c(simd, scalar) checksums of the register-spill kernel.
SEXP hwy_test_spill(SEXP x) {
  const size_t n = checked_length(x);
  const double* px = REAL(x);
  double simd = 0.0, scalar = 0.0;
  HIGHWAY_R_BEGIN
    simd = highway_demo::SpillChecksum(px, n);
    scalar = highway_demo::SpillChecksumScalar(px, n);
  HIGHWAY_R_END
  SEXP out = PROTECT(Rf_allocVector(REALSXP, 2));
  REAL(out)[0] = simd;
  REAL(out)[1] = scalar;
  UNPROTECT(1);
  return out;
}

// HWY_ABORT on R's main thread: must surface as an R error via r_guard.h.
SEXP hwy_test_abort_main(SEXP code) {
  const int c = Rf_asInteger(code);
  HIGHWAY_R_BEGIN
    highway_demo::AbortInKernel(c);
  HIGHWAY_R_END
  return R_NilValue;
}

// HWY_ABORT on a worker std::thread, caught by highway::c_guard on that
// thread. Returns list(status, message).
SEXP hwy_test_abort_worker(SEXP code) {
  const int c = Rf_asInteger(code);
  int status = -1;
  char msg[256];
  msg[0] = '\0';
  HIGHWAY_R_BEGIN
    std::thread worker([&] {
      status = highway::c_guard([&] { highway_demo::AbortInKernel(c); }, msg,
                                sizeof(msg));
    });
    worker.join();
  HIGHWAY_R_END
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 2));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(status));
  SET_VECTOR_ELT(out, 1, Rf_mkString(msg));
  SEXP nms = PROTECT(Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(nms, 0, Rf_mkChar("status"));
  SET_STRING_ELT(nms, 1, Rf_mkChar("message"));
  Rf_setAttrib(out, R_NamesSymbol, nms);
  UNPROTECT(2);
  return out;
}

// Runs the exp kernel on `nthreads` std::threads at once, each over all of x
// into its own output column. If `cold`, the dispatch state is reset first
// so that the threads race on Highway's first-call target selection. Returns
// an n x nthreads matrix.
SEXP hwy_test_threads(SEXP x, SEXP nthreads, SEXP cold) {
  const size_t n = checked_length(x);
  const int nt = Rf_asInteger(nthreads);
  if (nt < 1 || nt > 2) Rf_error("'nthreads' must be 1 or 2 (CRAN limit)");
  const bool reset = Rf_asLogical(cold) == TRUE;
  SEXP out = PROTECT(Rf_allocMatrix(REALSXP, static_cast<int>(n), nt));
  const double* px = REAL(x);
  double* pout = REAL(out);
  int status[2] = {HIGHWAY_STATUS_OK, HIGHWAY_STATUS_OK};
  char msg[2][256];
  msg[0][0] = msg[1][0] = '\0';
  HIGHWAY_R_BEGIN
    if (reset) hwy::GetChosenTarget().DeInit();
    std::atomic<int> ready{0};
    std::atomic<bool> cancel{false};
    std::vector<std::thread> pool;
    // Joins whatever was started, also if starting a later thread throws.
    struct Joiner {
      std::vector<std::thread>& threads;
      std::atomic<bool>& cancel;
      ~Joiner() {
        cancel.store(true);
        for (auto& th : threads) {
          if (th.joinable()) th.join();
        }
      }
    } joiner{pool, cancel};
    pool.reserve(static_cast<size_t>(nt));
    for (int t = 0; t < nt; ++t) {
      pool.emplace_back([&, t] {
        status[t] = highway::c_guard(
            [&] {
              ready.fetch_add(1);
              // Spin until every thread is ready, so the first calls overlap.
              while (ready.load() < nt && !cancel.load()) {
              }
              highway_demo::Exp(px, pout + static_cast<size_t>(t) * n, n);
            },
            msg[t], sizeof(msg[t]));
      });
    }
    for (auto& th : pool) th.join();  // normal path; Joiner covers the rest
  HIGHWAY_R_END
  for (int t = 0; t < nt; ++t) {
    if (status[t] != HIGHWAY_STATUS_OK)
      Rf_error("worker thread %d failed (status %d): %s", t, status[t], msg[t]);
  }
  UNPROTECT(1);
  return out;
}

}  // extern "C"
