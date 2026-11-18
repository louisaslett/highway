// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Native routine registration for the R package 'highway'.

#define R_NO_REMAP
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <Rinternals.h>

#include "highway/runtime.h"

extern "C" {
SEXP hwy_version(void);
SEXP hwy_supported_targets(void);
SEXP hwy_compiled_targets(void);
SEXP hwy_dispatched_target(void);
SEXP hwy_lanes_f64(void);
SEXP hwy_demo_exp(SEXP);
SEXP hwy_demo_log(SEXP);
SEXP hwy_demo_sum(SEXP, SEXP);
SEXP hwy_test_force_target(SEXP);
SEXP hwy_test_spill(SEXP);
SEXP hwy_test_abort_main(SEXP);
SEXP hwy_test_abort_worker(SEXP);
SEXP hwy_test_threads(SEXP, SEXP, SEXP);
}

namespace {

// Casting through void (*)(void) is the documented way to convert between
// function pointer types without -Wcast-function-type.
template <class F>
DL_FUNC as_dl_func(F f) {
  return reinterpret_cast<DL_FUNC>(reinterpret_cast<void (*)(void)>(f));
}

}  // namespace

extern "C" attribute_visible void R_init_highway(DllInfo* dll) {
  static const R_CallMethodDef call_entries[] = {
      {"hwy_version", as_dl_func(&hwy_version), 0},
      {"hwy_supported_targets", as_dl_func(&hwy_supported_targets), 0},
      {"hwy_compiled_targets", as_dl_func(&hwy_compiled_targets), 0},
      {"hwy_dispatched_target", as_dl_func(&hwy_dispatched_target), 0},
      {"hwy_lanes_f64", as_dl_func(&hwy_lanes_f64), 0},
      {"hwy_demo_exp", as_dl_func(&hwy_demo_exp), 1},
      {"hwy_demo_log", as_dl_func(&hwy_demo_log), 1},
      {"hwy_demo_sum", as_dl_func(&hwy_demo_sum), 2},
      {"hwy_test_force_target", as_dl_func(&hwy_test_force_target), 1},
      {"hwy_test_spill", as_dl_func(&hwy_test_spill), 1},
      {"hwy_test_abort_main", as_dl_func(&hwy_test_abort_main), 1},
      {"hwy_test_abort_worker", as_dl_func(&hwy_test_abort_worker), 1},
      {"hwy_test_threads", as_dl_func(&hwy_test_threads), 3},
      {nullptr, nullptr, 0}};
  R_registerRoutines(dll, nullptr, call_entries, nullptr, nullptr);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
  // One-time CPU detection now rather than on the first kernel call.
  highway::warm_up();
}
