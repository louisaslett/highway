// .Call glue for consumer package {{P}}, using the plain R C API and
// highway/r_guard.h. Every C symbol is prefixed with the package name so that
// two consumers loaded with dyn.load(local = FALSE) cannot interpose each
// other's glue; the test is about Highway's own state.

#define R_NO_REMAP
#include <Rinternals.h>

#include <stdint.h>
#include <string.h>

#include <atomic>
#include <thread>
#include <vector>

#include "highway-kernels.h"
#include "highway/c_guard.h"
#include "highway/r_guard.h"
#include "highway/runtime.h"
#include "hwy/targets.h"
#include "{{P}}_extra.h"

namespace {

std::atomic<int> g_handler_calls{0};

// Custom abort handler: counts calls and returns, after which Highway throws
// highway::abort_error anyway. Thread-safe and free of R API calls.
void counting_handler(const char*, int, const char*) {
  g_handler_calls.fetch_add(1);
}

SEXP names_of(int64_t targets) {
  const R_xlen_t n = static_cast<R_xlen_t>(highway::count_targets(targets));
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  R_xlen_t i = 0;
  highway::for_each_target(targets, [&](int64_t, const char* name) {
    SET_STRING_ELT(out, i++, Rf_mkChar(name));
  });
  UNPROTECT(1);
  return out;
}

int64_t bit_of(const char* name) {
  int64_t bit = 0;
  highway::for_each_target(INT64_MAX, [&](int64_t b, const char* nm) {
    if (strcmp(nm, name) == 0) bit = b;
  });
  return bit;
}

}  // namespace

extern "C" {

SEXP {{P}}_add(SEXP x, SEXP y) {
  const R_xlen_t n = Rf_xlength(x);
  if (Rf_xlength(y) != n) Rf_error("lengths differ");
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  const double* px = REAL(x);
  const double* py = REAL(y);
  double* po = REAL(out);
  HIGHWAY_R_BEGIN
    {{P}}_hwy::add_doubles(px, py, po, static_cast<size_t>(n));
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

SEXP {{P}}_exp(SEXP x) {
  const R_xlen_t n = Rf_xlength(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  const double* px = REAL(x);
  double* po = REAL(out);
  HIGHWAY_R_BEGIN
    {{P}}_hwy::exp_doubles(px, po, static_cast<size_t>(n));
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

SEXP {{P}}_log(SEXP x) {
  const R_xlen_t n = Rf_xlength(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  const double* px = REAL(x);
  double* po = REAL(out);
  HIGHWAY_R_BEGIN
    {{P}}_extra::log_doubles(px, po, static_cast<size_t>(n));
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

SEXP {{P}}_targets(void) {
  int64_t sup = 0, disp = 0;
  HIGHWAY_R_BEGIN
    sup = hwy::SupportedTargets();
    disp = {{P}}_extra::dispatched_target();
  HIGHWAY_R_END
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 3));
  SET_VECTOR_ELT(out, 0, names_of(sup));
  SET_VECTOR_ELT(out, 1, names_of({{P}}_extra::compiled_targets()));
  SET_VECTOR_ELT(out, 2, names_of(disp));
  SEXP nms = PROTECT(Rf_allocVector(STRSXP, 3));
  SET_STRING_ELT(nms, 0, Rf_mkChar("supported"));
  SET_STRING_ELT(nms, 1, Rf_mkChar("compiled"));
  SET_STRING_ELT(nms, 2, Rf_mkChar("dispatched"));
  Rf_setAttrib(out, R_NamesSymbol, nms);
  UNPROTECT(2);
  return out;
}

// "" resets; otherwise restricts this package's dispatch to one target.
SEXP {{P}}_force_target(SEXP name) {
  const char* nm = CHAR(STRING_ELT(name, 0));
  int ok = 0;
  HIGHWAY_R_BEGIN
    hwy::SetSupportedTargetsForTest(0);
    if (nm[0] == '\0') {
      ok = 1;
    } else {
      const int64_t bit = bit_of(nm);
      if (bit != 0 &&
          (hwy::SupportedTargets() & {{P}}_extra::compiled_targets() & bit)) {
        hwy::SetSupportedTargetsForTest(bit);
        ok = 1;
      }
    }
  HIGHWAY_R_END
  return Rf_ScalarLogical(ok);
}

SEXP {{P}}_chosen_initialized(void) {
  return Rf_ScalarLogical(hwy::GetChosenTarget().IsInitialized() ? 1 : 0);
}

SEXP {{P}}_deinit(void) {
  hwy::GetChosenTarget().DeInit();
  return R_NilValue;
}

SEXP {{P}}_warm_up(void) {
  HIGHWAY_R_BEGIN
    highway::warm_up();
  HIGHWAY_R_END
  return R_NilValue;
}

SEXP {{P}}_set_abort_handler(SEXP on) {
  hwy::SetAbortFunc(Rf_asLogical(on) == TRUE ? &counting_handler : nullptr);
  return R_NilValue;
}

SEXP {{P}}_handler_calls(void) {
  return Rf_ScalarInteger(g_handler_calls.load());
}

SEXP {{P}}_abort(SEXP code) {
  const int c = Rf_asInteger(code);
  HIGHWAY_R_BEGIN
    {{P}}_extra::abort_kernel(c);
  HIGHWAY_R_END
  return R_NilValue;
}

// Abort on each of `nthreads` worker threads, each caught by c_guard on its
// own thread. Returns the status codes.
SEXP {{P}}_abort_workers(SEXP code, SEXP nthreads) {
  const int c = Rf_asInteger(code);
  const int nt = Rf_asInteger(nthreads);
  SEXP out = PROTECT(Rf_allocVector(INTSXP, nt));
  int* po = INTEGER(out);
  HIGHWAY_R_BEGIN
    std::vector<std::thread> pool;
    for (int t = 0; t < nt; ++t) {
      pool.emplace_back([&, t] {
        char msg[128];
        po[t] = highway::c_guard([&] { {{P}}_extra::abort_kernel(c); }, msg,
                                 sizeof(msg));
      });
    }
    for (auto& th : pool) th.join();
  HIGHWAY_R_END
  UNPROTECT(1);
  return out;
}

// exp on `nthreads` threads at once, each over its own chunk of x; if cold,
// reset dispatch first so that all threads race on the first dispatch.
SEXP {{P}}_exp_threads(SEXP x, SEXP nthreads, SEXP cold) {
  const size_t n = static_cast<size_t>(Rf_xlength(x));
  const int nt = Rf_asInteger(nthreads);
  const bool reset = Rf_asLogical(cold) == TRUE;
  SEXP out = PROTECT(Rf_allocVector(REALSXP, static_cast<R_xlen_t>(n)));
  const double* px = REAL(x);
  double* po = REAL(out);
  std::vector<int> status(static_cast<size_t>(nt), -1);
  HIGHWAY_R_BEGIN
    if (reset) hwy::GetChosenTarget().DeInit();
    std::atomic<int> ready{0};
    std::vector<std::thread> pool;
    const size_t chunk = (n + static_cast<size_t>(nt) - 1) / static_cast<size_t>(nt);
    for (int t = 0; t < nt; ++t) {
      pool.emplace_back([&, t] {
        const size_t begin = static_cast<size_t>(t) * chunk;
        const size_t end = begin + chunk < n ? begin + chunk : n;
        status[static_cast<size_t>(t)] = highway::c_guard([&] {
          ready.fetch_add(1);
          while (ready.load() < nt) {
          }
          if (begin < end) {
            {{P}}_hwy::exp_doubles(px + begin, po + begin, end - begin);
          }
        });
      });
    }
    for (auto& th : pool) th.join();
  HIGHWAY_R_END
  for (int s : status) {
    if (s != HIGHWAY_STATUS_OK) Rf_error("worker failed with status %d", s);
  }
  UNPROTECT(1);
  return out;
}

}  // extern "C"
